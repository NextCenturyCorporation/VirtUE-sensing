/******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/printk.h>
#include "controller.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("In-VirtUE Kernel Controller");

static int __init controller_init(void)
{
    printk(KERN_INFO "In-kernel probe controller\n");
    return 0;
}

static void __exit controller_cleanup(void)
{

}

module_init(controller_init);
module_exit(controller_cleanup);
