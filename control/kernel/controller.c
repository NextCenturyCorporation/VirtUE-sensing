/******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include "controller.h"
#include "uname.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("In-VirtUE Kernel Controller");

LIST_HEAD(active_sensors);
struct list_head *probe_queues[0x10];

int probe_socket;

int _pr(uint64_t, uint8_t *);


struct kernel_sensor ks = {.id="kernel-sensor",
						   .lock=__SPIN_LOCK_UNLOCKED(lock),
						   .kworker=NULL,
						   .kwork=NULL,
						   .probes[0].id="probe-controller",
						   .probes[0].lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].probe=_pr};

/* template probe function  */
int _pr(uint64_t flags, uint8_t *probe_data)
{
	return 0;
}

/* a generic sensor function, called from kernel worker thread. */
/* a sensor that did something would replace the schedule call */
/* with some sleep/reschedule logic */
void  k_sensor(struct kthread_work *work)
{

  DMSG();

  while(!kthread_should_stop()) {
	  ssleep(5);
    printk("%s\n", ks.kworker->task->comm);
  }
  DMSG();

  return;
}


static inline struct kthread_worker *init_worker(void)
{
	struct kthread_worker *w = kmalloc(sizeof(*w), GFP_KERNEL);
	if (!w)
		return NULL;
#ifdef CONT_OLD_API
	init_kthread_worker(w);
#else
	kthread_init_worker(w);
#endif
	return w;
}


static inline struct kthread_work *init_work(void (fn)(struct kthread_work *) )
{
	struct kthread_work *w = kmalloc(sizeof(*w), GFP_KERNEL);
	if (!w)
		return NULL;
#ifdef CONT_OLD_API
	init_kthread_work(w, fn);
#else
	kthread_init_work(w, fn);
#endif
	return w;
}

int started_task = 0;

/* init function for the controller, creates a worker
 *  and links a work node into the worker's list, runs the worker. */
static int __init controller_init(void)
{

	int ccode = -ENOMEM;
	struct kthread_work *my_work = init_work(k_sensor);
	struct kthread_worker *my_worker = init_worker();
	DMSG();

	if (my_work == NULL || my_worker == NULL)
	{
		DMSG();
		goto err_exit;
	}

	DMSG();
	printk(KERN_ALERT "my_worker is %p; my_work is %p\n", my_worker, my_work);
	DMSG();
	my_worker->task = kthread_run(kthread_worker_fn, my_work, "run-kernel-probes");
	DMSG();
	if (IS_ERR(my_worker->task)) {
		ccode = -ENOSYS;
		DMSG();
		printk(KERN_ALERT "error starting the worker thread kthread_worker_fn %p"\
			   "my_work %p my_worker %p\n", kthread_worker_fn,
			   my_work, my_worker);
		goto err_exit;
	}
	started_task = 1;
	DMSG();

	printk("%p\n", ks.kworker->task);
	return 0;

err_exit:
	if(my_work != NULL) {
		kfree(my_work);
		my_work = NULL;
	}

	if(my_worker != NULL) {
		kfree(my_worker);
		my_worker = NULL;
	}

	DMSG();
	return ccode;
}

static void __exit controller_cleanup(void)
{
	if (started_task == 1) {
		kthread_stop(ks.kworker->task);
		/* remainder of kernel_sensor shutdown goes here */
	}

}

module_init(controller_init);
module_exit(controller_cleanup);
