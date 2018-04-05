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
struct connection listener;
EXPORT_SYMBOL(listener);

struct kernel_ps_probe kps_probe;
struct kernel_lsof_probe klsof_probe;

static unsigned int lsof_repeat = 1;
static unsigned int lsof_timeout = 1;

static unsigned int ps_repeat = 1;
static unsigned int ps_timeout = 1;

char *socket_name = "/var/run/kernel_sensor";

module_param(lsof_repeat, uint, 0644);
module_param(lsof_timeout, uint, 0644);

MODULE_PARM_DESC(ps_repeat, "How many times to run the kernel lsof function");
MODULE_PARM_DESC(ps_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "lsof function");

module_param(ps_repeat, uint, 0644);
module_param(ps_timeout, uint, 0644);

MODULE_PARM_DESC(ps_repeat, "How many times to run the kernel ps function");
MODULE_PARM_DESC(ps_timeout,
				 "How many seconds to sleep in between calls to the kernel " \
				 "ps function");
module_param(socket_name, charp, 0644);



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

static inline int
build_discovery_buffer(uint8_t **buf, size_t *len)
{

	uint8_t *cursor;
	int remaining, count = 0;
	struct probe *p_cursor;

	assert(buf && len);

	*len = CONNECTION_MAX_HEADER;
	*buf = kzalloc(CONNECTION_MAX_HEADER, GFP_KERNEL);
	if (*buf <= 0) {
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
			}
			*cursor++ = D_QUOTE; remaining--;
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
	if (remaining > 1) {
		*cursor = R_BRACKET; remaining--;
		*cursor = 0x00; remaining--;
	}
	if (remaining > 0x100) {
		*buf = krealloc(*buf, (*len - (remaining - 1)), GFP_KERNEL);
		*len -= (remaining - 1);
	}
	return 0;

err_exit:
	if (*buf) kfree(*buf);
	*len = -ENOMEM;
	return -ENOMEM;
}

static int
print_kernel_lsof(struct kernel_lsof_probe *parent,
							 uint8_t *tag,
							 uint64_t nonce,
							 int count)
{

	int index;
	unsigned long flags;
	struct kernel_lsof_data *klsof_p;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}
	for (index = 0; index < LSOF_ARRAY_SIZE; index++)  {
		klsof_p = flex_array_get(parent->klsof_data_flex_array, index);
		if (klsof_p->nonce != nonce) {
			break;
		}
		printk(KERN_INFO "%s %d:%d \n",
			   tag, count, index);
	}
	spin_unlock_irqrestore(&parent->lock, flags);
	if (index == LSOF_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
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

int kernel_ps(struct kernel_ps_probe *parent, int count, uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	struct kernel_ps_data kpsd;
	unsigned long flags;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}

	for_each_process(task) {
		kpsd.nonce = nonce;
		kpsd.index = index;
		kpsd.user_id = task_uid(task);
		kpsd.pid_nr = task->pid;
		memcpy(kpsd.comm, task->comm, TASK_COMM_LEN);
		if (index <  PS_ARRAY_SIZE) {
			flex_array_put(parent->kps_data_flex_array, index, &kpsd, GFP_ATOMIC);
		} else {
			index = -ENOMEM;
			goto unlock_out;
		}
		index++;
	}

unlock_out:
	spin_unlock_irqrestore(&parent->lock, flags);
	return index;
}

int kernel_lsof(struct kernel_lsof_probe *parent, int count, uint64_t nonce)
{
	int index = 0;
	struct task_struct *task;
	struct kernel_lsof_data klsofd;
	unsigned long flags;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}

	for_each_process(task) {
		klsofd.nonce = nonce;
		klsofd.index = index;
		klsofd.user_id = task_uid(task);
		klsofd.pid_nr = task->pid;
		memcpy(klsofd.comm, task->comm, TASK_COMM_LEN);
		if (index <  PS_ARRAY_SIZE) {
			flex_array_put(parent->klsof_data_flex_array, index, &klsofd, GFP_ATOMIC);
		} else {
			index = -ENOMEM;
			goto unlock_out;
		}
		index++;
	}

unlock_out:
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

void *destroy_kernel_sensor(struct kernel_sensor *sensor)
{

	struct probe *probe_p;
	struct connection *conn_c;
	unsigned long flags;
	assert(sensor);

	/* sensor is the parent of all probes */

	rcu_read_lock();
	probe_p = list_first_or_null_rcu(&sensor->probes,
									 struct probe,
									 l_node);
	rcu_read_unlock();
	while (probe_p != NULL) {
		spin_lock_irqsave(&sensor->lock, flags);
		list_del_rcu(&probe_p->l_node);
		spin_unlock_irqrestore(&sensor->lock, flags);
		synchronize_rcu();
		if (__FLAG_IS_SET(probe_p->flags, PROBE_KPS)) {
			((struct kernel_ps_probe *)probe_p)->_destroy(probe_p);
		} else if (__FLAG_IS_SET(probe_p->flags, PROBE_KLSOF)) {
			((struct kernel_lsof_probe *)probe_p)->_destroy(probe_p);
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
		spin_lock_irqsave(&sensor->lock, flags);
		list_del_rcu(&conn_c->l_node);
		spin_unlock_irqrestore(&sensor->lock, flags);
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
		spin_lock_irqsave(&sensor->lock, flags);
        list_del_rcu(&conn_c->l_node);
	    spin_unlock_irqrestore(&sensor->lock, flags);
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

	if (probe_struct->repeat && (! SHOULD_SHUTDOWN)) {
		probe_struct->repeat--;
		sleep(probe_struct->timeout);
		init_and_queue_work(work, co_worker, run_kps_probe);
	}
	return;
}

void  run_klsof_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_lsof_probe *probe_struct =
		container_of(work, struct kernel_lsof_probe, work);
	static int count;
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));

	/**
	 * if another process is reading the ps flex_array, kernel_ps
	 * will return -EFAULT. therefore, reschedule and try again.
	 */
	while( probe_struct->lsof(probe_struct, count, nonce) == -EAGAIN) {
		schedule();
	}
