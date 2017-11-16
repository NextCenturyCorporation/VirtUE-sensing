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

struct kernel_sensor ks = {.id="kernel-sensor",
						   .lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].id="probe-controller",
						   .probes[0].lock=__SPIN_LOCK_UNLOCKED(lock),
						   .probes[0].probe=_pr};

static int run_controller(void *__unused)
{
	while (!kthread_should_stop()) {
		schedule_timeout_interruptible(HZ*30);
	}
	return 0;
}



static int __init controller_init(void)
{
	ks.sensor_task = kthread_run(run_controller, &ks, "run-kernel-probes");
	if (IS_ERR(ks.sensor_task)) {
		WARN_ON(1);
	}
	return 0;
}

static void __exit controller_cleanup(void)
{
	kthread_stop(ks.sensor_task);
}

module_init(controller_init);
module_exit(controller_cleanup);
