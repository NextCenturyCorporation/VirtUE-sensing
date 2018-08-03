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

/**
 * TODO:  https://lwn.net/Articles/588444/
 * checking for unused flags
 **/

struct kernel_ps_sensor kps_probe;
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

long chunk_size = 0x400;
module_param(chunk_size, long, 0644);
MODULE_PARM_DESC(chunk_size, "chunk size for virtual files");

long max_size = (~0UL >> 1);
module_param(max_size, long, 0644);
MODULE_PARM_DESC(max_size, "largest file buffer to allocate");




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

int
is_regular_file(struct file *f)
{
	if (!S_ISREG(file_inode(f)->i_mode))
		return 0;

	if (i_size_read(file_inode(f)) <= 0)
		return 0;

	return 1;
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
		}
		filp_close(f, 0);
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
		}
		filp_close(f, 0);
	} else {
		ccode = -EBADF;
  		pr_err("Unable to open file: %s (%ld)", name, ccode);
	}
	return ccode;
}
STACK_FRAME_NON_STANDARD(read_file);

ssize_t
kernel_read_file_with_name(char *name, void **buf, size_t max_count, loff_t *pos)
{
#ifdef MODERN_FILE_API
	return kernel_read_file_from_path(name,
									  buf,
									  pos,
									  max_count,
									  READING_UNKNOWN);
#else
	return -ENOSYS;
#endif
}


ssize_t
vfs_read_file(char *name, void **buf, size_t max_count, loff_t *pos)
{
	ssize_t ccode = 0;
	struct file *f = NULL;

	assert(buf);
	*buf = NULL;
	assert(pos);
	*pos = 0LL;

	f = filp_open(name, O_RDONLY, 0);
	if (f) {
		ssize_t chunk = chunk_size, allocated = 0, cursor = 0;
		*buf = kzalloc(chunk, GFP_KERNEL);
		if (*buf) {
			allocated = chunk;
		} else {
			ccode =  -ENOMEM;
			goto out_err;
		}

		do {
			/**
			 * read one chunk at a time
			 **/
			cursor = *pos; /* initially zero, then positioned with further reads */
			ccode = kernel_read(f, *buf + cursor, chunk, pos);
			if (ccode < 0) {
				pr_err("Unable to read file chunk: %s (%ld)", name, ccode);
				goto out_err;
			}
			if (ccode > 0) {
				*buf = krealloc(*buf, allocated + chunk, GFP_KERNEL);
				if (! *buf) {
					ccode = -ENOMEM;
					goto out_err;
				}
				allocated += chunk;
			}
		} while (ccode && allocated <= max_count);
		filp_close(f, 0);
	} else {
		ccode = -EBADF;
		pr_err("Unable to open file: %s (%ld)", name, ccode);
	}
	return ccode;

out_err:
	if (f) {
		filp_close(f, 0);
	}
	if  (*buf) {
		kfree(*buf);
		*buf = NULL;
	}
	return ccode;
}


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

/**
 * Assumes: caller holds p->lock
 **/
int
build_pid_index_unlocked(struct sensor *p,
						 struct flex_array *a,
						 uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	static pid_el pel;

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
			printk(KERN_DEBUG "kernel pid index array over-run\n");
			return -ENOMEM;
		}
	}
	return index;
}


int
build_pid_index(struct sensor *p, struct flex_array *a, uint64_t nonce)
{
	int index = 0;

	if (!spin_trylock(&p->lock)) {
		return -EAGAIN;
	}
	index = build_pid_index_unlocked(p, a, nonce);
	spin_unlock(&p->lock);
	return index;
}
STACK_FRAME_NON_STANDARD(build_pid_index);

/**
 * TODO: convert get_probe to use the probe->uuid field
 **/
/**
 *  probe is LOCKED upon return
 **/
