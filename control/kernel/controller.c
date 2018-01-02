/******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/

#include "controller-linux.h"
#include "controller.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("In-VirtUE Kernel Controller");

LIST_HEAD(active_sensors);
struct list_head *probe_queues[0x10];

int probe_socket;

struct kthread_work *controller_work;
struct kthread_worker *controller_worker;

struct kernel_sensor ks = {.id="kernel-sensor",
						   .lock=__SPIN_LOCK_UNLOCKED(lock),
						   .kworker=NULL,
						   .kwork=NULL,
						   .probes[0].probe_id="probe-controller",
						   .probes[0].probe_lock=__SPIN_LOCK_UNLOCKED(lock)};


#ifdef NOTHING
/*     struct task_struct *task = get_current); */
static int kernel_ps(void)
{
	int ccode =  0;
	struct task_struct *task;
	uint8_t *header = "USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND";
	printk(KERN_ALERT "%s\n", header);

	for_each_process(task) {
		printk(KERN_ALERT "%s [%d]\n", task->comm, task->pid);
		ccode++;
	}

    return ccode;
}
#endif


/* ugly but expedient way to support < 4.9 kernels */
/* todo: convert to macros and move to header */
/* CONT_INIT_AND_QUEUE, CONT_INIT_WORK, CONT_QUEUE_WORK */

static bool
init_and_queue_work(struct kthread_work *work,
					struct kthread_worker *worker,
					void (*function)(struct kthread_work *))
{
	CONT_INIT_WORK(work, function);
	return CONT_QUEUE_WORK(worker, work);

}

/**
 * Flushes this work off any work_list it is on.
 * If work is executing or scheduled, this routine
 * will block until the work is no longer being run.
 */
void *destroy_probe_work(struct kthread_work *work)
{

	CONT_FLUSH_WORK(work);
	memset(work, 0, sizeof(struct kthread_work));
	return work;
}


/**
 * destroy a struct probe_s, by tearing down
 * its allocated resources.
 * To free a dynamically allocated probe, you
 * could call it like this:
 *
 *   kfree(destroy_k_probe(probe));
 *
 *  The inside call frees memory allocated for the probe id
 *   and probe data, destroys a probe work node if it is there,
 *  and returns a zero'ed out memory buffer to the outer call.
 */
void *destroy_k_probe(struct probe_s *probe)
{
	uint8_t *pid = NULL, *pdata = NULL;
	struct kthread_work *pwork = probe->probe_work;
	pid = probe->probe_id;
	pdata = probe->data;
	memset(probe, 0, sizeof(struct probe_s));
	if (pid) {
		kfree(pid);
	}
	if (pdata) {
		kfree(pdata);
	}
	if (pwork) {
		destroy_probe_work(pwork);
	}
	return probe;
}


struct kthread_worker *
__cont_create_worker(int cpu, unsigned int flags,
					 const char namefmt[], va_list args)
{
	struct kthread_worker *worker;
	struct task_struct *task;
	int node = -1;

	worker = kzalloc(sizeof(*worker), GFP_KERNEL);
	if (!worker)
		return ERR_PTR(-ENOMEM);

	CONT_INIT_WORKER(worker);

	if (cpu >= 0)
		node = cpu_to_node(cpu);
	task = kthread_create_on_node(kthread_worker_fn, worker, cpu, namefmt, args);

	if (IS_ERR(task))
		goto fail_task;

	if (cpu >= 0)
		kthread_bind(task, cpu);
#ifdef NEW_API
	worker->flags = flags;
#endif
	worker->task = task;
	wake_up_process(task);
	return worker;

fail_task:
	kfree(worker);
	return ERR_CAST(task);
}


#ifdef OLD_API
struct kthread_worker *
kthread_create_worker(unsigned int flags, const char namefmt[], ...)
{
	struct kthread_worker *worker;
	va_list args;

	va_start(args, namefmt);
	worker = __cont_create_worker(CONT_CPU_ANY, flags, namefmt, args);
	va_end(args);

	return worker;
}

void kthread_destroy_worker(struct kthread_worker *worker)
{
	struct task_struct *task;

	task = worker->task;
	if (WARN_ON(!task))
		return;

	flush_kthread_worker(worker);
	kthread_stop(task);
	WARN_ON(!list_empty(&worker->work_list));
	kfree(worker);

	;
}

#endif

