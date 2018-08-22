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
#include "jsmn/jsmn.h"
#include "uname.h"
#include "import-header.h"
#include "controller-flags.h"
#define _MODULE_LICENSE "GPL v2"
#define _MODULE_AUTHOR "Michael D. Day II <mike.day@twosixlabs.com>"
#define _MODULE_INFO "In-Virtue Kernel Controller"

#define PROTOCOL_VERSION "\'Virtue-protocol-version\': 0.1"
#define SESSION_RESPONSE "{\'Virtue-protocol-version\': 0.1}\n\0"

extern int print_to_log;
extern atomic64_t SHOULD_SHUTDOWN;
extern long max_size;
extern long chunk_size;


enum json_array_chars {
	L_BRACKET = 0x5b, SPACE = 0x20, D_QUOTE = 0x22, COMMA = 0x2c, R_BRACKET = 0x5d,
	S_QUOTE = 0x27
};

#define NUM_COMMANDS 12
enum message_command {CONNECT = 0, DISCOVERY, OFF, ON, INCREASE, DECREASE,
					  LOW, DEFAULT, HIGH, ADVERSARIAL, RESET,
					  RECORDS};
typedef enum message_command command;

extern uint8_t *cmd_strings[]; /* in controller-common.c */

/* max message header size */
#define CONNECTION_MAX_HEADER 0x400
#define CONNECTION_MAX_REQUEST CONNECTION_MAX_HEADER
#define CONNECTION_MAX_BUFFER 0x800
#define CONNECTION_MAX_MESSAGE 0x1000
#define CONNECTION_MAX_REPLY CONNECTION_MAX_MESSAGE
#define MAX_LINE_LEN CONNECTION_MAX_MESSAGE
#define MAX_NONCE_SIZE UUID_STRING_LEN + 4
#define MAX_CMD_SIZE 128
#define MAX_ID_SIZE MAX_CMD_SIZE
#define MAX_NAME_SIZE MAX_CMD_SIZE
#define MAX_TOKENS 64
#define SENSOR_UUID_SIZE 16
struct jsmn_message;
struct jsmn_session;
void dump_session(struct jsmn_session *s);
int dump(const char *js, jsmntok_t *t, size_t count, int indent);

/**
 * a jsmn_message is a single jsonl object, read and created by a connection
 * the jsonl control protocol expects messages to be a request and matching
 * replies (one or more).
 *
 * Each new request message automatically creates a new jsmn_session (below).
 * a successful session is one request message and at least one reply
 * message.
 *
 * A jsmn_session provides glue that binds a request to matching responses.
 *
 * The struct connection (defined in controller.h) initializes each jsmn_message
 * with the struct socket that produced the data (read or written).
 *
 * You may always relate a struct connection to a jsmn_message by their having
 * the same struct socket.
 *
 **/

struct jsmn_message
{
	struct list_head e_messages;
	spinlock_t sl;
	jsmn_parser parser;
	jsmntok_t tokens[MAX_TOKENS];
	uint8_t *line;
	size_t len;
	int type;
	int count; /* token count */
	struct jsmn_session *s;
	struct socket *socket;
};

/**
 * a session may have one request and multiple replies
 **/
struct jsmn_session
{
	struct list_head session_entry;
	spinlock_t sl;
	uint8_t nonce[MAX_NONCE_SIZE];
	struct jsmn_message *req;
	struct list_head h_replies;
	uint8_t cmd[MAX_CMD_SIZE];
	/**
	 * reminder: converting probe_id to probe_name be be aligned with
	 * user-space sensor wrapper
	 **/
	uint8_t sensor_name[MAX_NAME_SIZE];
	uint8_t sensor_uuid[SENSOR_UUID_SIZE];
};


void free_message(struct jsmn_message *m);
struct jsmn_message * new_message(uint8_t *line, size_t len);
void free_session(struct jsmn_session *s);
int parse_json_message(struct jsmn_message *m);
void init_jsonl_parser(void);


struct sensor;

struct sensor_msg
{
	int id;
	int ccode;
	void *input;
	ssize_t input_len;
	void *output;
	ssize_t output_len;
};

struct records_request
{
	struct jsmn_message *json_msg;
	bool run_probe;
	bool clear;
	uint64_t nonce;
	int index;
	int range;
};

struct records_reply
{
	int index;
	int range;
	uint8_t *records;
	ssize_t records_len;
};

/**
 * state request and reply will handle all messages except for
 * Discovery and Records
 **/
struct state_request
{
	command  cmd; /* enum message_command */
};

struct state_reply
{
	command cmd; /* enum message_command */
	int state;
};

int
process_state_message(struct sensor *sensor, struct sensor_msg *msg);

static inline void sleep(unsigned sec)
{
	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(sec * HZ);
	}
}

