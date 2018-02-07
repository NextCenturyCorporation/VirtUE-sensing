/*******************************************************************
 * In-Virtue Kernel Controller
 *
 * Copyright (C) 2017-2018  Michael D. Day II
 * Copyright (C) 2017-2018  Two Six Labs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *******************************************************************/
#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#define _MODULE_LICENSE "GPL v2"
#define _MODULE_AUTHOR "Michael D. Day II <mike.day@twosixlabs.com>"
#define _MODULE_INFO "In-Virtue Kernel Controller"

/* the kernel itself has dynamic trace points, they
 *  need to be part of the probe capability.
 */

#include "uname.h"

/* - - probe - -
 *
 * A probe is an active sensor. It is a function that runs as a kernel
 * task at regular intervals to obtain information or to check the
 * truth of various assertions.

 * It's expected that most sensors will be comprised of one or more
 * probes; that a sensor is not always an active probe and a probe is
 * not always a sensor.
 *
 * By defining a passive sensor as a special kind of probe--a
 * "passive probe"--we can say that a sensor is always also probe.
 *
 * A Sensor is also one or more passive or active probes.
 *
 * At the prototype stage, a sensor will have a fixed array of
 * un-ininitialized probes, and those probes will be initiailized as
 * they are registered with the kernel.
 */



#define PROBE_ID_SIZE       0x1000
#define PROBE_DATA_SIZE     0x4000
#define PS_HEADER_SIZE      0x100

/* probe flag defines - need to make this into a bitmap */

#define PROBE_INITIALIZED    0x01
#define PROBE_DESTROYED      0x02
#define PROBE_KPS            0x04

/**
 * __task_cred - Access a task's objective credentials
 * @task: The task to query
 *
 * Access the objective credentials of a task.  The caller must hold the RCU
 * readlock.
 *
 * The result of this function should not be passed directly to get_cred();
 * rather get_task_cred() should be used instead.
 *
 *  #define __task_cred(task)
 *	rcu_dereference((task)->real_cred)
 **/


/**
 * #ifdef CONFIG_TASK_XACCT
 * here is RSS memory in struct stack struct:
 * Accumulated RSS usage:
 * u64				acct_rss_mem1;
 * Accumulated virtual memory usage:
 * u64				acct_vm_mem1;
 * stime + utime since last update:
 * u64				acct_timexpd;
 * #endif
 **/

/**
 * executable name, excluding path.
 * - normally initialized setup_new_exec()
 * - access it with [gs]et_task_comm()
 * - lock it with task_lock()
 **/
extern const struct cred *get_task_cred(struct task_struct *);

/*
 * The task state array is a strange "bitmap" of
 * reasons to sleep. Thus "running" is zero, and
 * you can test for combinations of others with
 * simple bit tests.
 */
#if 0
static const char * const task_state_array[] = {
	"R (running)",		/*   0 */
	"S (sleeping)",		/*   1 */
	"D (disk sleep)",	/*   2 */
	"T (stopped)",		/*   4 */
	"t (tracing stop)",	/*   8 */
	"X (dead)",		/*  16 */
	"Z (zombie)",		/*  32 */
};

static inline const char *get_task_state(struct task_struct *tsk)

/**
 * struct task_cputime - collected CPU time counts
 * @utime:		time spent in user mode, in nanoseconds
 * @stime:		time spent in kernel mode, in nanoseconds
 * @sum_exec_runtime:	total time spent on the CPU, in nanoseconds
 *
 * This structure groups together three kinds of CPU time that are tracked for
 * threads and thread groups.  Most things considering CPU time want to group
 * these counts together and treat all three of them in parallel.
 */
	struct task_cputime {
		u64				utime;
		u64				stime;
		unsigned long long		sum_exec_runtime;
	};

static inline void task_cputime(struct task_struct *t,
								u64 *utime, u64 *stime)
{
	*utime = t->utime;
	*stime = t->stime;
}
#endif

