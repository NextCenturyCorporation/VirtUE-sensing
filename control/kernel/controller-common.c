/*******************************************************************
 * in-Virtue Kernel Controller
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

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *******************************************************************/

#include "controller-linux.h"
#include "controller.h"
#include "jsmn/jsmn.h"
#include "uname.h"

struct kernel_sensor k_sensor;
EXPORT_SYMBOL(k_sensor);

atomic64_t SHOULD_SHUTDOWN = ATOMIC64_INIT(0);
EXPORT_SYMBOL(SHOULD_SHUTDOWN);


struct kernel_ps_probe kps_probe;
struct kernel_lsof_probe klsof_probe;
struct kernel_sysfs_probe ksysfs_probe;

int lsof_repeat = 1;
int lsof_timeout = 1;
int lsof_level = 1;

int ps_repeat = 1;
int ps_timeout = 1;
int ps_level = 1;

int sysfs_repeat = 1;
int sysfs_timeout = 1;
int sysfs_level = 1;

int print_to_log = 1;

char *socket_name = "/var/run/kernel_sensor";
char *lockfile_name = "/var/run/kernel_sensor.lock";


module_param(lsof_repeat, int, 0644);
module_param(lsof_timeout, int, 0644);
module_param(lsof_level, int, 0644);

MODULE_PARM_DESC(lsof_repeat, "How many times to run the kernel lsof function");
MODULE_PARM_DESC(lsof_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "lsof function");
MODULE_PARM_DESC(lsof_level, "How invasively to probe open files");

module_param(ps_repeat, int, 0644);
module_param(ps_timeout, int, 0644);
module_param(ps_level, int, 0644);

MODULE_PARM_DESC(ps_repeat, "How many times to run the kernel ps function");
MODULE_PARM_DESC(ps_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "ps function");
MODULE_PARM_DESC(ps_level, "How invasively to probe processes");


module_param(sysfs_repeat, int, 0644);
module_param(sysfs_timeout, int, 0644);
module_param(sysfs_level, int, 0644);

MODULE_PARM_DESC(sysfs_repeat, "How many times to run the kernel sysfs function");
MODULE_PARM_DESC(sysfs_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "sysfs function");
MODULE_PARM_DESC(sysfs_level, "How invasively to probe open files");

module_param(socket_name, charp, 0644);

module_param(print_to_log, int, 0644);
MODULE_PARM_DESC(print_to_log, "print probe output to messages");


/**
 * Note on /sys and /proc files:
 * on Linux 4.x they stat as having no blocks and zero size,
 * but they do have a blocksize of 0x400. So, by default, we
 * will allocate a buffer the size of one block
 **/

int
file_getattr(struct file *f, struct kstat *k)
{
	int ccode = 0;
	memset(k, 0x00, sizeof(struct kstat));
#ifdef MODERN_FILE_API
	ccode = vfs_getattr(&f->f_path, k, 0x00000fffU, KSTAT_QUERY_FLAGS);
#else
	ccode = vfs_getattr(&f->f_path, k);
#endif
	return ccode;
}


ssize_t
write_file_struct(struct file *f, void *buf, size_t count, loff_t *pos)
{
	ssize_t ccode;
#ifdef MODERN_FILE_API
	ccode = kernel_write(f, buf, count, pos);
#else
	ccode = kernel_write(f, (char *)buf, count, *pos);
#endif
	if (ccode < 0) {
		pr_err("Unable to write file: %p (%ld)", f, ccode);
		return ccode;
	}

	return ccode;
}
STACK_FRAME_NON_STANDARD(write_file_struct);

ssize_t
read_file_struct(struct file *f, void *buf, size_t count, loff_t *pos)
{
	ssize_t ccode;

#ifdef MODERN_FILE_API
	ccode = kernel_read(f, buf, count, pos);
#else
	ccode = kernel_read(f, *pos, (char *)buf, count);
#endif
	if (ccode < 0) {
		pr_err("Unable to read file: %p (%ld)", f, ccode);
		return ccode;
	}

	return ccode;
}
STACK_FRAME_NON_STANDARD(read_file_struct);