ssize_t
kfs_write_to_offset(char *filename, void *data, ssize_t len, loff_t *pos);

ssize_t
kfs_read_from_offset(char *filename, void *data, ssize_t len, loff_t *pos);

/** these take a struct file **/
ssize_t
kfs_read(struct file *file, void *buf, size_t count, loff_t *pos);

ssize_t
kfs_write(struct file *file, void *buf, size_t count, loff_t *pos);

ssize_t k_socket_read(struct socket *sock,
					  size_t size,
					  void *in,
					  unsigned int flags);


ssize_t k_socket_write(struct socket *sock,
					   size_t size,
					   void *out,
					   unsigned int flags);

/* - - sensor - -
 *
 * A sensor is a function that runs as a kernel
 * task at regular intervals to obtain information or to check the
 * truth of various assertions.
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
   @brief struct sensor is a generic struct that is designed to be
   incorporated into a more specific type of sensor.

   struct sensor is initialized and destroyed by its incorporating
   data structure. It has function pointers for init and destroy
   methods.

   members:
   anonymous union:
   - lock is a spinlock, available as a mutex object if needed
   - s_lock is a semaphore.

   - name is the non-unique identifier for the sensor type
   - uuid is a 128-bit binary uuid typedef
   - uuid_string is a 36 + 1 byte string that holds a parsed uuid string

   - struct sensor *(*init)(struct sensor * sensor,
   uint8_t *name, int name_len)
   points to an initializing function that will prepare the sensor to run.
   It takes a pointer to sensor memory that has already been allocated,
   and a pointer to allocated memory containing the name of the sensor.
   name is copied into a newly allocated memory buffer.

   - void *(*destroy)(struct probe *probe) points to a destructor function
   that stops kernel threads and tears down probe resources, frees id and
   data memory but does not free probe memory.

   - int (*message)(struct sensor *, struct sensor_msg *)
   invokes a function in the sensor that responds to the message

   - flags, state, timeout, and repeat control the operation of the sensor.

   - struct kthread_worker worker, and struct kthread_work work
   are both used to schedule the sensor as a kernel thread.

   - struct list_node l_node is the linked list node pointer. It
   is used by the parent sensor to manage the sensor as a child
**/

struct sensor {
	union {
		spinlock_t lock;
		struct semaphore s_lock;
	};
	uint8_t *name;
/**
 * see <linux/uuid.h> and <uapi/linux/uuid.h>
 * UUID_SIZE 16
 * UUID_STRING_LEN 36
 * https://en.wikipedia.org/wiki/Universally_unique_identifier
 **/
	uuid_t uuid;
	uint8_t uuid_string[UUID_STRING_LEN + 1];
	struct sensor *(*init)(struct sensor *, uint8_t *, int);
	void *(*destroy)(struct sensor *);
	int (*message)(struct sensor *, struct sensor_msg *);
	int (*state_change)(struct sensor *, struct sensor_msg *);
	uint64_t flags, state;  /* see controller-flags.h */
	int timeout, repeat;
	struct kthread_worker worker;
	struct kthread_work work;
	struct list_head l_node;
};

int
get_sensor(uint8_t *key, struct sensor **sensor);

int
get_sensor_name(uint8_t *probe_id, struct sensor **sensor);

int
get_sensor_uuid(uint8_t *uuid, struct sensor **p);

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


/* connection struct is used for both listening and connected sockets */
/* function pointers for listen, accept, close */
struct connection {
	struct sensor;
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
	struct sensor;
	struct kernel_sensor *(*_init)(struct kernel_sensor *);
	void *(*_destroy)(struct kernel_sensor *);
	struct list_head sensors;
	struct list_head listeners;
	struct list_head connections;
};


/*******************************************************************************
 * ps probe
 *
 ******************************************************************************/

extern int ps_repeat;
extern int ps_timeout;
extern int ps_level;

