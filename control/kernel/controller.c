/******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include "controller.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("In-VirtUE Kernel Controller");

LIST_HEAD(active_probes);
int probe_socket;

/* template probe function  */
int _pr(uint64_t flags, uint8_t *probe_data) 
{
	return 0;
}

struct probe_s p = {.id="probe-controller",
					.lock=__SPIN_LOCK_UNLOCKED(lock),
					.probe=_pr};

static int __init controller_init(void)
{
    printk(KERN_INFO "In-kernel probe controller, %s\n", p.id);
    return 0;
}

static void __exit controller_cleanup(void)
{

}

module_init(controller_init);
module_exit(controller_cleanup);