ssize_t
write_file(char *name, void *buf, size_t count, loff_t *pos)
{
	ssize_t ccode;
	struct file *f;
	f = filp_open(name, O_WRONLY, 0);
	if (f) {
#ifdef MODERN_FILE_API
		ccode = kernel_write(f, buf, count, pos);
#else
		ccode = kernel_write(f, (char *)buf, count, *pos);
#endif
		if (ccode < 0) {
			pr_err("Unable to write file: %s (%ld)", name, ccode);
			filp_close(f, 0);
			return ccode;
		}
	} else {
		ccode = -EBADF;
		pr_err("Unable to open file: %s (%ld)", name, ccode);
	}
	return ccode;
}
STACK_FRAME_NON_STANDARD(write_file);

ssize_t
read_file(char *name, void *buf, size_t count, loff_t *pos)
{
	ssize_t ccode;
	struct file *f;

	f = filp_open(name, O_RDONLY, 0);
	if (f) {
#ifdef MODERN_FILE_API
		ccode = kernel_read(f, buf, count, pos);
#else
		ccode = kernel_read(f, *pos, (char *)buf, count);
#endif
		if (ccode < 0) {
			pr_err("Unable to read file: %s (%ld)", name, ccode);
			filp_close(f, 0);
			return ccode;
		}
		filp_close(f, 0);
	} else {
		ccode = -EBADF;
  		pr_err("Unable to open file: %s (%ld)", name, ccode);
	}

	return ccode;
}
STACK_FRAME_NON_STANDARD(read_file);

struct task_struct *
get_task_by_pid_number(pid_t pid)
{
	struct pid *p = NULL;
	struct task_struct *ts = NULL;
	p = find_get_pid(pid);
	if (p) {
		ts = get_pid_task(p, PIDTYPE_PID);
	}

	return ts;
}


int
build_pid_index(struct probe *p, struct flex_array *a, uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	unsigned long flags;
	static pid_el pel;

	if (!spin_trylock_irqsave(&p->lock, flags)) {
		return -EAGAIN;
	}

	rcu_read_lock();
	for_each_process(task) {
        pel.pid = task->pid;
		pel.uid = task_uid(task);
		pel.nonce = nonce;

		if (index <  PID_EL_ARRAY_SIZE) {
			flex_array_put(a,
						   index,
						   &pel,
						   GFP_ATOMIC);
			index++;
		} else {
			printk(KERN_INFO "kernel pid index array over-run\n");
			index = -ENOMEM;
			goto unlock_out;
		}
	}
	rcu_read_unlock();

unlock_out:
	spin_unlock_irqrestore(&p->lock, flags);

	return index;

}
STACK_FRAME_NON_STANDARD(build_pid_index);

/**
 *  probe is LOCKED upon return
 **/
int
get_probe(uint8_t *probe_id, struct probe **p)
{
	struct probe *probe_p = NULL;
	int ccode = -ENFILE;
	assert(probe_id);
	assert(p && *p);

	*p = NULL;
	rcu_read_lock();
	list_for_each_entry_rcu(probe_p, &k_sensor.probes, l_node) {
		if (! strncmp(probe_p->id, probe_id, strlen(probe_id))) {
			if(!spin_trylock(&probe_p->lock)) {
				ccode =  -EAGAIN;
			} else {
				p = &probe_p;
				ccode = 0;
				goto exit;
			}
		}
	}
exit:
	rcu_read_unlock();
	return ccode;
}

/**
 * The discovery buffer needs to be a formatted as a JSON array,
 * with each probe's ID string as an element in the array.
 **/