/**
 *  call print by default. But, in the future there will be other
 *  options, notably outputting in json format to a socket
 **/
	probe_struct->print(probe_struct, "kernel-lsof", nonce, ++count);

	if (probe_struct->repeat && (! SHOULD_SHUTDOWN)) {
		probe_struct->repeat--;
		sleep(probe_struct->timeout);
		init_and_queue_work(work, co_worker, run_klsof_probe);
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

struct probe *init_probe(struct probe *probe,
						 uint8_t *id, int id_size)
{
	if (!probe) {
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD_RCU(&probe->l_node);
	probe->init =  init_probe;
	probe->destroy = destroy_probe;
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

static void *destroy_kernel_lsof_probe(struct probe *probe)
{
	struct kernel_lsof_probe *lsof_p = (struct kernel_lsof_probe *)probe;
	assert(lsof_p && __FLAG_IS_SET(lsof_p->flags, PROBE_KLSOF));

	if (__FLAG_IS_SET(probe->flags, PROBE_INITIALIZED)) {
		destroy_probe(probe);
	}

	if (lsof_p->klsof_data_flex_array) {
		flex_array_free(lsof_p->klsof_data_flex_array);
	}

	memset(lsof_p, 0, sizeof(struct kernel_lsof_probe));
	return lsof_p;
}

struct kernel_lsof_probe *init_kernel_lsof_probe(struct kernel_lsof_probe *lsof_p,
											 uint8_t *id, int id_len,
											 int (*print)(struct kernel_lsof_probe *,
														  uint8_t *, uint64_t, int))
{
	int ccode = 0;
	struct probe *tmp;

	if (!lsof_p) {
		return ERR_PTR(-ENOMEM);
	}
	memset(lsof_p, 0, sizeof(struct kernel_lsof_probe));
	/* init the anonymous struct probe */

	tmp = init_probe((struct probe *)lsof_p, id, id_len);
	/* tmp will be a good pointer if init returned successfully,
	   an error pointer otherwise */
	if (lsof_p != (struct kernel_lsof_probe *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}


	/** init timeout and repeat
	 * they are passed on the command line, or (eventually) read
	 * from sysfs
	 * TODO: these need to be separate from the kps probe
	 **/
	__SET_FLAG(lsof_p->flags, PROBE_KLSOF);

	lsof_p->timeout = ps_timeout;
	lsof_p->repeat = ps_repeat;

	lsof_p->_init = init_kernel_lsof_probe;
	lsof_p->_destroy = destroy_kernel_lsof_probe;
	if (print) {
		lsof_p->print = print;
	} else {
		lsof_p->print = print_kernel_lsof;
	}

	lsof_p->lsof = kernel_lsof;
	lsof_p->klsof_data_flex_array =
		flex_array_alloc(LSOF_DATA_SIZE, LSOF_ARRAY_SIZE, GFP_KERNEL);

	if (!lsof_p->klsof_data_flex_array) {
/* flex_array_alloc will return NULL upon failure, a valid pointer otherwise */
		ccode = -ENOMEM;
		goto err_exit;
	}
	ccode = flex_array_prealloc(lsof_p->klsof_data_flex_array, 0, LSOF_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_flex_array;
	}

	printk(KERN_INFO "kernel-lsof LSOF_DATA_SIZE %ld LSOF_ARRAY_SIZE %ld\n",
		   LSOF_DATA_SIZE, LSOF_ARRAY_SIZE);


/* now queue the kernel thread work structures */
	CONT_INIT_WORK(&lsof_p->work, run_klsof_probe);
	__SET_FLAG(lsof_p->flags, PROBE_HAS_WORK);
	CONT_INIT_WORKER(&lsof_p->worker);
	CONT_QUEUE_WORK(&lsof_p->worker,
					&lsof_p->work);
	kthread_run(kthread_worker_fn, &lsof_p->worker,
				"kernel-lsof");
	return lsof_p;

err_free_flex_array:
	flex_array_free(lsof_p->klsof_data_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}



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

	unsigned long flags;

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

	spin_lock_irqsave(&k_sensor.lock, flags);
	/* link this probe to the sensor struct */
	list_add_rcu(&ps_probe->l_node, &k_sensor.probes);
	spin_unlock_irqrestore(&k_sensor.lock, flags);

    /**
     * initialize the lsof probe
	 **/

	lsof_probe = init_kernel_lsof_probe(&klsof_probe,
										"Kernel LSOF Probe",
										strlen("Kernel LSOF Probe") + 1,
										print_kernel_lsof);
	spin_lock_irqsave(&k_sensor.lock, flags);
	/* link this probe to the sensor struct */
	list_add_rcu(&lsof_probe->l_node, &k_sensor.probes);
	spin_unlock_irqrestore(&k_sensor.lock, flags);


	return ccode;

err_exit:
	if (ps_probe != ERR_PTR(-ENOMEM)) {
		if (__FLAG_IS_SET(ps_probe->flags, PROBE_INITIALIZED)) {
			ps_probe->_destroy((struct probe *)ps_probe);
		}
		kfree(ps_probe);
		ps_probe = NULL;
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
	SHOULD_SHUTDOWN = 1;
	k_sensor._destroy(&k_sensor);

}

module_init(kcontrol_init);
module_exit(controller_cleanup);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO);
