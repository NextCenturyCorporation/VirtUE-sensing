/*******************************************************************
 * in-Virtue Kernel Controller
 *
 * Copyright (C) 2017-2018  Michael D. Day II
 * Copyright (C) 2017-2018  Two Six Labs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *******************************************************************/

#include "controller-linux.h"
#include "controller.h"

/**
 * move these into struct kernel_ps_probe
 **/
static unsigned int ps_repeat = 1;
static unsigned int ps_timeout = 60;

module_param(ps_repeat, uint, 0644);
module_param(ps_timeout, uint, 0644);

MODULE_PARM_DESC(ps_repeat, "How many times to run the kernel ps function");
MODULE_PARM_DESC(ps_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "ps function");

LLIST_HEAD(sensors);

struct kernel_sensor k_sensor;

/** locking the kernel-ps flex_array
 * to write to or read from the pre-allocated flex array parts,
 * one must hold the kps_spinlock. The kernel_ps function will
 * execute a trylock and return with -EAGAIN if it is unable
 * to lock the array.
 *
 * A reader can decide best how to poll for the array spinlock, but
 * it should hold the lock while using pointers from the array.
 **/

/**
 * output the kernel-ps list to the kernel log
 *
 * @param uint8 *tag - tag added to each line, useful for filtering
 *        log output.
 * @param count The number of times kernel-ps has been run as a probe
 * @param nonce Identifies flex_array elements that should be printed,
 *              also terminates the printing loop
 * @return -EAGAIN if unable to obtain kernel-ps flex array lock,
 *         -ENOMEM if the ps flex_array is too small to contain every
 *                 running process
 *         The number of kernel-ps records printed if successful
 **/

static int print_kernel_ps(struct kernel_ps_probe *parent,
						   uint8_t *tag,
						   uint64_t nonce,
						   int count)
{
	int index;
	unsigned long flags;
	struct kernel_ps_data *kpsd_p;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}
	for (index = 0; index < PS_ARRAY_SIZE; index++)  {
		kpsd_p = flex_array_get(parent->kps_data_flex_array, index);
		if (kpsd_p->nonce != nonce) {
		    printk(KERN_INFO "print_kernel_ps exiting NONCE %llx KPSD NONCE: %llx\n",
				   nonce, kpsd_p->nonce);
			break;
		}

		printk(KERN_INFO "%s %d:%d %s [%d] [%d] [%llx]\n",
			   tag, count, index, kpsd_p->comm, kpsd_p->pid_nr,
			   kpsd_p->user_id.val, nonce);
	}
	spin_unlock_irqrestore(&parent->lock, flags);
	if (index == PS_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
}

int kernel_ps(struct kernel_ps_probe *parent, int count, uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	struct kernel_ps_data kpsd;
	unsigned long flags;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}

	for_each_process(task) {
		kpsd.nonce = nonce;
		kpsd.index = index;
		kpsd.user_id = task_uid(task);
		kpsd.pid_nr = task->pid;
		memcpy(kpsd.comm, task->comm, TASK_COMM_LEN);
		if (index <  PS_ARRAY_SIZE) {
			flex_array_put(parent->kps_data_flex_array, index, &kpsd, GFP_ATOMIC);
		} else {
			index = -ENOMEM;
			goto unlock_out;
		}
		index++;
	}

unlock_out:
	spin_unlock_irqrestore(&parent->lock, flags);
    return index;
}


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
 * destroy a struct probe, by tearing down
 * its allocated resources.
 * To free a dynamically allocated probe, you
 * could call it like this:
 *
 *   kfree(destroy_probe(probe));
 *
 *  The inside call frees memory allocated for the probe id
 *   and probe data, destroys a probe work node if it is there,
 *  and returns a zero'ed out memory buffer to the outer call.
 */
void *destroy_probe(struct probe *probe)
{
	assert(probe && (probe->flags & PROBE_INITIALIZED));

	if (probe->id && (probe->flags & PROBE_HAS_ID_FIELD)) {
		kfree(probe->id);
		probe->id = NULL;
	}
	if (probe->data && (probe->flags & PROBE_HAS_DATA_FIELD)) {
		kfree(probe->data);
		probe->data = NULL;
	}
/* TODO: audit kernel/kthread.c to make sure we are not double-dequeueing
   work- e...g. is flush_kthread_worker sufficient */
	if (probe->flags & PROBE_HAS_WORK) {
		kthread_destroy_worker(&probe->worker);
		destroy_probe_work(&probe->work);
	}

	return probe;
}