/**
 * @brief build a JSON array containing the id strings of each parser
 * registered with the sensor.
 *
 * @param uint8_t **buf pointer-to-a-pointer that we will allocate for
 *        the caller, to hold a json array of probe ids.
 * @param size_t *len - pointer to an integer that will contain the
 *        size of the buffer we (re)allocate for the caller.
 * @return zero upon success, -ENOMEM in case of error.
 *
 *
 * This is ugly for two reasons. First, we don't pre-calculate the length
 * of the combined id strings. So we allocate a buffer that we _think_
 * will be large enough, and then realloc it to a smaller buffer at the end,
 * if we have extra memory (greater than 256.
 *
 * if the initial array is too small, we just give up and return -ENOMEM.
 * The right thing to do would be to krealloc to a larger buffer. That
 * would mean re-assigning the cursor. Not a lot of work, so it is something
 * to do if necessary.
 *
 * Secondly, building the JSON array is a manual process, so creating the
 * array one character at a time is needed. And the remaining length calculation
 * is erring on the the side of being conservative.
 *
 * TODO: maintain this buffer array before its needed, by re-constructing it
 * every time a probe registers or de-registers.
 *
 * TODO: krealloc the buffer to a larger size if the initial buffer is too
 * small.
 **/

int
build_discovery_buffer(uint8_t **buf, size_t *len)
{

	uint8_t *cursor;
	int remaining = 0, count = 0;
	struct probe *p_cursor = NULL;

	assert(buf && len);

	*len = CONNECTION_MAX_HEADER;
	*buf = kzalloc(CONNECTION_MAX_HEADER, GFP_KERNEL);
	if (*buf <= 0) {
		*buf = NULL;
		*len = 0;
		return -ENOMEM;
	}

	remaining = *len;
	cursor = *buf;
	*cursor++ = L_BRACKET; remaining--;
	*cursor++ = D_QUOTE; remaining--;

	rcu_read_lock();
	list_for_each_entry_rcu(p_cursor, &k_sensor.probes, l_node) {
		int id_len;
		if ((remaining - 5) > (id_len = strlen(p_cursor->id))) {
			if (count > 0) {
				*cursor++ = COMMA; remaining--;
				*cursor++ = SPACE; remaining--;
				*cursor++ = D_QUOTE; remaining--;
			}
			strncpy(cursor, p_cursor->id, remaining - 3);
			remaining -= id_len;
			cursor += id_len;
			*cursor++ = D_QUOTE; remaining--;
			count++;
		} else {
			goto err_exit;
		}
	}
	rcu_read_unlock();
	printk(KERN_DEBUG "closing discovery message, remaining %d\n", remaining);
	if (remaining > 2) {
		*cursor++ = R_BRACKET; remaining--;
		*cursor++ = 0x00; remaining--;
	}
	if (remaining > 0x100) {
		*buf = krealloc(*buf, (*len - (remaining - 2)), GFP_KERNEL);
		*len -= (remaining - 2);
	}
	printk(KERN_DEBUG "discovery buffer: %s\n", *buf);

	return 0;

err_exit:
	if (*buf) kfree(*buf);
	*buf = NULL;
	*len = -ENOMEM;
	return -ENOMEM;
}


/**
 * locking the kernel-ps flex_array
 * to write to or read from the pre-allocated flex array parts,
 * one must hold the kps_spinlock. The kernel_ps function will
 * execute a trylock and return with -EAGAIN if it is unable
 * to lock the array.
 *
 * A reader can decide best how to poll for the array spinlock, but
 * it should hold the lock while using pointers from the array.
 **/

/**
 * output the kernel-ps list to the kernel log
 *
 * @param uint8 *tag - tag added to each line, useful for filtering
 *        log output.
 * @param count The number of times kernel-ps has been run as a probe
 * @param nonce Identifies flex_array elements that should be printed,
 *              also terminates the printing loop
 * @return -EAGAIN if unable to obtain kernel-ps flex array lock,
 *         -ENOMEM if the ps flex_array is too small to contain every
 *                 running process
 *         The number of kernel-ps records printed if successful
 **/

