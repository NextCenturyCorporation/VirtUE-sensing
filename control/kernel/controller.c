/******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include "controller.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("In-VirtUE Kernel Controller");

LIST_HEAD(active_sensors);
struct list_head *probe_queues[0x10];

int probe_socket;

struct kthread_work *co_work;
struct kthread_worker *co_worker;

struct kernel_sensor ks = {.id="kernel-sensor",
						   .lock=__SPIN_LOCK_UNLOCKED(lock),
						   .kworker=NULL,
						   .kwork=NULL,
						   .probes[0].id="probe-controller",
						   .probes[0].lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].probe=k_probe};

/* ugly but expedient way to support < 4.9 kernels */
static bool
init_and_queue_work(struct kthread_work *work,
					struct kthread_worker *worker,
					void (*function)(struct kthread_work *))
{
#ifdef OLD_API
	init_kthread_work(work, function);
#else
	kthread_init_work(work, function);
#endif

#ifdef OLD_API
	return queue_kthread_work(worker, work);
#else
	return kthread_queue_work(worker, work);
#endif

}



/* This is a worker, called from kernel worker thread.
 * it needs to execute quickly and can't hold any locks or
 * blocking objects. Once it runs once, it must be re-initialized
 * and re-queued to run again..
 */

void  k_probe(struct kthread_work *work)
{
	static int countdown = 4;
	struct kthread_worker *co_worker = work->worker;
	printk(KERN_ALERT "generic probe function\n");
	if (countdown) {
		countdown--;
		init_and_queue_work(work, co_worker, k_probe);
	}

  return;
}


/* init function for the controller, creates a worker
 *  and links a work node into the worker's list, runs the worker. */
static int __init controller_init(void)
{

	int ccode = 0;
	DMSG();

	co_work = kmalloc(sizeof(*co_work), GFP_KERNEL);
	if (!co_work) {
		DMSG();
		return -ENOMEM;
	}

	do {
	  co_worker = kthread_create_worker(0, "unremarkable-\%p", &ks);
		schedule();
		if (ERR_PTR(-ENOMEM) == co_worker) {
			ccode = -ENOMEM;
			goto err_exit;
		}
	} while (ERR_PTR(-EINTR) == co_worker);

	printk(KERN_ALERT "co_worker is %p; co_work is %p\n", co_worker, co_work);
	DMSG();

err_exit:
	kfree(co_work);
	co_work = NULL;
	return ccode;
}

static void __exit controller_cleanup(void)
{
	printk(KERN_ALERT "controller cleanup\n");
    kthread_destroy_worker(ks.kworker);
	/* work nodes do not get destroyed along with a worker */
	if (co_work) {
		kfree(co_work);
	}
}

module_init(controller_init);
module_exit(controller_cleanup);
