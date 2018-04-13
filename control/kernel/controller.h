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
#include "controller-linux.h"
//#include "uname.h"
#include "controller-flags.h"
#define _MODULE_LICENSE "GPL v2"
#define _MODULE_AUTHOR "Michael D. Day II <mike.day@twosixlabs.com>"
#define _MODULE_INFO "In-Virtue Kernel Controller"

static int SHOULD_SHUTDOWN;

enum json_array_chars {
	L_BRACKET = 0x5b, SPACE = 0x20, D_QUOTE = 0x22, COMMA = 0x2c, R_BRACKET = 0x5d
};

static inline void sleep(unsigned sec)
{
	if (! SHOULD_SHUTDOWN) {
	__set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(sec * HZ);
	}
}

/* the kernel itself has dynamic trace points, they
 *  need to be part of the probe capability.
 */

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



/**
   @brief struct probe is an a generic struct that is designed to be
   incorporated into a more specific type of probe.

   struct probe is initialized and destroyed by its incorporating
   data structure. It has function pointers for init and destroy
   methods.

   members:
   - probe_lock is available as a mutex object if needed

   - probe_id is meant to uniqueily identify the probe,
   it contains a copy of memory passed to the
   probe in the init call.

   - struct probe *(*init)(struct probe *probe,
   uint8_t *id, int id_size,
   uint8_t *data, int data_size) points to
   an initializing function that will prepare the probe to run.
   It takes a pointer to probe memory that has already been allocated,
   and a pointer to allocated memory that identifies the probe. The
   id and data are copied into a newly allocated memory buffer.

   - void *(*destroy)(struct probe *probe) points to a destructor function
   that stops kernel threads and tears down probe resources, frees id and
   data memory but does not free probe memory.

   - int (*send_msg_to_probe)(struct probe *probe, int length, void *buf)
   causes the probe to copy length bytes of memory from buf. If
   successful, it returns the number of bytes copied, or a negative
   error code.

   - int (*rcv_msg_from_probe)(struct probe *, void **ptr) causes the probe to
   allocate buffer and assign it to *ptr. It returns the length of
   the buffer at *ptr, or a negative number if an error occured.
   if the probe does not have any messages to copy to the caller, it
   will return -EAGAIN.

   - struct kthread_worker probe_worker, and struct kthread_work probe_work
   are both used to schedule the probe as a kernel thread.

   - struct list_node l_node is the linked list node pointer. It
   is used by the parent sensor to manage the probe as a peer of more than
   one siblings.

   - uint8 *data is a generic pointer whose use may be to store probe data
   structures.

**/
struct probe {
	spinlock_t lock;
	uint8_t *id;
	struct probe *(*init)(struct probe *, uint8_t *, int);
	void *(*destroy)(struct probe *);
	int (*send_msg)(struct probe *, int, void *);
	int (*rcv_msg)(struct probe *, int, void **);
	int (*start_stop)(struct probe *, uint64_t flags);
	uint64_t flags;  /* expect that flags will contain level bits */
	int timeout, repeat;
	struct kthread_worker worker;
	struct kthread_work work;
	struct list_head l_node;
};


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



/**
 * struct socket: include/linux/net.h: 110
 **/

/* max message header size */
#define CONNECTION_MAX_HEADER 0x400
#define CONNECTION_MAX_MESSAGE 0x1000
/* connection struct is used for both listening and connected sockets */
/* function pointers for listen, accept, close */
struct connection {
	struct probe;
	/**
	 * _init parameters:
	 * uint64_t flags - will have the PROBE_LISTENER or PROBE_CONNECTED bit set
	 * void * data depends on the value of flags:
	 *    if __FLAG_IS_SET(flags, PROBE_LISTENER), then data points to a
	 *    string in the form of "/var/run/socket-name".
	 *    if __FLAG_IS_SET(flags, PROBE_CONNECTED, then data points to a
	 *    struct socket
	 **/
	struct connection *(*_init)(struct connection *, uint64_t, void *);
	void *(*_destroy)(struct connection *);
	struct socket *connected;
	uint8_t path[UNIX_PATH_MAX];
};

struct kernel_sensor {
	struct probe;
	struct kernel_sensor *(*_init)(struct kernel_sensor *);
	void *(*_destroy)(struct kernel_sensor *);
	struct list_head probes;
	struct list_head listeners;
	struct list_head connections;
};


/*******************************************************************************
 * ps probe
 *
 ******************************************************************************/
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
	(FLEX_ARRAY_ELEMENTS_PER_PART(PS_DATA_SIZE) * FLEX_ARRAY_NR_BASE_PTRS)

#define PS_ARRAY_SIZE ((PS_APPARENT_ARRAY_SIZE) - 1)

/**
 * @brief a probe that produces a list of kernel processes similar to the
 *        ps command.
 *
 *  members:
 *
 *  - probe is an anonymous struct probe (see above)
 *
 *  - flex_array is a kernel flex_array, which will be pre-allocated
 *    to hold data from the kernel's process list
 *
 *  - print is a function pointer - it may be initialized with custom
 *    output functions. The default is to printk kernel process data
 *    to /var/log/messages using printk
 *
 *  - ps is a function pointer - it may be initialized with a custom
 *    process probe function. The default is to collect process information
 *    from the kernel's scheduler.
 *
 *  - _init is a function pointer, it points to a function to initialize
 *    the ps probe. It calls a struct probe->init function to initialize
 *    the probe anonymous structure.
 *
 *  - _destroy is a function pointer, it points to a function for destroying
 *    the ps probe. It calls probe->destroy to tear down the anonymous
 *    struct probe.
 **/