static int print_kernel_ps(struct kernel_ps_probe *parent,
						   uint8_t *tag,
						   uint64_t nonce,
						   int count)
{
	int index;
	unsigned long flags;
	struct kernel_ps_data *kpsd_p;

	if(unlikely(!print_to_log)) {
		return 0;
	}

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}
	for (index = 0; index < PS_ARRAY_SIZE; index++)  {
		kpsd_p = flex_array_get(parent->kps_data_flex_array, index);
		if (kpsd_p->nonce != nonce) {
			break;
		}
		printk(KERN_INFO "%s %d:%d %s [%d] [%d] [%llx]\n",
			   tag, count, index, kpsd_p->comm, kpsd_p->pid_nr,
			   kpsd_p->user_id.val, nonce);
	}
	spin_unlock_irqrestore(&parent->lock, flags);
	if (index == PS_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
}
STACK_FRAME_NON_STANDARD(print_kernel_ps);

int kernel_ps(struct kernel_ps_probe *parent, int count, uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	struct kernel_ps_data kpsd;
	unsigned long flags;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}
	rcu_read_lock();
	for_each_process(task) {
		task_lock(task);

		kpsd.nonce = nonce;
		kpsd.index = index;
		kpsd.user_id = task_uid(task);
		kpsd.pid_nr = task->pid;
		memcpy(kpsd.comm, task->comm, TASK_COMM_LEN);
		if (index <  PS_ARRAY_SIZE) {
			flex_array_put(parent->kps_data_flex_array, index, &kpsd, GFP_ATOMIC);
		} else {
			index = -ENOMEM;
			task_unlock(task);
			goto unlock_out;
		}
		index++;
		task_unlock(task);
	}
unlock_out:
	rcu_read_unlock();
	spin_unlock_irqrestore(&parent->lock, flags);
	return index;
}


/* ugly but expedient way to support < 4.9 kernels */
/* todo: convert to macros and move to header */
/* CONT_INIT_AND_QUEUE, CONT_INIT_WORK, CONT_QUEUE_WORK */

bool
init_and_queue_work(struct kthread_work *work,
					struct kthread_worker *worker,
					void (*function)(struct kthread_work *))
{


	CONT_INIT_WORK(work, function);
	return CONT_QUEUE_WORK(worker, work);

}
EXPORT_SYMBOL(init_and_queue_work);

/**
 * Flushes this work off any work_list it is on.
 * If work is executing or scheduled, this routine
 * will block until the work is no longer being run.
 */
void *destroy_probe_work(struct kthread_work *work)
{
	CONT_FLUSH_WORK(work);
	memset(work, 0, sizeof(struct kthread_work));
	return work;
}
/**
 * destroy a struct probe, by tearing down
 * its allocated resources.
 * To free a dynamically allocated probe, you
 * could call it like this:
 *
 *   kfree(destroy_probe(probe));
 *
 *  The inside call frees memory allocated for the probe id
 *   and probe data, destroys a probe work node if it is there,
 *  and returns a zero'ed out memory buffer to the outer call.
 */
void *destroy_probe(struct probe *probe)
{
	assert(probe && __FLAG_IS_SET(probe->flags, PROBE_INITIALIZED));
	__CLEAR_FLAG(probe->flags, PROBE_INITIALIZED);

	if (__FLAG_IS_SET(probe->flags, PROBE_HAS_WORK)) {
		controller_destroy_worker(&probe->worker);
		__CLEAR_FLAG(probe->flags, PROBE_HAS_WORK);
	}
	if (probe->id &&
		__FLAG_IS_SET(probe->flags, PROBE_HAS_ID_FIELD)) {
		__CLEAR_FLAG(probe->flags, PROBE_HAS_ID_FIELD);
		kfree(probe->id);
		probe->id = NULL;
	}
	return probe;
}
STACK_FRAME_NON_STANDARD(destroy_probe);

