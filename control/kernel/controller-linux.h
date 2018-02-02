#ifndef _CONTROLLER_LINUX_H
#define _CONTROLLER_LINUX_H
/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
/* the kernel itself has dynamic trace points, they
 *  need to be part of the probe capability.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
/*#include <trace/ftrace.h>*/
#include <linux/llist.h>
#include <linux/flex_array.h>
#include <linux/socket.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <net/af_unix.h>
#define assert(s) do{if (!(s)) panic(#s);} while(0);
#endif // CONTROLLER_LINUX_H