void *destroy_kernel_sensor(struct kernel_sensor *sensor)
{

	struct probe *probe, *tmp;
	struct llist_node *llnode ;
	assert(sensor);

	/* sensor is the parent of all probes */
	if (!llist_empty(&sensor->l_head)) {
		llnode = llist_del_all(&sensor->l_head);
		llist_for_each_entry_safe(probe, tmp, llnode, l_node) {
			if (probe->flags & PROBE_KPS) {
				((struct kernel_ps_probe *)probe)->_destroy(probe);
			} else {
				probe->destroy(probe);
			}
			kfree(probe);
		}
	}
	/* now destroy the sensor's anonymous probe struct */
    probe = (struct probe *)sensor;
	destroy_probe(probe);
	memset(sensor, 0, sizeof(struct kernel_sensor));
	return sensor;
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

/**
 * shuts down a kthread worker, but does not free the
 * memory - part of the transisiton to the worker belonging
 * inside the probe struct.
 **/
void
kthread_destroy_worker(struct kthread_worker *worker)

{
	struct task_struct *task;

	task = worker->task;
	if (WARN_ON(!task)) {
		return;
	}

	flush_kthread_worker(worker);
	kthread_stop(task);
	WARN_ON(!list_empty(&worker->work_list));
	return;
}

#endif


/**
 * init_k_sensor - initialize a kernel sensor, which is the parent
 * of k_probes.
 *
 * sensor is already allocated
 *
 * - clear sensor memory
 * - init the spinlock and probe list head
 * - allocate memory for sensor's ID and data
 * - does not initialize kwork work node, which is done by the
 *   kthreads library.
 **/
struct kernel_sensor * init_kernel_sensor(struct kernel_sensor *sensor)
{
    if (!sensor) {
		return ERR_PTR(-ENOMEM);
	}
	memset(sensor, 0, sizeof(struct kernel_sensor));

/* we have an anonymous struct probe, need to initialize it.
 * the anonymous struct is the first member of this structure.
 */
	init_probe((struct probe *)sensor,
			   "Kernel Sensor", strlen("Kernel Sensor") + 1,
			   NULL, -1);
	init_llist_head(&sensor->l_head);
	init_llist_head(&sensor->probes);
	sensor->_destroy = destroy_kernel_sensor;
	/* initialize the socket later when we listen*/
	return(sensor);
}




static inline void sleep(unsigned sec)
{
	__set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(sec * HZ);
}


/**
 * A probe routine is a kthread  worker, called from kernel thread.
 * Once it runs once, it must be re-initialized
 * and re-queued to run again...see /include/linux/kthread.h
 **/

/**
 * ps probe function
 *
 * Called by the kernel contoller kmod to "probe" the processes
 * running on this kernel. Keeps track of how many times is has run.
 * Each time the sample runs it increments its count, prints probe
 * information, and either reschedules itself or dies.
 **/
void  k_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_ps_probe *probe_struct =
		container_of(work, struct kernel_ps_probe, work);
	static int count;
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));

	/**
	 * if another process is reading the ps flex_array, kernel_ps
	 * will return -EFAULT. therefore, reschedule and try again.
	 */
	while( probe_struct->ps(probe_struct, count, nonce) == -EAGAIN) {
		schedule();
	}
/**
 *  call print_kernel_ps by default. But, in the future there will be other             *  options, notably outputting in json format to a socket
 **/
	probe_struct->print(probe_struct, "kernel-ps", nonce, ++count);

	if (probe_struct->repeat) {
		probe_struct->repeat--;
		sleep(probe_struct->timeout);
		init_and_queue_work(work, co_worker, k_probe);
	}
	return;
}


/**
 * init_probe - initialize a probe that has already been allocated.
 * struct probe is usually used as an anonymous structure in a more
 * specialized type of probe, for example kernel-ps or kernel-lsof
 *  - probe memory is already allocated and should be zero'd by the
 *    caller
 *  - initialize the probe's spinlock and list head
 *  - possibly allocates memory for the probe's ID and data
 *  - does not initialize probe->probe_work with a work node,
 *    which does need to get done by the kthreads library.
 * Initialize a probe structure.
 * void *init_probe(struct probe *p, uint8_t *id, uint8_t *data)
 *
 **/

struct probe *init_probe(struct probe *probe,
						 uint8_t *id, int id_size,
						 uint8_t *data, int data_size)
{
	if (!probe) {
		return ERR_PTR(-ENOMEM);
	}

	probe->init =  init_probe;
	probe->destroy = destroy_probe;
	if (id && id_size > 0) {
		probe->id = kzalloc(id_size, GFP_KERNEL);
		if (!probe->id) {
			return ERR_PTR(-ENOMEM);
		}
		probe->flags |= PROBE_HAS_ID_FIELD;
		memcpy(probe->id, id, id_size);
	}

	if (data && data_size > 0) {
		probe->data = kzalloc(data_size, GFP_KERNEL);
		if (!probe->data) {
			goto err_exit;
		}
		probe->flags |= PROBE_HAS_DATA_FIELD;
		memcpy(probe->data, data, data_size);
	}

	probe->lock=__SPIN_LOCK_UNLOCKED("probe");
	/* flags, timeout, repeat are zero'ed */
	/* probe_work is NULL */

	probe->flags |= PROBE_INITIALIZED;
	return probe;

err_exit:
	if (probe->id && (probe->flags & PROBE_HAS_ID_FIELD)) {
		kfree(probe->id);
	}
	return ERR_PTR(-ENOMEM);
}


