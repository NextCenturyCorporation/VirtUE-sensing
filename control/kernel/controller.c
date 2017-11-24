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
	  printk("nothing to see here\n");
	  ssleep(60);
  }
  DMSG();

  return;
}


/* init function for the controller, creates a worker
 *  and links a work node into the worker's list, runs the worker. */
static int __init controller_init(void)
{

	int ccode = 0;
/* kthread_create_worker */

	struct kthread_work *my_work;
	struct kthread_worker *my_worker;
	DMSG();

	my_work = kmalloc(sizeof(*my_work), GFP_KERNEL);
	if (!my_work) {
		DMSG();
		return -ENOMEM;
	}

	do {
	  my_worker = kthread_create_worker(0, "unremarkable-\%p", &ks);
		schedule();
		if (ERR_PTR(-ENOMEM) == my_worker) {
			ccode = -ENOMEM;
			goto err_exit;
		}
	} while (ERR_PTR(-EINTR) == my_worker);
	
	printk(KERN_ALERT "my_worker is %p; my_work is %p\n", my_worker, my_work);
	DMSG();

#ifdef OLD_API
	init_kthread_work(my_work, k_sensor);
#else
	kthread_init_work(my_work, k_sensor);
#endif
	ks.kworker = my_worker;
	ks.kwork = my_work;
	DMSG();
#ifdef OLD_API
	queue_kthread_work(my_worker, my_work);
#else
	kthread_queue_work(my_worker, my_work);
#endif
	DMSG();

err_exit:
	return ccode;
	
}

static void __exit controller_cleanup(void)
{
	kthread_destroy_worker(ks.kworker);
	
}

module_init(controller_init);
module_exit(controller_cleanup);