int
get_probe(uint8_t *sensor_name, struct sensor **p)
{
	struct sensor *sensor_p = NULL;
	int ccode = -ENFILE;
	assert(sensor_name);
	assert(p);

	*p = NULL;
	rcu_read_lock();
	list_for_each_entry_rcu(sensor_p, &k_sensor.sensors, l_node) {
		if (! strncmp(sensor_p->name, sensor_name, strlen(sensor_p->name))) {
			if(!spin_trylock(&sensor_p->lock)) {
				ccode =  -EAGAIN;
				goto exit;
			} else {
				*p = sensor_p;
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
 * @brief build a JSON array containing the id strings of each probe
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
	struct sensor *s_cursor = NULL;

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
	list_for_each_entry_rcu(s_cursor, &k_sensor.sensors, l_node) {
		int id_len;
		if ((remaining - 5) > (id_len = strlen(s_cursor->name))) {
			if (count > 0) {
				*cursor++ = COMMA; remaining--;
				*cursor++ = SPACE; remaining--;
				*cursor++ = D_QUOTE; remaining--;
			}
			strncpy(cursor, s_cursor->name, remaining - 3);
			remaining -= id_len;
			cursor += id_len;
			*cursor++ = D_QUOTE; remaining--;
			count++;
		} else {
			goto err_exit;
		}
	}
	rcu_read_unlock();
	if (remaining > 2) {
		*cursor++ = R_BRACKET; remaining--;
		*cursor++ = 0x00; remaining--;
	}
	if (remaining > 0x100) {
		*buf = krealloc(*buf, (*len - (remaining - 2)), GFP_KERNEL);
		*len -= (remaining - 2);
	}

	return 0;

err_exit:
	if (*buf) kfree(*buf);
	*buf = NULL;
	*len = -ENOMEM;
	return -ENOMEM;
}
\

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
void *destroy_sensor(struct sensor *sensor)
{
	assert(sensor && __FLAG_IS_SET(sensor->flags, SENSOR_INITIALIZED));
	__CLEAR_FLAG(sensor->flags, SENSOR_INITIALIZED);

	if (__FLAG_IS_SET(sensor->flags, SENSOR_HAS_WORK)) {
		controller_destroy_worker(&sensor->worker);
		__CLEAR_FLAG(sensor->flags, SENSOR_HAS_WORK);
	}
	if (sensor->name &&
		__FLAG_IS_SET(sensor->flags, SENSOR_HAS_NAME_FIELD)) {
		__CLEAR_FLAG(sensor->flags, SENSOR_HAS_NAME_FIELD);
		kfree(sensor->name);
		sensor->name = NULL;
	}
	return sensor;
}
STACK_FRAME_NON_STANDARD(destroy_sensor);

void *destroy_kernel_sensor(struct kernel_sensor *sensor)
{

	struct sensor *sensor_p;
	struct connection *conn_c;
	assert(sensor);

	/* sensor is the parent of all probes */

	rcu_read_lock();
	sensor_p = list_first_or_null_rcu(&sensor->sensors,
									 struct sensor,
									 l_node);
	rcu_read_unlock();
	while (sensor_p != NULL) {
		spin_lock(&sensor->lock);
		list_del_rcu(&sensor_p->l_node);
		spin_unlock(&sensor->lock);
		synchronize_rcu();
		if (__FLAG_IS_SET(sensor_p->flags, SENSOR_KPS)) {
			((struct kernel_ps_sensor *)sensor_p)->_destroy(sensor_p);
		} else if (__FLAG_IS_SET(sensor_p->flags, SENSOR_KLSOF)) {
			((struct kernel_lsof_probe *)sensor_p)->_destroy(sensor_p);
		} else if (__FLAG_IS_SET(sensor_p->flags, SENSOR_KSYSFS)) {
			((struct kernel_sysfs_probe *)sensor_p)->_destroy(sensor_p);
		} else {
			sensor_p->destroy(sensor_p);
		}
		/**
		 * TODO: kernel-ps is statically allocated, but not all
		 * probes will be. some probes will need to be kfreed()'d
		 **/
		rcu_read_lock();
		sensor_p = list_first_or_null_rcu(&sensor->sensors,
										 struct sensor,
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
		if (__FLAG_IS_SET(conn_c->flags, SENSOR_LISTEN)) {
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
		if (__FLAG_IS_SET(conn_c->flags, SENSOR_CONNECT)) {
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
	sensor_p = (struct sensor *)sensor;
	destroy_sensor(sensor_p);
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
	init_sensor((struct sensor *)sensor,
			   "Kernel Sensor", strlen("Kernel Sensor") + 1);
	sensor->_init = init_kernel_sensor;
	INIT_LIST_HEAD_RCU(&sensor->sensors);
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


int
default_sensor_message(struct sensor *sensor, struct sensor_msg *msg)
{
	assert(sensor && msg);

	if (msg->id < CONNECT || msg->id > RECORDS) {
		return -EINVAL;
	}
	msg->ccode = 0;
	return 0;
}


struct sensor *init_sensor(struct sensor *sensor,
						 uint8_t *name, int name_size)
{
	if (!sensor) {
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD_RCU(&sensor->l_node);
	sensor->init =  init_sensor;
	sensor->destroy = destroy_sensor;
	sensor->message = default_sensor_message;
	if (name && name_size > 0) {
		sensor->name = kzalloc(name_size, GFP_KERNEL);
		if (!sensor->name) {
			return ERR_PTR(-ENOMEM);
		}
		__SET_FLAG(sensor->flags, SENSOR_HAS_NAME_FIELD);
/**
 * memcpy the number of chars minus the null terminator,
 * which is already NULL. make sure to call this with
 * stlen(id) + 1 as the length of the id field.
 **/
		memcpy(sensor->name, name, name_size - 1);
	}
	/* generate the probe uuid */
	generate_random_uuid(sensor->uuid);
	sensor->lock=__SPIN_LOCK_UNLOCKED("sensor");
	/* flags, timeout, repeat are zero'ed */
	/* probe_work is NULL */

	__SET_FLAG(sensor->flags, SENSOR_INITIALIZED);
	return sensor;
}
EXPORT_SYMBOL(init_sensor);




/**
 * this init reflects the correct workflow - the struct work needs to be
 * initialized and queued into the worker, before running the worker.
 * the worker will dequeue the work and run work->func(work),
 * then someone needs to decide if the work should be requeued.
 **/
static int __init kcontrol_init(void)
{
	int ccode = 0;
	struct kernel_ps_sensor *ps_sensor = NULL;
	struct kernel_lsof_probe *lsof_probe = NULL;
//	struct kernel_sysfs_probe *sysfs_probe = NULL;

	if (&k_sensor != init_kernel_sensor(&k_sensor)) {
		return -ENOMEM;
	}

	/**
	 * initialize the ps probe
	 **/
	ps_sensor = init_kernel_ps_sensor(&kps_probe,
									"Kernel PS Sensor",
									strlen("Kernel PS Sensor") + 1,
									print_kernel_ps);

	if (ps_sensor == ERR_PTR(-ENOMEM)) {
		ccode = -ENOMEM;
		goto err_exit;
	}

	spin_lock(&k_sensor.lock);
	/* link this probe to the sensor struct */
	list_add_rcu(&ps_sensor->l_node, &k_sensor.sensors);
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
	list_add_rcu(&lsof_probe->l_node, &k_sensor.sensors);
	spin_unlock(&k_sensor.lock);


/**
 * initialize the sysfs probe


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

**/

/**
 * initialize the socket interface
 * TODO: socket_interface_init always returns zero, it should
 * return an error code
 **/
    socket_interface_init();
	return ccode;

err_exit:
	if (ps_sensor != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(ps_sensor->flags, SENSOR_INITIALIZED)) {
			ps_sensor->_destroy((struct sensor *)ps_sensor);
		}
		kfree(ps_sensor);
		ps_sensor = NULL;
	}

	if (lsof_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(lsof_probe->flags, SENSOR_INITIALIZED)) {
			lsof_probe->_destroy((struct sensor *)lsof_probe);
		}
		kfree(lsof_probe);
		lsof_probe = NULL;
	}

	/**
	if (sysfs_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(sysfs_probe->flags, SENSOR_INITIALIZED)) {
			sysfs_probe->_destroy((struct sensor *)sysfs_probe);
		}
		kfree(sysfs_probe);
		sysfs_probe = NULL;
	}
	**/
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
