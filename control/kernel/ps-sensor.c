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

/**
 * @brief copy one record of the kernel_ps list to a memory buffer
 * to be called by the record message handler to build a single
 * record response message
 *
 * @param parent - the kernel ps probe instance
 * @param msg - a struct probe_msg that contains input and
 *        output parameters.
 * @param tag - expected to be the probe id string
 * @return error code or zero
 *
 * Optionally runs the ps probe to re-populate the flex array
 * with new ps records.
 * Note: the parent probe is LOCKED by the caller, and caller
 * will unlock the probe upon return from this function.
 **/
int
kernel_ps_get_record(struct kernel_ps_sensor *parent,
					 struct sensor_msg *msg,
					 uint8_t *tag)
{
	struct kernel_ps_data *kpsd_p;
	int ccode = 0;
	ssize_t cur_len = 0;
	struct records_request *rr = (struct records_request *)msg->input;
	struct records_reply *rp = (struct records_reply *)msg->output;
	uint8_t *r_header = "{" PROTOCOL_VERSION ", reply: [";

	assert(parent);
	assert(msg);
	assert(tag);
	assert(rr);
	assert(rp);
	assert(msg->input_len == (sizeof(struct records_request)));
	assert(msg->output_len == (sizeof(struct records_reply)));
	assert(rr->index >= 0 && rr->index < PS_ARRAY_SIZE);
	if (rr->index >= PS_ARRAY_SIZE) {
		printk(KERN_DEBUG "rr->index too large?? %d\n", rr->index);
	}

	if (!rp->records || rp->records_len <=0) {
		return -ENOMEM;
	}

	if (rr->run_probe) {
		/**
		 * refresh all the ps records in the flex array
		 **/
		ccode = kernel_ps_unlocked(parent, rr->nonce);
		if (ccode < 0) {
			return ccode;
		}
	}

	kpsd_p = flex_array_get(parent->kps_data_flex_array, rr->index);
	if (!kpsd_p || kpsd_p->clear == FLEX_ARRAY_FREE ||
		(rr->nonce && kpsd_p->nonce != rr->nonce)) {
		/**
		 * when there is no entry, or the entry is clear, return an
		 * empty record response. per the protocol control/kernel/messages.md
		 **/
		cur_len = scnprintf(rp->records,
							rp->records_len - 1,
							"%s \'%s\', \'%s\', \'%s\']}\n",
							r_header,
							rr->json_msg->s->nonce,
							parent->name,
							parent->uuid_string);
		rp->index = -ENOENT;
		goto record_created;
	}

	/**
	 * build the record json object(s)
	 **/
	cur_len = scnprintf(rp->records,
						rp->records_len - 1,
						"%s \'%s\', \'%s\', \'%s\', \'%s\', \'%d\', \'%s\'. \'%d\',"
                        "\'%d\', \'%llx\']}\n",
						r_header, rr->json_msg->s->nonce, parent->name,
						tag, parent->uuid_string, rr->index, kpsd_p->comm,
						kpsd_p->pid_nr, kpsd_p->user_id.val, rr->nonce);
	rp->index = rr->index;
record_created:
	if (kpsd_p && rr->clear) {
		flex_array_clear(parent->kps_data_flex_array, rr->index);
	}
	rp->records_len = cur_len;
	rp->records[cur_len] = 0x00;
	rp->records = krealloc(rp->records, cur_len + 1, GFP_KERNEL);
	rp->range = rr->range;
	return 0;
}