static void *destroy_kernel_ps_probe(struct probe *probe)
{
	struct kernel_ps_probe *ps_p = (struct kernel_ps_probe *)probe;
	assert(ps_p);

	if (probe->flags & PROBE_INITIALIZED) {
		destroy_probe(probe);
	}

	if (ps_p->kps_data_flex_array) {
		flex_array_free(ps_p->kps_data_flex_array);
	}

	memset(ps_p, 0, sizeof(struct kernel_ps_probe));
	return ps_p;
}

struct kernel_ps_probe *init_kernel_ps_probe(struct kernel_ps_probe *ps_p,
											 uint8_t *id, int id_len,
											 int (*print)(struct kernel_ps_probe *,
														  uint8_t *, uint64_t, int))
{
	int ccode = 0;
	struct probe *tmp;

	if (!ps_p) {
		return ERR_PTR(-ENOMEM);
	}
	memset(ps_p, 0, sizeof(struct kernel_ps_probe));
	/* init the anonymous struct probe */

	tmp = init_probe((struct probe *)ps_p, id, id_len, NULL, -1);
	/* tmp will be a good pointer if init returned successfully,
	   an error pointer otherwise */
	if (ps_p != (struct kernel_ps_probe *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}

	/** init timeout and repeat
	 * they are passed on the command line, or (eventually) read
	 * from sysfs
     **/
	ps_p->timeout = ps_timeout;
	ps_p->repeat = ps_repeat;

	ps_p->_init = init_kernel_ps_probe;
	ps_p->_destroy = destroy_kernel_ps_probe;
	if (print) {
		ps_p->print = print;
	} else {
		ps_p->print = print_kernel_ps;
	}

	ps_p->kps_data_flex_array =
		flex_array_alloc(PS_DATA_SIZE, PS_ARRAY_SIZE, GFP_KERNEL);

	if (!ps_p->kps_data_flex_array) {
/* flex_array_alloc will return NULL upon failure, a valid pointer otherwise */
		ccode = -ENOMEM;
		goto err_exit;
	}
	ccode = flex_array_prealloc(ps_p->kps_data_flex_array, 0, PS_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_flex_array;
	}

	printk(KERN_INFO "PS_DATA_SIZE %ld PS_ARRAY_SIZE %ld\n",
		   PS_DATA_SIZE, PS_ARRAY_SIZE);


/* now queue the kernel thread work structures */
	CONT_INIT_WORK(&ps_p->work, k_probe);
	ps_p->flags |= PROBE_HAS_WORK;
	CONT_INIT_WORKER(&ps_p->worker);
	CONT_QUEUE_WORK(&ps_p->worker,
					&ps_p->work);
	kthread_run(kthread_worker_fn, &ps_p->worker,
				"kernel-ps\%lx", (unsigned long)ps_p);
	return ps_p;

err_free_flex_array:
	flex_array_free(ps_p->kps_data_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
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
	struct kernel_ps_probe *ps_probe = NULL;

	if (&k_sensor != init_kernel_sensor(&k_sensor)) {
		return -ENOMEM;
	}
	ps_probe = kzalloc(sizeof(struct kernel_ps_probe), GFP_KERNEL);
	if (!ps_probe) {
		DMSG();
		ccode = -ENOMEM;
		goto err_exit;
	}

	ps_probe = init_kernel_ps_probe(ps_probe,
						 "Kernel PS Probe",
						 strlen("Kernel PS Probe") + 1,
						 print_kernel_ps);

	if (ps_probe == ERR_PTR(-ENOMEM)) {
		DMSG();
		ccode = -ENOMEM;
		goto err_exit;
	}

	/* link this probe to the sensor struct */
	if (! llist_add(&ps_probe->l_node, &k_sensor.l_head)) {
		ccode = -EINVAL;
		goto err_exit;
	}

	return ccode;

err_exit:
	DMSG();
	if (ps_probe != ERR_PTR(-ENOMEM)) {
		if (ps_probe->flags & PROBE_INITIALIZED) {
			ps_probe->_destroy((struct probe *)ps_probe);
		}
		kfree(ps_probe);
		ps_probe = NULL;
	}
	return ccode;
}

/**
 * now there is a sensor struct, and probes are linked to the sensor in a list
 *
 **/
static void __exit controller_cleanup(void)
{
	/* destroy, but do not free the sensor */
	/* sensor is statically allocated, no need to free it. */
	k_sensor._destroy(&k_sensor);

}

module_init(__kcontrol_init);
module_exit(controller_cleanup);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO);
