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
	static int did_not_run=1;
	while (did_not_run){
		printk("%s\n", ks.kworker->task->comm);
		did_not_run = 0;
	}

	schedule();

	return ;
}


static inline struct kthread_worker *init_worker(void)
{
	struct kthread_worker *w = kmalloc(sizeof(*w), GFP_KERNEL);
	if (!w)
		return ERR_PTR(-ENOMEM);

	w->lock = __SPIN_LOCK_UNLOCKED((ktr)->lock);  /* spinlock_t */
	w->work_list.next = w->work_list.prev = &(w->work_list);
	return w;
}


static inline struct kthread_work *init_work(void (fn)(struct kthread_work *) )
{
	struct kthread_work *w = kmalloc(sizeof(*w), GFP_KERNEL);
	if (!w)
		return ERR_PTR(-ENOMEM);

	w->node.next = w->node.prev = &(w->node);
	w->func = fn;
	return w;
}
/* init function for the controller, creates a worker
 *  and links a work node into the worker's list, runs the worker. */
static int __init controller_init(void)
{

	int ccode = -ENOMEM;
	struct kthread_work *my_work = init_work(k_sensor);
	struct kthread_worker *my_worker = init_worker();
	ks.kworker = my_worker;
	ks.kwork = my_work;

	/* note for now we are using the statically defined sensor struct, */
	/* if needed, we can dyamically allocate it. */
	if (my_work == NULL || my_worker == NULL)
		goto err_exit;

	my_worker->task = kthread_run(kthread_worker_fn, my_work, "run-kernel-probes");
	if (IS_ERR(my_worker->task)) {
		ccode = -ENOSYS;
		goto err_exit;
	}

	printk("%p\n", ks.kworker->task);
	return 0;

err_exit:
	if(my_work != NULL)
		kfree(my_work);
	if(my_worker != NULL)
		kfree(my_worker);
	return ccode;
}

static void __exit controller_cleanup(void)
{
	kthread_stop(ks.kworker->task);
	if (ks.kworker != NULL)
		kfree(ks.kworker);
	if (ks.kwork != NULL)
		kfree(ks.kwork);
}

module_init(controller_init);
module_exit(controller_cleanup);