struct kernel_ps_data {
	int index; /* used for access to using flex_array.h */
	uint64_t nonce;
	kuid_t user_id;
	int pid_nr;  /* see struct pid.upid.nrin linux/pid.h  */
	uint64_t load_avg;
	uint64_t util_avg; /* see struct sched_avg in linux/sched.h */
	struct files_struct *files;
#define TASK_STATE_LEN 24
	uint8_t state[TASK_STATE_LEN];
	uint64_t start_time; /* task->start_time */
	uint64_t u_time;
	uint64_t s_time;
	uint64_t task_time;
	spinlock_t sl;
	uint8_t comm[TASK_COMM_LEN+1];
};


#define PS_DATA_SIZE sizeof(struct kernel_ps_data)

/**
 * see include/linux/flex_array.h for the definitions of
 * FLEX_ARRAY_NR_BASE_PTRS and FLEX_ARRA_ELEMENTS_PER_PART
 * this is a conservatice calculation to ensure we don't try to
 * pre allocate a flex_array with too many elements
 **/

#define PS_APPARENT_ARRAY_SIZE											\
	FLEX_ARRAY_ELEMENTS_PER_PART(PS_DATA_SIZE) * FLEX_ARRAY_NR_BASE_PTRS \

#define PS_ARRAY_SIZE (PS_APPARENT_ARRAY_SIZE) - 1


/**
 * workspace for kernel-lsof probe data
 * line numbers from kernel version 4.1.3
 * struct file in include/linux/fs.h:828
 * struct path in include/linux/path.h:7
 * struct file_operations in include/linux/fs.h:1573
 * count, flags, mode
 * struct fown_struct f_owner in include/linux/fs.h:797
 * struct cred f_cred in include/linux/cred.h
 * typedef unsigned __bitwise__ fmode_t in include/linux/types.h
 *
 **/
struct kernel_lsof_data {
	struct file f;
	struct path p;
	struct fown_struct owner;
	atomic_long_t count;
	unsigned int flags;
	fmode_t mode;
};


/**
   @brief struct probe is a child of a struct kernel_sensor and provides
   a specific kernel instrumentation

   struct probe it may have peers, which are organized as nodes on
   the kernel_sensor's probe list. struct probe t is initialized and
   destroyed by its parent kernel_sensor.

   members:
   probe_lock is available a mutex object if needed
   probe_id is meant to uniqueily identify the probe,
   it should point to allocated memory passed to the
   probe in the init call.
   struct probe *(*init)(struct probe *probe, uint_t *id) points to
   an initializing function that will prepare the probe to run.
   It takes a pointer to probe memory that has already been allocated,
   and a pointer to allocated memory that identifies the probe. The
   id is copied into a newly allocated memory buffer.
   void *(*destroy)(struct probe *probe) points to a destructor function that stops
   kernel threads and tears down probe resources, but does not free
   probe memory.
   int (*send_msg_to_probe)(struct probe *probe, int length, void *buf)
   causes the probe to copy length bytes of memory from buf. If
   successful, it returns the number of bytes copied, or a negative
   error code.
   int (*rcv_msg_from_probe)(struct probe *, void **ptr) causes the probe to
   allocate buffer and assign it to *ptr. It returns the length of
   the buffer at *ptr, or a negative number if an error occured.
   if the probe does not have any messages to copy to the caller, it
   will return -EAGAIN.
   struct kthread_worker probe_worker, and
   struct kthread_work probe_work are both used to schedule the probe as a
   kernel thread.
   struct llist_node l_node is the lockless linked list node pointer. It
   is used by the parent sensor to manage the probes.
   uint8 *data is a generic pointer whose use will be to store probe data
   structures such as flexible arrays.

**/
struct probe {
	/** always keep the lock as the first member of struct probe,
        in order to take the address of the struct probe when it
        is an anonymous struct element of a parent.
	**/
	spinlock_t lock;
	uint8_t *id;
	struct probe *(*init)(struct probe *, uint8_t *);
	void *(*destroy)(struct probe *);
	int (*send_msg)(struct probe *, int, void *);
	int (*rcv_msg)(struct probe *, int, void **);
	int (*start_stop)(struct probe *, uint64_t flags);
	uint64_t flags, timeout, repeat; /* expect that flags will contain level bits */
	struct kthread_worker worker;
	struct kthread_work work;
	struct llist_node l_node;
	uint8_t *data;
};