void *destroy_kernel_sensor(struct kernel_sensor *sensor)
{

	struct probe *probe_p;
	struct connection *conn_c;
	assert(sensor);

	/* sensor is the parent of all probes */

	rcu_read_lock();
	probe_p = list_first_or_null_rcu(&sensor->probes,
									 struct probe,
									 l_node);
	rcu_read_unlock();
	while (probe_p != NULL) {
		spin_lock(&sensor->lock);
		list_del_rcu(&probe_p->l_node);
		spin_unlock(&sensor->lock);
		synchronize_rcu();
		if (__FLAG_IS_SET(probe_p->flags, PROBE_KPS)) {
			((struct kernel_ps_probe *)probe_p)->_destroy(probe_p);
		} else if (__FLAG_IS_SET(probe_p->flags, PROBE_KLSOF)) {
			((struct kernel_lsof_probe *)probe_p)->_destroy(probe_p);
		} else if (__FLAG_IS_SET(probe_p->flags, PROBE_KSYSFS)) {
			((struct kernel_sysfs_probe *)probe_p)->_destroy(probe_p);
		} else {
			probe_p->destroy(probe_p);
		}
		/**
		 * TODO: kernel-ps is statically allocated, but not all
		 * probes will be. some probes will need to be kfreed()'d
		 **/
		rcu_read_lock();
		probe_p = list_first_or_null_rcu(&sensor->probes,
										 struct probe,
										 l_node);
		rcu_read_unlock();
	}
	synchronize_rcu();
	rcu_read_lock();
	conn_c = list_first_or_null_rcu(&sensor->listeners,
									struct connection,
									l_node);
	rcu_read_unlock();
	while (conn_c != NULL) {
		spin_lock(&sensor->lock);
		list_del_rcu(&conn_c->l_node);
		spin_unlock(&sensor->lock);
		synchronize_rcu();
		if (__FLAG_IS_SET(conn_c->flags, PROBE_LISTEN)) {
			/**
			 * the listener is statically allocated, no need to
			 * free the memory
			 **/

			conn_c->_destroy(conn_c);
		}
		synchronize_rcu();
		rcu_read_lock();
		conn_c = list_first_or_null_rcu(&sensor->listeners,
										struct connection,
										l_node);
		rcu_read_unlock();
	}
	conn_c = NULL;
	rcu_read_lock();
	conn_c = list_first_or_null_rcu(&sensor->connections,
									struct connection,
									l_node);
	rcu_read_unlock();
	while (conn_c != NULL) {
		spin_lock(&sensor->lock);
        list_del_rcu(&conn_c->l_node);
	    spin_unlock(&sensor->lock);
		if (__FLAG_IS_SET(conn_c->flags, PROBE_CONNECT)) {
			/**
			 * a connection is dynamically allocated, so
			 * need to kfree the memory
			 **/

			conn_c->_destroy(conn_c);
			synchronize_rcu();
			kfree(conn_c);
			conn_c = NULL;
		}
		rcu_read_lock();
		conn_c = list_first_or_null_rcu(&sensor->connections,
										struct connection,
										l_node);
		rcu_read_unlock();
	}
/* now destroy the sensor's anonymous probe struct */
	probe_p = (struct probe *)sensor;
	destroy_probe(probe_p);
	memset(sensor, 0, sizeof(struct kernel_sensor));
	return sensor;
}
STACK_FRAME_NON_STANDARD(destroy_kernel_sensor);


/**
 * shuts down a kthread worker, but does not free the
 * memory - part of the transisiton to the worker belonging
 * inside the probe struct.
 **/
void
controller_destroy_worker(struct kthread_worker *worker)

{
	struct task_struct *task;

	task = worker->task;
	if (WARN_ON(!task)) {
		return;
	}

	CONT_FLUSH_WORKER(worker);
	kthread_stop(task);
	WARN_ON(!list_empty(&worker->work_list));
	return;
}