/**
 * init_k_probe - initialize a kprobe that has already been allocated.
 *  - clear the kprobe memory
 *  - initialize the probe's spinlock and list head
 *  - allocates memory for the probe's ID and data
 *  - does not initialize probe->probe_work with a work node,
 *    which does need to get done by the kthreads library.
 * Initialize a kernel probe structure.
 * void *init_k_probe(struct probe_s *p)
 */
struct probe_s *init_k_probe(struct probe_s *probe)
{
	if (!probe) {
		return ERR_PTR(-ENOMEM);
	}
	memset(probe, 0, sizeof(struct probe_s));
	probe->probe_id = kzalloc(PROBE_ID_SIZE, GFP_KERNEL);
	if (!probe->probe_id) {
		return ERR_PTR(-ENOMEM);
	}

	probe->probe_lock=__SPIN_LOCK_UNLOCKED("probe");
	/* flags, timeout, repeat are zero'ed */
	/* probe_work is NULL */
	INIT_LIST_HEAD(&(probe)->probe_list);

	probe->data = kzalloc(PROBE_DATA_SIZE, GFP_KERNEL);
	if (!probe->data) {
		goto err_exit;
	}
	return probe;

err_exit:
	if (probe->probe_id) {
		kfree(probe->probe_id);
	}
	if (probe->data) {
		kfree(probe->data);
	}
	return ERR_PTR(-ENOMEM);
}


/**
 * A probe routine is a kthread  worker, called from kernel thread.
 * it needs to execute quickly and can't hold any locks or
 * blocking objects. Once it runs once, it must be re-initialized
 * and re-queued to run again...see /include/linux/kthread.h
 **/

/**
 * Sample probe function
 *
 * Called by the "sensor" thread to "probe."
 * This sample keeps track of how many times is has run.
 * Each time the sample runs it increments its count, prints probe
 * information, and either reschedules itself or dies.
 *
 **/
void  k_probe(struct kthread_work *work)
{

	uint64_t *count;
	struct kthread_worker *co_worker = work->worker;
    struct probe_s *probe_struct =
		(struct probe_s *)container_of(&work, struct probe_s, probe_work);

/* this pragma is necessary until we make more explicit use of probe_struct->data */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
	count = (uint64_t *)probe_struct->data;
	*count++;

	printk(KERN_ALERT "probe: %s; flags: %llx; timeout: %lld; repeat: %lld; count: %lld\n",
		   probe_struct->probe_id,
		   probe_struct->flags,
		   probe_struct->timeout,
		   probe_struct->repeat,
		   *count);

//kernel_ps();

	if (probe_struct->repeat) {
		probe_struct->repeat--;

/* audit this workflow, mdd */
		init_and_queue_work(work, co_worker, k_probe);
	}
#pragma GCC diagnostic pop
	return;
}


/**
 * this init reflects the correct workflow - the struct work needs to be
 * initialized and queued into the worker, before running the worker.
 * the worker will dequeue the work and run work->func(work),
 * then someone needs to decide if the work should be requeued.
 **/
static int __init __kcontrol_init(void)
{
	int ccode = 0;
	DMSG();
	controller_work = kzalloc(sizeof(*controller_work), GFP_KERNEL);
	if (!controller_work) {
		DMSG();
		return -ENOMEM;
	}
	controller_worker = kzalloc(sizeof(struct kthread_worker), GFP_KERNEL);
	if (!controller_worker) {
		DMSG();
		ccode = -ENOMEM;
		goto err_exit;
	}

	init_kthread_work(controller_work, k_probe);
	init_kthread_worker(controller_worker);
	queue_kthread_work(controller_worker, controller_work);
	kthread_run(kthread_worker_fn, controller_worker,
				"unremarkable-\%p", &ks);

	return ccode;

err_exit:
	DMSG();
	if (controller_work) {
		kfree(controller_work);
		controller_work = NULL;
	}
	if (controller_worker) {
		kfree(controller_worker);
		controller_worker = NULL;
	}

	return ccode;
}

static void __exit controller_cleanup(void)
{
	printk(KERN_ALERT "controller cleanup\n");
	if (ks.kworker) {
		DMSG();
		kthread_destroy_worker(ks.kworker);
	}

	/* work nodes do not get destroyed along with a worker */
	if (controller_work) {
		DMSG();
		kfree(controller_work);
		controller_work = NULL;
		DMSG();
	}
}

module_init(__kcontrol_init);
module_exit(controller_cleanup);