struct kernel_ps_probe {
	struct probe;
	struct flex_array *kps_data_flex_array;
	int (*print)(struct kernel_ps_probe *, uint8_t *, uint64_t, int);
	int (*ps)(struct kernel_ps_probe *, int, uint64_t);
	struct kernel_ps_probe *(*_init)(struct kernel_ps_probe *,
									 uint8_t *,
		                             int (*print)(struct kernel_ps_probe *,
												  uint8_t *, uint64_t, int));
	void *(*_destroy)(struct probe *);
};



/* probes are run by kernel worker threads (struct kthread_worker)
 * and they are structured as kthread "works" (struct kthread_work)
 */

#define CONT_CPU_ANY -1
struct kthread_worker *
kthread_create_worker(unsigned int flags, const char namefmt[], ...);

void kthread_destroy_worker(struct kthread_worker *worker);

struct probe *init_probe(struct probe *probe, uint8_t *id);
void *destroy_probe_work(struct kthread_work *work);
void *destroy_k_probe(struct probe *probe);



/**
 *
 * "more stable" api from here onward
 */
uint8_t *register_probe(uint64_t flags,
				   int (*probe)(uint64_t, uint8_t *),
				   int delay, int timeout, int repeat);
int unregister_probe(uint8_t *probe_id);

/* allocates and returns a buffer with probe data */
uint8_t *list_probes(int level, uint8_t *filter);

int set_probe_level(int level, uint8_t *filter);

/* retrieve the probe to the client or set to the server, */
/* the probe data structure */
int retrieve_probe(uint8_t *probe_id,
				   uint8_t *results_buf,
				   int buf_size);

uint64_t update_probe(uint8_t *probe_id,
					  uint8_t *update,
					  int update_size);


/**
 * @brief The kernel sensor is the parent of one or more probes
 *
 * The Kernel Sensor  is similar to its child probes in the sense
 * it uses kernel threads (kthreads) to run tasks. However, rather than
 * probe kernel instrumentation, it runs tasks to manage and service
 * its child probes. For example, presenting a socket interface
 * to user space.
 *
 * members:
 *
 * lock is a resource available to manage exclusive
 *      access to the kernel_sensor
 * flags is a bit mask that will hold information for the
 *       kernel sensor
 * id is meant to uniqueily identify the sensor,
 *     it should point to allocated memory passed to the
 *     kernel sensor in the init call.
 * struct kernel_sensor *(*init)(struct kernel_sensor *sensor, uint8_t *id)
 *     initializes memory that has already been allocated to have working
 *     semsor resources.
 * void *(*destroy)(struct kernel_sensor *sensor) will tear down resources,
 *      destroy child probes, de-schedule kernel threads, and zero-out sensor
 *      memory. It does not free sensor memory.
 * kwork and kworker are kernel structures to run sensor tasks.
 * l_head is the head of the lockless probe list. Each child probe is
 *      linked to this list
 * s is a unix domain socket used to communicate with user space.
**/

struct kernel_sensor {
	struct probe;
	struct kernel_sensor (*_init)(struct kernel_sensor *, uint8_t *);
	void *(*_destroy)(struct kernel_sensor *);
	struct llist_head l_head;
	struct llist_head probes;
	struct unix_sock s;
};

uint8_t *register_sensor(struct kernel_sensor *s);
int unregister_sensor(uint8_t *sensor_id);
uint8_t *list_sensors(uint8_t *filter);
#define DMSG() printk(KERN_INFO "DEBUG: kernel-ps Passed %s %d \n",__FUNCTION__,__LINE__);

#endif // CONTROLLER_H