/**
 * init_k_sensor - initialize a kernel sensor, which is the parent
 * of k_probes.
 *
 * sensor is already allocated
 *
 * - clear sensor memory
 * - init the spinlock and probe list head
 * - allocate memory for sensor's ID and data
 * - does not initialize kwork work node, which is done by the
 *   kthreads library.
 **/
struct kernel_sensor * init_kernel_sensor(struct kernel_sensor *sensor)
{
	if (!sensor) {
		return ERR_PTR(-ENOMEM);
	}
	memset(sensor, 0, sizeof(struct kernel_sensor));

/* we have an anonymous struct probe, need to initialize it.
 * the anonymous struct is the first member of this structure.
 */
	init_probe((struct probe *)sensor,
			   "Kernel Sensor", strlen("Kernel Sensor") + 1);
	sensor->_init = init_kernel_sensor;
	INIT_LIST_HEAD_RCU(&sensor->probes);
	INIT_LIST_HEAD_RCU(&sensor->listeners);
	INIT_LIST_HEAD_RCU(&sensor->connections);
	sensor->_destroy = destroy_kernel_sensor;
	/* initialize the socket later when we listen*/
	return(sensor);
}

/**
 * A probe routine is a kthread  worker, called from kernel thread.
 * Once it runs once, it must be re-initialized
 * and re-queued to run again...see /include/linux/kthread.h
 **/

/**
 * ps probe function
 *
 * Called by the kernel contoller kmod to "probe" the processes
 * running on this kernel. Keeps track of how many times is has run.
 * Each time the sample runs it increments its count, prints probe
 * information, and either reschedules itself or dies.
 **/
void  run_kps_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_ps_probe *probe_struct =
		container_of(work, struct kernel_ps_probe, work);
	static int count;
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));

	/**
	 * if another process is reading the ps flex_array, kernel_ps
	 * will return -EFAULT. therefore, reschedule and try again.
	 */
	while( probe_struct->ps(probe_struct, count, nonce) == -EAGAIN) {
		schedule();
	}
/**
 *  call print_kernel_ps by default. But, in the future there will be other             *  options, notably outputting in json format to a socket
 **/
	probe_struct->print(probe_struct, "kernel-ps", nonce, ++count);
	probe_struct->repeat--;
	if (probe_struct->repeat > 0 && (! atomic64_read(&SHOULD_SHUTDOWN))) {
		int i;
		for (i = 0;
			 i < probe_struct->timeout && (! atomic64_read(&SHOULD_SHUTDOWN));
			 i++) {
			sleep(1);
		}
		if (! atomic64_read(&SHOULD_SHUTDOWN)) {
			init_and_queue_work(work, co_worker, run_kps_probe);
		}
	}
	return;
}

/**
 * init_probe - initialize a probe that has already been allocated.
 * struct probe is usually used as an anonymous structure in a more
 * specialized type of probe, for example kernel-ps or kernel-lsof
 *  - probe memory is already allocated and should be zero'd by the
 *    caller
 *  - initialize the probe's spinlock and list head
 *  - possibly allocates memory for the probe's ID and data
 *  - does not initialize probe->probe_work with a work node,
 *    which does need to get done by the kthreads library.
 * Initialize a probe structure.
 * void *init_probe(struct probe *p, uint8_t *id, uint8_t *data)
 *
 **/

/**
 * probe is RECEIVING the message
 **/


extern int
default_send_msg_to(struct probe *probe, int msg, void *in_buf, ssize_t len);

extern int
default_rcv_msg_from(struct probe *probe,
					 int msg,
					 void **out_buf,
					 ssize_t *len);

