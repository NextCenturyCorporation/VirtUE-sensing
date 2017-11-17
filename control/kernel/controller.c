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

/* template probe function  */
int _pr(uint64_t flags, uint8_t *probe_data)
{
	return 0;
}

void  k_sensor(struct kthread_work *work)
{
/* ok, this is an opportunity for the sensors and their probes to
 * have a more stable presence
 * we have the kthread_work struct, now we need to index our other
 * information.
 */
	struct kernel_sensor *ks_ = NULL, *bs_ = NULL;
	ks_ = (struct kernel_sensor *) container_of(work,  struct kernel_sensor, kwork);

/* now put myself on the list to run again */

	while (1){
		if (ks_ != bs_) {
			struct task_struct *ts_inside_info;
			bs_ = (struct kernel_sensor *) container_of(work, struct kernel_sensor, kwork);
			ts_inside_info = bs_->kworker.task;
			if (bs_ == ks_) {
				printk("%s\n", ts_inside_info->comm);
			}
		}

		/* run each probe as it times out */
		break;
	}

	return ;
}

struct kernel_sensor ks = {.id="kernel-sensor",
						   .lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].id="probe-controller",
						   .probes[0].lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].probe=_pr};
/* init function for the controller, creates a worker
 *  and links a work node into the worker's list, runs the worker. */
static int __init controller_init(void)
{
	struct kthread_worker *kt_wr = &ks.kworker;
	struct kthread_work *kt_wk = &ks.kwork;

	kt_wr->lock = __SPIN_LOCK_UNLOCKED((kt_wr)->lock);  /* spinlock_t */
	kt_wr->work_list.next = kt_wr->work_list.prev = &(kt_wr->work_list);
	kt_wk->node.next = kt_wk->node.prev = &(kt_wk->node);
	kt_wk->func = k_sensor;

	printk("%p\n", kt_wr);

	ks.kworker.task = kthread_run(kthread_worker_fn, &ks.kwork, "run-kernel-probes");
	if (IS_ERR(ks.kworker.task)) {
		WARN_ON(1);
	}

	/* has k_sensor already been queued? */
	/* if so, is it able to re-queue itself? */
	/*queue_kthread_work(&ks.kworker, &ks.kwork); */

	return 0;
}

static void __exit controller_cleanup(void)
{
	kthread_stop(ks.kworker.task);
}

module_init(controller_init);
module_exit(controller_cleanup);