/**
 * @brief output the kernel-ps list to the kernel log
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

int
print_kernel_ps(struct kernel_ps_sensor *parent,
				uint8_t *tag,
				uint64_t nonce,
				int count)
{
	int index;
	struct kernel_ps_data *kpsd_p;

	if(unlikely(!print_to_log)) {
		return 0;
	}

	if (!spin_trylock(&parent->lock)) {
		return -EAGAIN;
	}
	for (index = 0; index < PS_ARRAY_SIZE; index++)  {
		kpsd_p = flex_array_get(parent->kps_data_flex_array, index);
		if (!kpsd_p) {
			return -ENFILE;
		}
		if (kpsd_p->clear == FLEX_ARRAY_FREE) {
			continue;
		}
		if (nonce && kpsd_p->nonce != nonce) {
			break;
		}
		printk(KERN_INFO "%s %d:%d %s [%d] [%d] [%llx]\n",
			   tag, count, index, kpsd_p->comm, kpsd_p->pid_nr,
			   kpsd_p->user_id.val, nonce);
	}
	spin_unlock(&parent->lock);
	if (index == PS_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
}
STACK_FRAME_NON_STANDARD(print_kernel_ps);

int kernel_ps_unlocked(struct kernel_ps_sensor *parent, uint64_t nonce)
{

	struct task_struct *task;
	struct kernel_ps_data kpsd = {0};
	int index = 0;

	rcu_read_lock();
	for_each_process(task) {
		task_lock(task);
		kpsd.nonce = nonce;
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
		task_unlock(task);
		index++;
	}
unlock_out:
	rcu_read_unlock();
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

inline int
kernel_ps(struct kernel_ps_sensor *parent, int count, uint64_t nonce)
{

	int index;

	if (!spin_trylock(&parent->lock)) {
		return -EAGAIN;
	}
	index = kernel_ps_unlocked(parent, nonce);
	spin_unlock(&parent->lock);
	return index;
}


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
	struct kernel_ps_sensor *probe_struct =
		container_of(work, struct kernel_ps_sensor, work);
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
 * probe is LOCKED upon entry
 **/
static int
ps_message(struct sensor *sensor, struct sensor_msg *msg)
{
	switch(msg->id) {
	case RECORDS:
	{
		return kernel_ps_get_record((struct kernel_ps_sensor *)sensor,
									msg,
									"kernel-ps");
	}
	default:
	{
		/**
		 * process_state_message will cause invalid msg->id codes
		 * to fail with -EINVAL
		 **/
		return sensor->state_change(sensor, msg);
	}
	}
}

static void *destroy_kernel_ps_sensor(struct sensor *sensor)
{
	struct kernel_ps_sensor *ps_p = (struct kernel_ps_sensor *)sensor;
	assert(ps_p && __FLAG_IS_SET(ps_p->flags, SENSOR_KPS));

	if (__FLAG_IS_SET(sensor->flags, SENSOR_INITIALIZED)) {
		destroy_sensor(sensor);
	}

	if (ps_p->kps_data_flex_array) {
		flex_array_free(ps_p->kps_data_flex_array);
	}

	memset(ps_p, 0, sizeof(struct kernel_ps_sensor));
	return ps_p;
}
STACK_FRAME_NON_STANDARD(destroy_kernel_ps_sensor);

struct kernel_ps_sensor *init_kernel_ps_sensor(struct kernel_ps_sensor *ps_p,
											 uint8_t *id, int id_len,
											 int (*print)(struct kernel_ps_sensor *,
														  uint8_t *, uint64_t, int))
{
	int ccode = 0;
	struct sensor *tmp;

	if (!ps_p) {
		return ERR_PTR(-ENOMEM);
	}
	memset(ps_p, 0, sizeof(struct kernel_ps_sensor));
	/* init the anonymous struct probe */

	tmp = init_sensor((struct sensor *)ps_p, id, id_len);
	/* tmp will be a good pointer if init returned successfully,
	   an error pointer otherwise */
	if (ps_p != (struct kernel_ps_sensor *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}


	/** init timeout and repeat
	 * they are passed on the command line, or (eventually) read
	 * from sysfs
	 **/
	__SET_FLAG(ps_p->flags, SENSOR_KPS);

	ps_p->timeout = ps_timeout;
	ps_p->repeat = ps_repeat;

	ps_p->_init = init_kernel_ps_sensor;
	ps_p->_destroy = destroy_kernel_ps_sensor;
	ps_p->message = ps_message;

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
	__SET_FLAG(ps_p->flags, SENSOR_HAS_WORK);
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
STACK_FRAME_NON_STANDARD(init_kernel_ps_sensor);
