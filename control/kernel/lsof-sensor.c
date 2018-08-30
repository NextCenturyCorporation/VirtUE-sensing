/*******************************************************************
 * in-Virtue Kernel Controller: lsof probe
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
#include "import-header.h"



/**
 * probe is LOCKED upon entry
 **/
static int
lsof_message(struct sensor *sensor, struct sensor_msg *msg)
{
	switch(msg->id) {
	case RECORDS: {
		return kernel_lsof_get_record((struct kernel_lsof_sensor *)sensor,
									  msg,
									  "kernel-lsof");
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

/**
 * default, dummy filter, for development and test
 * hard-wired to filter out all processes with a
 * pid > 1, which means systemd
 **/
int
lsof_pid_filter(struct kernel_lsof_sensor *p,
				struct kernel_lsof_data* d,
				void *cmp)

{
	return (d->pid_nr == *(pid_t *)cmp) ? 1 : 0;
}


/**
 * hard-wired to filter out all processes with a
 * uid != *data
 **/
int
lsof_uid_filter(struct kernel_lsof_sensor *p,
				struct kernel_lsof_data *d,
				void *cmp)
{
	kuid_t *user_id  = (kuid_t *)cmp;

	return (d->user_id.val == user_id->val) ? 1 : 0;
}

int
lsof_all_files(struct kernel_lsof_sensor *p,
			   struct kernel_lsof_data *d,
			   void *cmp)
{
	return 1;
}

/**
 * @brief copy one record of the kernel_lsof list to a memory buffer
 * to be called by the record message handler to build a single
 * record response message
 *
 * @param parent - the kernel ps probe instance
 * @param msg - a struct probe_msg that contains input and
 *        output parameters.
 * @param tag - expected to be the probe id string
 * @return error code or zero
 *
 * Optionally runs the lsof probe to re-populate the flex array
 * with new lsof records.
 * Note: the parent probe is LOCKED by the caller
 **/
int
kernel_lsof_get_record(struct kernel_lsof_sensor *parent,
					   struct sensor_msg *msg,
					   uint8_t *tag)
{
	struct kernel_lsof_data *klsof_p;
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
	assert(rr->index >= 0 && rr->index < LSOF_ARRAY_SIZE);

	if (rr->run_probe) {
		/**
		 * refresh all the ps records in the flex array
		 **/
		ccode = kernel_lsof_unlocked(parent, rr->nonce);
		if (ccode < 0) {
			return ccode;
		}
	}

	klsof_p = flex_array_get(parent->klsof_data_flex_array, rr->index);
	if (!klsof_p || klsof_p->clear == FLEX_ARRAY_FREE ||
		(rr->nonce && klsof_p->nonce != rr->nonce)) {
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

	if (!rp->records || rp->records_len <=0) {
		return -ENOMEM;
	}
	/**
	 * build the record json object(s)
	 **/

	cur_len = scnprintf(rp->records,
						rp->records_len - 1,
						"%s \'%s\', \'%s\', \'%s\', \'%s\',"
						"\'uid: %d pid: %d flags: %x "
						"mode: %x count: %ld %s\']}\n",
						r_header,
						rr->json_msg->s->nonce,
						parent->name,
						tag,
						parent->uuid_string,
						klsof_p->user_id.val,
						klsof_p->pid_nr,
						klsof_p->flags,
						klsof_p->mode,
						atomic64_read(&klsof_p->count),
						klsof_p->dpath + klsof_p->dp_offset);
	rp->index = rr->index;

record_created:
	if (klsof_p && rr->clear) {
		flex_array_clear(parent->klsof_data_flex_array, rr->index);
	}

	rp->records_len = cur_len;
	rp->records[cur_len] = 0x00;
	rp->records = krealloc(rp->records, cur_len + 1, GFP_KERNEL);
	rp->range = rr->range;
	return 0;
}

int
print_kernel_lsof(struct kernel_lsof_sensor *parent,
				  uint8_t *tag,
				  uint64_t nonce)
{

	int index;
	struct kernel_lsof_data *klsof_p;

	if(unlikely(!print_to_log)) {
		return 0;
	}

	if (!spin_trylock(&parent->lock)) {
		return -EAGAIN;
	}
	for (index = 0; index < LSOF_ARRAY_SIZE; index++)  {
		klsof_p = flex_array_get(parent->klsof_data_flex_array, index);
		if (klsof_p) {
			if (klsof_p->nonce != nonce) {
				break;
			}

			printk(KERN_INFO "%s uid: %d pid: %d flags: %x "
				   "mode: %x count: %lx %s \n",
				   tag,
				   klsof_p->user_id.val,
				   klsof_p->pid_nr,
				   klsof_p->flags,
				   klsof_p->mode,
				   atomic64_read(&klsof_p->count),
				   klsof_p->dpath + klsof_p->dp_offset);
		} else {
			printk(KERN_INFO "array indexing error in print lsof\n");
			spin_unlock(&parent->lock);
			return -ENOMEM;
		}
	}
	spin_unlock(&parent->lock);
	if (index == LSOF_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
}
STACK_FRAME_NON_STANDARD(print_kernel_lsof);


/**
 * caller holds a reference cont on struct task AND
 * holds a lock on p->lock
 **/
int
lsof_get_files_struct(struct kernel_lsof_sensor *p,
						  struct task_struct *t,
						  int *start,
						  uint64_t nonce)
{
	static struct kernel_lsof_data klsofd;
	int fd_index = 0;
	struct files_struct *files;
	struct file *file;
	struct fdtable *files_table;


	if (! pid_alive(t) || ! t->files) {
 		/**
 		 * occasionally there will be a dead task_struct
 		 * due to timing issues.
 		 **/
 		printk(KERN_INFO "task pid is dead: %d\n", t->pid);
 		return 0;
 	}
	files = t->files;

	memset(&klsofd, 0x00, sizeof(struct kernel_lsof_data));
	files_table = t->files->fdt;

	while(files_table && files_table->fd[fd_index]) {
		file = get_file(files_table->fd[fd_index]);
		klsofd.dp = d_path(&file->f_path,
						   klsofd.dpath, MAX_DENTRY_LEN - 1);
		klsofd.flags = file->f_flags;
		klsofd.mode = file->f_mode;
		/* don't include the probe in the count */
		klsofd.count = file->f_count;
		atomic_long_dec(&klsofd.count);
		fput_atomic(file);
		klsofd.dp_offset = (klsofd.dp - &klsofd.dpath[0]);
		klsofd.dp = (&klsofd.dpath[0] + klsofd.dp_offset);
		klsofd.user_id = task_uid(t);
		klsofd.nonce = nonce;
		klsofd.index = *start;
		klsofd.pid_nr = t->pid;
		if (*start <  LSOF_ARRAY_SIZE) {
			flex_array_put(p->klsof_data_flex_array,
						   *start,
						   &klsofd,
						   GFP_ATOMIC);
			(*start)++;
		} else {
			printk(KERN_INFO "lsof flex array over-run\n");
			return -ENOMEM;
		}
		fd_index++;
	}
	return 0;
}
STACK_FRAME_NON_STANDARD(lsof_get_files_struct);


/**
 * Assumes: caller holds p->lock
 **/

int
lsof_for_each_pid_unlocked(struct kernel_lsof_sensor *p,
						   uint64_t nonce)
{
	int index, ccode = 0, file_index = 0;
	struct task_struct *task;
	struct lsof_pid_el *pid_el_p;

	for (index = 0; index < PID_EL_ARRAY_SIZE; index++)  {
		pid_el_p = flex_array_get(p->klsof_pid_flex_array, index);
		if (pid_el_p) {
			if (pid_el_p->nonce != nonce) {
				break;
			}
			task = get_task_by_pid_number(pid_el_p->pid);
			if (task && pid_alive(task)) {
				ccode = lsof_get_files_struct(p,
											  task,
											  &file_index,
											  nonce);
				put_task_struct(task);
			}
		} else {
			printk(KERN_INFO "array indexing error in lsof_for_each_pid\n");
			return -ENOMEM;
		}
	}
	return file_index;
}

int
kernel_lsof_unlocked(struct kernel_lsof_sensor *p,
					 uint64_t nonce)
{
	int count;
	count = build_pid_index_unlocked((struct sensor *)p,
									 p->klsof_pid_flex_array,
									 nonce);
	count = lsof_for_each_pid_unlocked(p, nonce);
	return count;
}


int
kernel_lsof(struct kernel_lsof_sensor *p, uint64_t nonce)
{
	int count;

	if (!spin_trylock(&p->lock)) {
		return -EAGAIN;
	}
	count = build_pid_index_unlocked((struct sensor *)p,
									 p->klsof_pid_flex_array,
									 nonce);
	count = lsof_for_each_pid_unlocked(p, nonce);
	spin_unlock(&p->lock);
	return count;
}

void
run_klsof_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_lsof_sensor *probe_struct =
		container_of(work, struct kernel_lsof_sensor, work);
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));
	probe_struct->lsof(probe_struct, nonce);

/**
 *  call print by default. But, in the future there will be other
 *  options, notably outputting in json format to a socket
 **/
	probe_struct->print(probe_struct, "kernel-lsof", nonce);
	probe_struct->repeat--;
	if (probe_struct->repeat > 0 &&  (! atomic64_read(&SHOULD_SHUTDOWN))) {
		int i;
		for(i = 0;
			i < probe_struct->timeout && (! atomic64_read(&SHOULD_SHUTDOWN));
			i++) {
			sleep(1);
		}
		if (! atomic64_read(&SHOULD_SHUTDOWN))
			init_and_queue_work(work, co_worker, run_klsof_probe);
	}
	return;
}


void *
destroy_kernel_lsof_sensor(struct sensor *sensor)
{
	struct kernel_lsof_sensor *lsof_p = (struct kernel_lsof_sensor *)sensor;
	assert(lsof_p && __FLAG_IS_SET(lsof_p->flags, SENSOR_KLSOF));

	if (__FLAG_IS_SET(sensor->flags, SENSOR_INITIALIZED)) {
		destroy_sensor(sensor);
	}

	if (lsof_p->klsof_pid_flex_array) {
		flex_array_free(lsof_p->klsof_pid_flex_array);
	}

	if (lsof_p->klsof_data_flex_array) {
		flex_array_free(lsof_p->klsof_data_flex_array);
	}

	memset(lsof_p, 0, sizeof(struct kernel_lsof_sensor));
	return lsof_p;
}
STACK_FRAME_NON_STANDARD(destroy_kernel_lsof_sensor);

struct kernel_lsof_sensor *
init_kernel_lsof_sensor(struct kernel_lsof_sensor *lsof_p,
					   uint8_t *id, int id_len,
					   int (*print)(struct kernel_lsof_sensor *,
									uint8_t *, uint64_t),
					   int (*filter)(struct kernel_lsof_sensor *,
									 struct kernel_lsof_data *,
									 void *))
{
	int ccode = 0;
	struct sensor *tmp;

	if (!lsof_p) {
		return ERR_PTR(-ENOMEM);
	}
	memset(lsof_p, 0, sizeof(struct kernel_lsof_sensor));
	/* init the anonymous struct probe */

	tmp = init_sensor((struct sensor *)lsof_p, id, id_len);
	/* tmp will be a good pointer if init returned successfully,
	   an error pointer otherwise */
	if (lsof_p != (struct kernel_lsof_sensor *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}


	/** init timeout and repeat
	 * they are passed on the command line, or read
	 * from sysfs
	 **/
	__SET_FLAG(lsof_p->flags, SENSOR_KLSOF);

	lsof_p->timeout = lsof_timeout;
	lsof_p->repeat = lsof_repeat;

	lsof_p->_init = init_kernel_lsof_sensor;
	lsof_p->_destroy = destroy_kernel_lsof_sensor;
	lsof_p->message = lsof_message;
	if (print) {
		lsof_p->print = print;
	} else {
		lsof_p->print = print_kernel_lsof;
	}
	if (filter) {
		lsof_p->filter = filter;
	} else {
		lsof_p->filter = lsof_pid_filter;
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

	lsof_p->klsof_pid_flex_array =
		flex_array_alloc(PID_EL_SIZE, PID_EL_ARRAY_SIZE, GFP_KERNEL);
	if (!lsof_p->klsof_pid_flex_array) {
		ccode = -ENOMEM;
		goto err_free_flex_array;
	}

	ccode = flex_array_prealloc(lsof_p->klsof_pid_flex_array, 0,
								PID_EL_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_pid_flex_array;
	}

	printk(KERN_INFO "kernel-lsof LSOF_DATA_SIZE %ld LSOF_ARRAY_SIZE %ld\n",
		   LSOF_DATA_SIZE, LSOF_ARRAY_SIZE);


/* now queue the kernel thread work structures */
	CONT_INIT_WORK(&lsof_p->work, run_klsof_probe);
	__SET_FLAG(lsof_p->flags, SENSOR_HAS_WORK);
	CONT_INIT_WORKER(&lsof_p->worker);
	CONT_QUEUE_WORK(&lsof_p->worker,
					&lsof_p->work);
	kthread_run(kthread_worker_fn, &lsof_p->worker,
				"kernel-lsof");
	return lsof_p;
err_free_pid_flex_array:
	flex_array_free(lsof_p->klsof_pid_flex_array);
err_free_flex_array:
	flex_array_free(lsof_p->klsof_data_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}
STACK_FRAME_NON_STANDARD(init_kernel_lsof_sensor);