struct kernel_ps_data {
	uint8_t clear;
	uint8_t pad[7];
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
struct kernel_ps_sensor {
	struct sensor;
	struct flex_array *kps_data_flex_array;
	int (*print)(struct kernel_ps_sensor *, uint8_t *, uint64_t, int);
	int (*ps)(struct kernel_ps_sensor *, int, uint64_t);
	struct kernel_ps_sensor *(*_init)(struct kernel_ps_sensor *,
									  uint8_t *, int,
									  int (*print)(struct kernel_ps_sensor *,
												   uint8_t *, uint64_t, int));
	void *(*_destroy)(struct sensor *);
};

int
kernel_ps_get_record(struct kernel_ps_sensor *parent,
					 struct sensor_msg *msg,
					 uint8_t *tag);

int
kernel_ps_unlocked(struct kernel_ps_sensor *parent, uint64_t nonce);
int
kernel_ps(struct kernel_ps_sensor *parent, int count, uint64_t nonce);
struct kernel_ps_sensor *
init_kernel_ps_sensor(struct kernel_ps_sensor *ps_p,
					  uint8_t *id, int id_len,
					  int (*print)(struct kernel_ps_sensor *,
								   uint8_t *, uint64_t, int));

int
print_kernel_ps(struct kernel_ps_sensor *parent,
				uint8_t *tag,
				uint64_t nonce,
				int count);

/* probes are run by kernel worker threads (struct kthread_worker)
 * and they are structured as kthread "works" (struct kthread_work)
 */

#define CONT_CPU_ANY -1
struct kthread_worker *
controller_create_worker(unsigned int flags, const char namefmt[], ...);

void controller_destroy_worker(struct kthread_worker *worker);

struct sensor *init_sensor(struct sensor *sensor,
						   uint8_t *name,  int name_size);
void *destroy_sensor_work(struct kthread_work *work);
void *destroy_k_sensor(struct sensor *sensor);

bool init_and_queue_work(struct kthread_work *work,
						 struct kthread_worker *worker,
						 void (*function)(struct kthread_work *));


void *destroy_sensor(struct sensor *sensor);

/**
******************************************************************************
* lsof sensor
******************************************************************************
**/



/**
 * workspace for kernel-lsof sensor data
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
	kuid_t uid;
	pid_t pid;
};

struct lsof_pid_el
{
	kuid_t uid;
	pid_t pid;
	uint64_t nonce;
	struct pid *p;
	struct task_struct *t;
};

typedef struct lsof_pid_el pid_el;
/**
 * following two were generalized and hoisted out of lsof-probe.c
 * controller-common.c
 *
 **/

struct task_struct *
get_task_by_pid_number(pid_t pid);

/**
 * see include/linux/flex_array.h for the definitions of
 * FLEX_ARRAY_NR_BASE_PTRS and FLEX_ARRA_ELEMENTS_PER_PART
 * this is a conservatice calculation to ensure we don't try to
 * pre allocate a flex_array with too many elements
 **/

#define PID_EL_SIZE sizeof(pid_el)
#define PID_APPARENT_ARRAY_SIZE											\
	(FLEX_ARRAY_ELEMENTS_PER_PART(PID_EL_SIZE) * (FLEX_ARRAY_NR_BASE_PTRS))
#define PID_EL_ARRAY_SIZE ((PID_APPARENT_ARRAY_SIZE) - 1)

#define MAX_DENTRY_LEN 0x100
struct kernel_lsof_data {
	uint8_t clear;
	uint8_t pad[7];
	uint64_t nonce;
	int index;
	kuid_t user_id;
	pid_t pid_nr;  /* see struct pid.upid.nrin linux/pid.h  */
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


struct kernel_lsof_sensor {
	struct sensor;
	struct flex_array *klsof_pid_flex_array;
	struct flex_array *klsof_data_flex_array;
	int (*filter)(struct kernel_lsof_sensor *,
				  struct kernel_lsof_data *,
				  void *);
	int (*print)(struct kernel_lsof_sensor *, uint8_t *, uint64_t);
	int (*lsof)(struct kernel_lsof_sensor *, uint64_t);
	struct kernel_lsof_sensor *(*_init)(struct kernel_lsof_sensor *,
										uint8_t *, int,
										int (*print)(struct kernel_lsof_sensor *,
													 uint8_t *, uint64_t),
										int (*filter)(struct kernel_lsof_sensor *,
													  struct kernel_lsof_data *,
													  void *));
	void *(*_destroy)(struct sensor *);
};

extern struct kernel_lsof_sensor klsof_sensor;
extern int lsof_repeat;
extern int lsof_timeout;
extern int lsof_level;

int
build_pid_index_unlocked(struct sensor *p,
						 struct flex_array *a,
						 uint64_t nonce);
int
build_pid_index(struct sensor *p, struct flex_array *a, uint64_t nonce);

int
kernel_lsof_get_record(struct kernel_lsof_sensor *parent,
					   struct sensor_msg *msg,
					   uint8_t *tag);

int
lsof_for_each_pid_unlocked(struct kernel_lsof_sensor *p,
						   uint64_t nonce);

int lsof_pid_filter(struct kernel_lsof_sensor *p,
					struct kernel_lsof_data *d,
					void *cmp);


int
lsof_all_files(struct kernel_lsof_sensor *p,
			   struct kernel_lsof_data *d,
			   void *cmp);


int
lsof_uid_filter(struct kernel_lsof_sensor *p,
				struct kernel_lsof_data *d,
				void *cmp);

int
print_kernel_lsof(struct kernel_lsof_sensor *parent,
				  uint8_t *tag,
				  uint64_t nonce);

int
kernel_lsof_unlocked(struct kernel_lsof_sensor *p,
					 uint64_t nonce);

int
kernel_lsof(struct kernel_lsof_sensor *parent, uint64_t nonce);

void
run_klsof_probe(struct kthread_work *work);

struct kernel_lsof_sensor *
init_kernel_lsof_sensor(struct kernel_lsof_sensor *lsof_p,
						uint8_t *id, int id_len,
						int (*print)(struct kernel_lsof_sensor *,
									 uint8_t *, uint64_t),
						int (*filter)(struct kernel_lsof_sensor *,
									  struct kernel_lsof_data *,
									  void *));


void *
destroy_kernel_lsof_sensor(struct sensor *sensor);

/**
****************************************************************************
* sysfs sensor
****************************************************************************
**/

extern int sysfs_repeat;
extern int sysfs_timeout;
extern int sysfs_level;

struct kernel_sysfs_data {
	uint8_t clear;
	uint8_t regular_file;
	uint8_t pad[6];
	uint64_t nonce;
	int index;
	pid_t pid;
	struct kstat stat;
	void *data;
	size_t data_len;
	size_t ccode;
	uint8_t dpath[MAX_DENTRY_LEN + 1];
};

#define SYSFS_DATA_SIZE sizeof(struct kernel_sysfs_data)

/**
 * see include/linux/flex_array.h for the definitions of
 * FLEX_ARRAY_NR_BASE_PTRS and FLEX_ARRA_ELEMENTS_PER_PART
 * this is a conservatice calculation to ensure we don't try to
 * pre allocate a flex_array with too many elements
 **/

#define SYSFS_APPARENT_ARRAY_SIZE										\
	(FLEX_ARRAY_ELEMENTS_PER_PART(SYSFS_DATA_SIZE) * FLEX_ARRAY_NR_BASE_PTRS)

#define SYSFS_ARRAY_SIZE ((SYSFS_APPARENT_ARRAY_SIZE) - 1)

struct kernel_sysfs_sensor {
	struct sensor;
	struct flex_array *ksysfs_flex_array;
	struct flex_array *ksysfs_pid_flex_array;
	int (*print)(struct kernel_sysfs_sensor *, uint8_t *, uint64_t, int);
	int (*filter)(struct kernel_sysfs_sensor *,
				  struct kernel_sysfs_data *,
				  void *);
	int (*ksysfs)(struct kernel_sysfs_sensor *, int, uint64_t);
	int (*kernel_lsof)(struct kernel_lsof_sensor *parent, int count, uint64_t nonce);
	struct kernel_sysfs_sensor *(*_init)(struct kernel_sysfs_sensor *,
										 uint8_t *, int,
										 int (*print)(struct kernel_sysfs_sensor *,
													  uint8_t *, uint64_t, int),
										 int (*filter)(struct kernel_sysfs_sensor *,
													   struct kernel_sysfs_data *,
													   void *));
	void *(*_destroy)(struct sensor *);
};

extern struct kernel_sysfs_sensor ksysfs_sensor;

int
file_getattr(struct file *f, struct kstat *k);

int
is_regular_file(struct file *f);

ssize_t
kernel_read_file_with_name(char *name, void **buf, size_t max_count, loff_t *pos);

ssize_t
vfs_read_file(char *name, void **buf, size_t max_count, loff_t *pos);

ssize_t
write_file_struct(struct file *f, void *buf, size_t count, loff_t *pos);

ssize_t
read_file_struct(struct file *f, void *buf, size_t count, loff_t *pos);

ssize_t
write_file(char *name, void *buf, size_t count, loff_t *pos);

ssize_t
read_file(char *name, void *buf, size_t count, loff_t *pos);

int
print_sysfs_data(struct kernel_sysfs_sensor *, uint8_t *, uint64_t, int);

int
filter_sysfs_data(struct kernel_sysfs_sensor *,
				  struct kernel_sysfs_data *,
				  void *);
int
kernel_sysfs_unlocked(struct kernel_sysfs_sensor *, uint64_t);

int
kernel_sysfs(struct kernel_sysfs_sensor *, int, uint64_t);

struct kernel_sysfs_sensor *
init_sysfs_sensor(struct kernel_sysfs_sensor *,
				  uint8_t *, int,
				  int (*print)(struct kernel_sysfs_sensor *,
							   uint8_t *, uint64_t, int),
				  int (*filter)(struct kernel_sysfs_sensor *,
								struct kernel_sysfs_data *,
								void *));

void *
destroy_sysfs_sensor(struct sensor *sensor);

/**
 * ****************************************
 * json parser and socket interface
 ******************************************
 **/

int
__init socket_interface_init(void);

void
__exit socket_interface_exit(void);

int
build_discovery_buffer(uint8_t **buf, size_t *len);

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
#define DMSG()															\
	printk(KERN_INFO "DEBUG: kernel-sensor Passed %s %d \n",__FUNCTION__,__LINE__);

#endif // CONTROLLER_H
