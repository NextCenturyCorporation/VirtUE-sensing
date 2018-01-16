/*******************************************************************
 * In-Virtue Kernel Controller
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


static unsigned int ps_repeat = 1;
static unsigned int ps_timeout = 60;

module_param(ps_repeat, uint, 0644);
module_param(ps_timeout, uint, 0644);

MODULE_PARM_DESC(ps_repeat, "How many times to run the kernel ps function");
MODULE_PARM_DESC(ps_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "ps function");


LIST_HEAD(active_sensors);
struct list_head *probe_queues[0x10];

int probe_socket;

struct kthread_work *controller_work;
struct kthread_worker *controller_worker;
struct probe_s *controller_probe;
struct flex_array *kps_data_flex_array;

static int kernel_ps(int count)
{
	static LIST_HEAD(kps_l);
	int ccode = 0, index = 0;
	struct task_struct *task;
	struct kernel_ps_data kpsd;

	for_each_process(task) {
		struct kernel_ps_data *kpsd_p;
		kpsd.index = index;
		kpsd.user_id = task_uid(task);
		kpsd.pid_nr = task->pid;
		memcpy(kpsd.comm, task->comm, TASK_COMM_LEN);

		if (index <  PS_ARRAY_SIZE) {
			flex_array_put(kps_data_flex_array, index, &kpsd, GFP_ATOMIC);
			kpsd_p = flex_array_get(kps_data_flex_array, index);
			printk(KERN_INFO "kernel-ps-%d:%d %s [%d] [%d]\n",
				   count, index, kpsd_p->comm, kpsd_p->pid_nr,
				   kpsd_p->user_id.val);
		} else {
			printk(KERN_INFO "kernel-ps-%d:%d-not-stored: %s [%d] [%d]\n",
				   count, index, kpsd.comm, kpsd.pid_nr, kpsd.user_id.val);
		}
		index++;
	}

    return ccode;
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
	assert(probe);

	if (probe->probe_id) {
		kfree(probe->probe_id);
	}
	if (probe->data) {
		kfree(probe->data);
	}

	destroy_probe_work(&probe->probe_work);
	memset(probe, 0, sizeof(struct probe_s));
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
	kfree(worker);
	return;
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

/**
 * note jan 3 2018: don't always zero-out a probe, prepare for
 * showing persistent data in between probes (in between probe runs)
 **/
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
	INIT_LIST_HEAD(&(probe)->l);

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


static inline void sleep(unsigned sec)
{
	__set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(sec * HZ);
}


/**
 * A probe routine is a kthread  worker, called from kernel thread.
 * it needs to execute quickly and can't hold any locks or
 * blocking objects. Once it runs once, it must be re-initialized
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
	struct probe_s *probe_struct =
		container_of(work, struct probe_s, probe_work);
	static int count;

	kernel_ps(++count);

	if (probe_struct->repeat) {
		probe_struct->repeat--;
		sleep(probe_struct->timeout);
		init_and_queue_work(work, co_worker, k_probe);
	}
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
	kps_data_flex_array = flex_array_alloc(PS_DATA_SIZE, PS_ARRAY_SIZE, GFP_KERNEL);
	assert(kps_data_flex_array);

	controller_probe = kzalloc(sizeof(struct probe_s), GFP_KERNEL);
	if (!controller_probe) {
		DMSG();
		return -ENOMEM;
	}

	controller_probe = init_k_probe(controller_probe);
	if (controller_probe == ERR_PTR(-ENOMEM)) {
		DMSG();
		ccode = -ENOMEM;
		goto err_exit;
	}
	controller_probe->repeat = ps_repeat;
	controller_probe->timeout = ps_timeout;

	controller_work = &controller_probe->probe_work;

	controller_worker = kzalloc(sizeof(struct kthread_worker), GFP_KERNEL);
	if (!controller_worker) {
		DMSG();
		ccode = -ENOMEM;
		goto err_exit;
	}

	CONT_INIT_WORK(controller_work, k_probe);
	CONT_INIT_WORKER(controller_worker);
	CONT_QUEUE_WORK(controller_worker, controller_work);

	kthread_run(kthread_worker_fn, controller_worker,
				"unremarked\%lx", (unsigned long)controller_probe);
	return ccode;

err_exit:
	DMSG();
	if (controller_probe) {
		kfree(controller_probe);
		controller_probe = NULL;
	}
	if (controller_worker) {
		kfree(controller_worker);
		controller_worker = NULL;
	}

	return ccode;
}

static void __exit controller_cleanup(void)
{
	if (controller_probe) {
		DMSG();
		destroy_k_probe(controller_probe);
		kfree(controller_probe);
	}

	if (controller_worker) {
		DMSG();
		kthread_destroy_worker(controller_worker);
	}

}

module_init(__kcontrol_init);
module_exit(controller_cleanup);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO);
