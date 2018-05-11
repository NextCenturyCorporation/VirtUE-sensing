#ifndef _CONTROLLER_LINUX_H
#define _CONTROLLER_LINUX_H
/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/stat.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/cred.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
/*#include <trace/ftrace.h>*/
#include <linux/rculist.h>
#include <linux/flex_array.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <net/af_unix.h>
#include <asm/segment.h>
#include <linux/syscalls.h>
#include <uapi/linux/stat.h>
#include <asm/atomic64_64.h>
#define assert(s) if (!(s)) {DMSG(); printk(KERN_DEBUG "panic\n");}
#endif // CONTROLLER_LINUX_H