struct kernel_ps_probe {
	struct probe;
	struct flex_array *kps_data_flex_array;
	int (*print)(struct kernel_ps_probe *, uint8_t *, uint64_t, int);
	int (*ps)(struct kernel_ps_probe *, int, uint64_t);
	struct kernel_ps_probe *(*_init)(struct kernel_ps_probe *,
									 uint8_t *, int,
		                             int (*print)(struct kernel_ps_probe *,
												  uint8_t *, uint64_t, int));
	void *(*_destroy)(struct probe *);
};


/* probes are run by kernel worker threads (struct kthread_worker)
 * and they are structured as kthread "works" (struct kthread_work)
 */

#define CONT_CPU_ANY -1
struct kthread_worker *
controller_create_worker(unsigned int flags, const char namefmt[], ...);

void controller_destroy_worker(struct kthread_worker *worker);

struct probe *init_probe(struct probe *probe,
						 uint8_t *id,  int id_size);
void *destroy_probe_work(struct kthread_work *work);
void *destroy_k_probe(struct probe *probe);

bool init_and_queue_work(struct kthread_work *work,
					struct kthread_worker *worker,
					void (*function)(struct kthread_work *));


void *destroy_probe(struct probe *probe);

/********************************************************************************
 * lsof probe
 *******************************************************************************/



/**
 * workspace for kernel-lsof probe data
 * line numbers from kernel version 4.16
 * struct file in include/linux/fs.h:857
 * struct path in include/linux/path.h:8
 * struct file_operations in include/linux/fs.h:1702
 * struct inode_operations in include/linux/fs.h:1743
 * count, flags, mode
 * struct fown_struct f_own_struct in include/linux/fs.h:826
 * struct cred f_cred in include/linux/cred.h
 * typedef unsigned __bitwise__ fmode_t in include/linux/types.h
 *
 **/

/**
 * Filesystem information:
 *	struct fs_struct		*fs;
 *
 * Open file information:
 *	struct files_struct		*files;
 *
 * Namespaces:
 * struct nsproxy			*nsproxy;
 *
 * task.files->fd_array[].path
 **/


struct lsof_filter 
{
	kuid_t user_id;
	pid_t lastpid;
};
	

#define MAX_DENTRY_LEN 0x400
struct kernel_lsof_data {
	uint64_t index, nonce;
	kuid_t user_id;
	pid_t pid_nr;  /* see struct pid.upid.nrin linux/pid.h  */
	struct files_struct *files;
	struct fdtable *files_table;
	struct file *file;
	unsigned int *fds;
	struct path files_path;
	struct fown_struct owner;
	atomic_long_t count;
	unsigned int flags;
	fmode_t mode;
	uint8_t *dp;
	int dp_offset;
	uint8_t dpath[MAX_DENTRY_LEN];
};


#define LSOF_DATA_SIZE sizeof(struct kernel_lsof_data)

/**
 * see include/linux/flex_array.h for the definitions of
 * FLEX_ARRAY_NR_BASE_PTRS and FLEX_ARRA_ELEMENTS_PER_PART
 * this is a conservatice calculation to ensure we don't try to
 * pre allocate a flex_array with too many elements
 **/

#define LSOF_APPARENT_ARRAY_SIZE										\
	(FLEX_ARRAY_ELEMENTS_PER_PART(LSOF_DATA_SIZE) * FLEX_ARRAY_NR_BASE_PTRS)

#define LSOF_ARRAY_SIZE ((LSOF_APPARENT_ARRAY_SIZE) - 1)


struct kernel_lsof_probe {
	struct probe;
	struct flex_array *klsof_data_flex_array;
	int (*filter)(struct kernel_lsof_probe *,
				  struct kernel_lsof_data *,
				  void *);
	int (*print)(struct kernel_lsof_probe *, uint8_t *, uint64_t, int);
	int (*lsof)(struct kernel_lsof_probe *, int, uint64_t);
	struct kernel_lsof_probe *(*_init)(struct kernel_lsof_probe *,
									   uint8_t *, int,
									   int (*print)(struct kernel_lsof_probe *,
													uint8_t *, uint64_t, int),
									   int (*filter)(struct kernel_lsof_probe *,
													 struct kernel_lsof_data *,
													 void *));
	void *(*_destroy)(struct probe *);
};

extern struct kernel_lsof_probe klsof_probe;
extern unsigned int lsof_repeat;
extern unsigned int lsof_timeout;
extern unsigned int lsof_level;

int
lsof_all_files(struct kernel_lsof_probe *p,
				   struct kernel_lsof_data *d,
			   void *cmp);


int
lsof_uid_filter(struct kernel_lsof_probe *p,
				struct kernel_lsof_data *d,
				void *cmp);

int
print_kernel_lsof(struct kernel_lsof_probe *parent,
				  uint8_t *tag,
				  uint64_t nonce,
				  int count);

int
kernel_lsof(struct kernel_lsof_probe *parent, int count, uint64_t nonce);

void
run_klsof_probe(struct kthread_work *work);

struct kernel_lsof_probe *
init_kernel_lsof_probe(struct kernel_lsof_probe *lsof_p,
					   uint8_t *id, int id_len,
					   int (*print)(struct kernel_lsof_probe *,
									uint8_t *, uint64_t, int),
					   int (*filter)(struct kernel_lsof_probe *,
									 struct kernel_lsof_data *,
									 void *));


void *
destroy_kernel_lsof_probe(struct probe *probe);


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

uint8_t *register_sensor(struct kernel_sensor *s);
int unregister_sensor(uint8_t *sensor_id);
uint8_t *list_sensors(uint8_t *filter);
#define DMSG() \
	printk(KERN_INFO "DEBUG: kernel-sensor Passed %s %d \n",__FUNCTION__,__LINE__);

#endif // CONTROLLER_H