struct probe *init_probe(struct probe *probe,
						 uint8_t *id, int id_size)
{
	if (!probe) {
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD_RCU(&probe->l_node);
	probe->init =  init_probe;
	probe->destroy = destroy_probe;
	probe->send_msg_to_probe = default_send_msg_to;
	probe->rcv_msg_from_probe = default_rcv_msg_from;
	if (id && id_size > 0) {
		probe->id = kzalloc(id_size, GFP_KERNEL);
		if (!probe->id) {
			return ERR_PTR(-ENOMEM);
		}
		__SET_FLAG(probe->flags, PROBE_HAS_ID_FIELD);
/**
 * memcpy the number of chars minus the null terminator,
 * which is already NULL. make sure to call this with
 * stlen(id) + 1 as the length of the id field.
 **/
		memcpy(probe->id, id, id_size - 1);
	}

	probe->lock=__SPIN_LOCK_UNLOCKED("probe");
	/* flags, timeout, repeat are zero'ed */
	/* probe_work is NULL */

	__SET_FLAG(probe->flags, PROBE_INITIALIZED);
	return probe;
}
EXPORT_SYMBOL(init_probe);



static void *destroy_kernel_ps_probe(struct probe *probe)
{
	struct kernel_ps_probe *ps_p = (struct kernel_ps_probe *)probe;
	assert(ps_p && __FLAG_IS_SET(ps_p->flags, PROBE_KPS));

	if (__FLAG_IS_SET(probe->flags, PROBE_INITIALIZED)) {
		destroy_probe(probe);
	}

	if (ps_p->kps_data_flex_array) {
		flex_array_free(ps_p->kps_data_flex_array);
	}

	memset(ps_p, 0, sizeof(struct kernel_ps_probe));
	return ps_p;
}
STACK_FRAME_NON_STANDARD(destroy_kernel_ps_probe);

struct kernel_ps_probe *init_kernel_ps_probe(struct kernel_ps_probe *ps_p,
											 uint8_t *id, int id_len,
											 int (*print)(struct kernel_ps_probe *,
														  uint8_t *, uint64_t, int))
{
	int ccode = 0;
	struct probe *tmp;

	if (!ps_p) {
		return ERR_PTR(-ENOMEM);
	}
	memset(ps_p, 0, sizeof(struct kernel_ps_probe));
	/* init the anonymous struct probe */

	tmp = init_probe((struct probe *)ps_p, id, id_len);
	/* tmp will be a good pointer if init returned successfully,
	   an error pointer otherwise */
	if (ps_p != (struct kernel_ps_probe *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}


	/** init timeout and repeat
	 * they are passed on the command line, or (eventually) read
	 * from sysfs
	 **/
	__SET_FLAG(ps_p->flags, PROBE_KPS);

	ps_p->timeout = ps_timeout;
	ps_p->repeat = ps_repeat;

	ps_p->_init = init_kernel_ps_probe;
	ps_p->_destroy = destroy_kernel_ps_probe;
	if (print) {
		ps_p->print = print;
	} else {
		ps_p->print = print_kernel_ps;
	}

	ps_p->ps = kernel_ps;
	ps_p->kps_data_flex_array =
		flex_array_alloc(PS_DATA_SIZE, PS_ARRAY_SIZE, GFP_KERNEL);

	if (!ps_p->kps_data_flex_array) {
/* flex_array_alloc will return NULL upon failure, a valid pointer otherwise */
		ccode = -ENOMEM;
		goto err_exit;
	}
	ccode = flex_array_prealloc(ps_p->kps_data_flex_array, 0, PS_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_flex_array;
	}

	printk(KERN_INFO "kernel-ps PS_DATA_SIZE %ld PS_ARRAY_SIZE %ld\n",
		   PS_DATA_SIZE, PS_ARRAY_SIZE);


/* now queue the kernel thread work structures */
	CONT_INIT_WORK(&ps_p->work, run_kps_probe);
	__SET_FLAG(ps_p->flags, PROBE_HAS_WORK);
	CONT_INIT_WORKER(&ps_p->worker);
	CONT_QUEUE_WORK(&ps_p->worker,
					&ps_p->work);
	kthread_run(kthread_worker_fn, &ps_p->worker,
				"kernel-ps");
	return ps_p;

err_free_flex_array:
	flex_array_free(ps_p->kps_data_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}
STACK_FRAME_NON_STANDARD(init_kernel_ps_probe);


/**
 * this init reflects the correct workflow - the struct work needs to be
 * initialized and queued into the worker, before running the worker.
 * the worker will dequeue the work and run work->func(work),
 * then someone needs to decide if the work should be requeued.
 **/
static int __init kcontrol_init(void)
{
	int ccode = 0;
	struct kernel_ps_probe *ps_probe = NULL;
	struct kernel_lsof_probe *lsof_probe = NULL;
	struct kernel_sysfs_probe *sysfs_probe = NULL;

	if (&k_sensor != init_kernel_sensor(&k_sensor)) {
		return -ENOMEM;
	}

	/**
	 * initialize the ps probe
	 **/
	ps_probe = init_kernel_ps_probe(&kps_probe,
									"Kernel PS Probe",
									strlen("Kernel PS Probe") + 1,
									print_kernel_ps);

	if (ps_probe == ERR_PTR(-ENOMEM)) {
		ccode = -ENOMEM;
		goto err_exit;
	}

	spin_lock(&k_sensor.lock);
	/* link this probe to the sensor struct */
	list_add_rcu(&ps_probe->l_node, &k_sensor.probes);
	spin_unlock(&k_sensor.lock);

    /**
     * initialize the lsof probe
	 **/

	lsof_probe = init_kernel_lsof_probe(&klsof_probe,
										"Kernel LSOF Probe",
										strlen("Kernel LSOF Probe") + 1,
										print_kernel_lsof,
										lsof_pid_filter);
	if (lsof_probe == ERR_PTR(-ENOMEM)) {
		ccode = -ENOMEM;
		goto err_exit;
	}


	spin_lock(&k_sensor.lock);
	/* link this probe to the sensor struct */
	list_add_rcu(&lsof_probe->l_node, &k_sensor.probes);
	spin_unlock(&k_sensor.lock);


/**
 * initialize the sysfs probe
 **/

	sysfs_probe = init_sysfs_probe(&ksysfs_probe,
								   "Kernel Sysfs Probe",
								   strlen("Kernel Sysfs Probe") + 1,
								   print_sysfs_data,
								   filter_sysfs_data);

	if (sysfs_probe == ERR_PTR(-ENOMEM)) {
		ccode = -ENOMEM;
		goto err_exit;
	}

	spin_lock(&k_sensor.lock);
	list_add_rcu(&sysfs_probe->l_node, &k_sensor.probes);
	spin_unlock(&k_sensor.lock);

/**
 * initialize the socket interface
 * TODO: socket_interface_init always returns zero, it should
 * return an error code
 **/
    socket_interface_init();
	return ccode;

err_exit:
	if (ps_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(ps_probe->flags, PROBE_INITIALIZED)) {
			ps_probe->_destroy((struct probe *)ps_probe);
		}
		kfree(ps_probe);
		ps_probe = NULL;
	}

	if (lsof_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(lsof_probe->flags, PROBE_INITIALIZED)) {
			lsof_probe->_destroy((struct probe *)lsof_probe);
		}
		kfree(lsof_probe);
		lsof_probe = NULL;
	}

	if (sysfs_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(sysfs_probe->flags, PROBE_INITIALIZED)) {
			sysfs_probe->_destroy((struct probe *)sysfs_probe);
		}
		kfree(sysfs_probe);
		sysfs_probe = NULL;
	}
	return ccode;
}

/**
 * now there is a sensor struct, and probes are linked to the sensor in a list
 *
 **/
static void __exit controller_cleanup(void)
{
	/* destroy, but do not free the sensor */
	/* sensor is statically allocated, no need to free it. */
	atomic64_set(&SHOULD_SHUTDOWN, 1);
	socket_interface_exit();
/**
 * Note: always call socket_interface_exit() BEFORE calling k_sensor._destroy
 **/
	k_sensor._destroy(&k_sensor);
}

module_init(kcontrol_init);
module_exit(controller_cleanup);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO);
