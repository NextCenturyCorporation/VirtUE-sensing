/*******************************************************************
 * in-Virtue Kernel Controller: sysfs probe
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
 * as a demonstration for this early version of the sysfs probe,
 * all paths are under /proc, but this is not a restriction, just
 * a demonstration
 *
 **/
/**
 * Note: the parent probe is LOCKED by the caller, and caller
 * will unlock the probe upon return from this function.
 **/
int
sysfs_get_record(struct kernel_sysfs_probe *p,
				 struct probe_msg *msg,
				 uint8_t *tag)
{
	struct kernel_sysfs_data *kfsd_p;
	uint8_t *cursor;
	int ccode = 0;
	ssize_t cur_len = 0, raw_len = 0;
	struct records_request *rr = (struct records_request *)msg->input;
	struct records_reply *rp = (struct records_reply *)msg->output;
	uint8_t *r_header = "{" PROTOCOL_VERSION ", reply: [";
	uint8_t * raw_header = "{type: raw, length: ";

	assert(p);
	assert(msg);
	assert(tag);
	assert(rr);
	assert(rp);
	assert(msg->input_len == (sizeof(struct records_request)));
	assert(msg->output_len == (sizeof(struct records_reply)));
	assert(rr->index >= 0 && rr->index < SYSFS_ARRAY_SIZE);

	if (!rp->records || rp->records_len <=0) {
		return -ENOMEM;
	}


	if (rr->run_probe) {
		/**
		 * refresh all the ps records in the flex array
		 **/
		ccode = build_pid_index_unlocked(
			(struct probe *)p,
			p->ksysfs_pid_flex_array,
			rr->nonce);
		ccode = kernel_sysfs_unlocked(p, rr->nonce);
		if (ccode < 0) {
			return ccode;
		}
	}

	kfsd_p = flex_array_get(p->ksysfs_flex_array, rr->index);
	if (!kfsd_p || kfsd_p->clear == FLEX_ARRAY_FREE) {
	/**
	* when there is no entry, or the entry is clear, return an
	* empty record response. per the protocol control/kernel/messages.md
	**/
		cur_len = scnprintf(rp->records,
							rp->records_len - 1,
							"%s %s, %s ]}\n",
							r_header,
							rr->json_msg->s->nonce,
							p->id);

		rp->index = -ENOENT;
		goto record_created;
	}

	if (rr->nonce && kfsd_p->nonce != rr->nonce) {
		return -EINVAL;
	}


	/**
	 * build the record json object(s), with a raw extension
	 **/

	raw_len = sizeof(struct kstat) + kfsd_p->data_len;

	cur_len = scnprintf(rp->records,
						rp->records_len - 1,
						"%s %s, %s, %s %d %x %s %s %lx}]}\n",
						r_header,
						rr->json_msg->s->nonce,
						p->id,
						tag,
						rr->index,
						kfsd_p->pid,
						kfsd_p->dpath,
						raw_header,
						raw_len);

	rp->records = krealloc(rp->records, cur_len + raw_len + 1, GFP_KERNEL);
	if (! rp->records) {
		return -ENOMEM;
	}
	cur_len += (raw_len + 1);

	cursor = rp->records + cur_len;
	memcpy(cursor, &kfsd_p->stat, sizeof(struct kstat));
	cursor += sizeof(struct kstat);
	memcpy(cursor, kfsd_p->data, kfsd_p->data_len);
	if (kfsd_p && rr->clear) {
		kfree(kfsd_p->data);
		flex_array_clear(p->ksysfs_flex_array, rr->index);
	}

record_created:
	rp->records_len = cur_len;
	rp->records[cur_len] = 0x00;
	rp->range = rr->range;
	return 0;
}



static inline size_t
calc_file_size(struct kstat *kstat)
{
	if (kstat->size) {
		return kstat->size;
	}
	if (kstat->blocks) {
		return kstat->blocks * kstat->blksize;
	}
	return kstat->blksize > 0 ? kstat->blksize: 0x100;
}


/**
 * caller holds a reference count on struct task AND
 * holds a lock on p->lock
 *
 *
 * TODO: do we need the task_struct as a parameter?
 *       answer: no, we don't need the task struct.
 * TODO: remove task_struct, and change sysfs_get_record
 *       so it will read files based on a filter, instead
 *       of reading files in the /proc/ file system, which
 *       is the current proof-of-concept
 *
 **/
ssize_t sysfs_read_data(struct kernel_sysfs_probe *p,
						struct task_struct *t,
						int *start,
						char *path,
						uint64_t nonce)
{

	static struct kernel_sysfs_data ksysfsd, *clear_p;
	ssize_t ccode = 0;
	struct file *f = NULL;
	/**
	 * default size for /proc and /sys is 0x100, one block.
	 * they don't return a size in kstat
	 **/
	memset(&ksysfsd, 0x00, sizeof(struct kernel_sysfs_data));
	f = filp_open(path, O_RDONLY, 0);
	if (f) {
		size_t size = 0, max_size = 0x100000;
		loff_t pos = 0;
		ccode = file_getattr(f, &ksysfsd.stat);
		if (ccode) {
			printk(KERN_INFO "error getting file attributes %zx\n", ccode);
			goto err_exit;
		}
		/**
		 * get the size, or default size if /proc or /sys
		 * set an arbitrary limit of 1 MB for read buffer
		 **/
		size = min(calc_file_size(&ksysfsd.stat), max_size);
		ksysfsd.data = kzalloc(size, GFP_KERNEL);
		if (ksysfsd.data == NULL) {
			ccode = -ENOMEM;
			goto err_exit;
		}
		ksysfsd.data_len = size;
		ccode = ksysfsd.ccode = read_file_struct(f, ksysfsd.data, size, &pos);
		if (ccode < 0) {
			goto err_exit;
		}
		if (*start <  LSOF_ARRAY_SIZE) {
			/**
			 * check for an allocated data buffer stored in this
			 * array element - if its there, free it
			 **/
			 clear_p = flex_array_get(p->ksysfs_flex_array, *start);
			 if (clear_p && clear_p->clear != FLEX_ARRAY_FREE &&
				 clear_p->data) {
				kfree(clear_p->data);
			}

			flex_array_put(p->ksysfs_flex_array,
						   *start,
						   &ksysfsd,
						   GFP_ATOMIC);
			(*start)++;
		} else {
			printk(KERN_INFO "sysfs flex array over-run\n");
			ccode = -ENOMEM;
			goto err_exit;
		}

	} else {
		ccode = -EBADF;
		pr_err("sysfs-probe Unable to get a file handle: %s (%zx)\n", path, ccode);
		goto err_exit;
	}
	return ccode;

err_exit:
	if (f) {
		filp_close(f, 0);
	}
	if (ksysfsd.data != NULL) {
		kfree(ksysfsd.data);
	}
	return ccode;
}
STACK_FRAME_NON_STANDARD(sysfs_read_data);


int
sysfs_for_each_unlocked(struct kernel_sysfs_probe *p, uint64_t nonce)
{
	int index, ccode = 0, file_index = 0;
	struct task_struct *task;
	pid_el *pid_el_p;
	static char sysfs_path[MAX_DENTRY_LEN + 1];
	static char *format = "/proc/%d/mounts";


	for (index = 0; index < PID_EL_ARRAY_SIZE; index++)  {
		memset(sysfs_path, 0x00, MAX_DENTRY_LEN + 1);
		pid_el_p = flex_array_get(p->ksysfs_pid_flex_array, index);
		if (pid_el_p) {
			if (pid_el_p->nonce != nonce) {
				break;
			}

			snprintf(sysfs_path, MAX_DENTRY_LEN, format, pid_el_p->pid);
			file_index = index;
			task = get_task_by_pid_number(pid_el_p->pid);
			ccode = sysfs_read_data(p,
									task,
									&file_index,
									sysfs_path,
									nonce);
			put_task_struct(task);
		} else {
			printk(KERN_INFO "array indexing error in sysfs_for_each_pid\n");
			return -ENOMEM;
		}
	}
	return file_index;
}



int
sysfs_for_each_pid(struct kernel_sysfs_probe *p, int count, uint64_t nonce)
{
	int file_index = 0;

	if (!spin_trylock(&p->lock)) {
		return -EAGAIN;
	}

	file_index = sysfs_for_each_unlocked(p, nonce);

	spin_unlock(&p->lock);
	return file_index;
}
STACK_FRAME_NON_STANDARD(sysfs_for_each_pid);

int
kernel_sysfs_unlocked(struct kernel_sysfs_probe *p,
					  uint64_t nonce)
{
	int count;

	count = build_pid_index_unlocked((struct probe *)p,
									 p->ksysfs_pid_flex_array,
									 nonce);
	count = sysfs_for_each_unlocked(p, nonce);

	return count;
}

int
kernel_sysfs(struct kernel_sysfs_probe *p,
			 int c,
			 uint64_t nonce)
{
	int count;

	if (!spin_trylock(&p->lock)) {
		return -EAGAIN;
	}
	count = build_pid_index_unlocked((struct probe *)p,
									 p->ksysfs_pid_flex_array,
									 nonce);
	count = sysfs_for_each_unlocked(p, nonce);
	spin_unlock(&p->lock);
	return count;
}



int
filter_sysfs_data(struct kernel_sysfs_probe *p,
				  struct kernel_sysfs_data *sysfs_data,
				  void *data)
{
	return 1;
}


int
print_sysfs_data(struct kernel_sysfs_probe *p,
				 uint8_t *tag, uint64_t nonce, int count)
{
	if(print_to_log) {
		printk(KERN_INFO "%s %llx %d\n", tag, nonce, count);
	}

	return 0;
}


void
run_sysfs_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_sysfs_probe *probe_struct =
		container_of(work, struct kernel_sysfs_probe, work);
	int count = 0;
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));
	probe_struct->ksysfs(probe_struct, count, nonce);

/**
 *  call print by default. But, in the future there will be other
 *  options, notably outputting in json format to a socket
 * TODO: we now have the json output, so use it by default
 **/
	probe_struct->print(probe_struct, "kernel-sysfs", nonce, ++count);
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


/**
 * probe is LOCKED upon entry
 **/
static int
sysfs_message(struct probe *probe, struct probe_msg *msg)
{
	switch(msg->id) {
	case RECORDS: {
		return sysfs_get_record((struct kernel_sysfs_probe *)probe,
								msg,
								"kernel-sysfs");
	}
	default:
		return -EINVAL;
	}
}



void *
destroy_sysfs_probe(struct probe *probe)
{
	struct kernel_sysfs_probe *sysfs_p = (struct kernel_sysfs_probe *)probe;
	assert(sysfs_p && __FLAG_IS_SET(sysfs_p->flags, PROBE_KSYSFS));

	if (__FLAG_IS_SET(probe->flags, PROBE_INITIALIZED)) {
		destroy_probe(probe);
	}

	if (sysfs_p->ksysfs_pid_flex_array) {
		flex_array_free(sysfs_p->ksysfs_pid_flex_array);
	}

	if (sysfs_p->ksysfs_flex_array) {
		flex_array_free(sysfs_p->ksysfs_flex_array);
	}

	memset(sysfs_p, 0, sizeof(struct kernel_sysfs_probe));
	return sysfs_p;
}
STACK_FRAME_NON_STANDARD(destroy_sysfs_probe);

struct kernel_sysfs_probe *
init_sysfs_probe(struct kernel_sysfs_probe *sysfs_p,
				 uint8_t *id, int id_len,
				 int (*print)(struct kernel_sysfs_probe *,
							  uint8_t *, uint64_t, int),
				 int (*filter)(struct kernel_sysfs_probe *,
							   struct kernel_sysfs_data *,
							   void *))
{
	int ccode;
	struct probe *tmp;

	memset(sysfs_p, 0, sizeof(struct kernel_sysfs_probe));
	tmp = init_probe((struct probe *)sysfs_p, id, id_len);
	if(sysfs_p != (struct kernel_sysfs_probe *)tmp) {
		ccode = -ENOMEM;
		goto err_exit;
	}

	__SET_FLAG(sysfs_p->flags, PROBE_KSYSFS);
	sysfs_p->timeout = sysfs_timeout;
	sysfs_p->repeat = sysfs_repeat;

	sysfs_p->_init = init_sysfs_probe;
	sysfs_p->_destroy = destroy_sysfs_probe;
	sysfs_p->message = sysfs_message;


	if (print) {
		sysfs_p->print = print;
	} else {
		sysfs_p->print = print_sysfs_data;
	}
	if (filter) {
		sysfs_p->filter = filter;
	} else {
		sysfs_p->filter = filter_sysfs_data;
	}
	sysfs_p->ksysfs  = kernel_sysfs;


/**
 * allocate flex arrays
 **/

	sysfs_p->ksysfs_flex_array =
		flex_array_alloc(SYSFS_DATA_SIZE, SYSFS_ARRAY_SIZE, GFP_KERNEL);

	if (!sysfs_p->ksysfs_flex_array) {
/* flex_array_alloc will return NULL upon failure, a valid pointer otherwise */
		ccode = -ENOMEM;
		goto err_exit;
	}
	ccode = flex_array_prealloc(sysfs_p->ksysfs_flex_array, 0, SYSFS_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_flex_array;
	}

	sysfs_p->ksysfs_pid_flex_array =
		flex_array_alloc(PID_EL_SIZE, PID_EL_ARRAY_SIZE, GFP_KERNEL);
	if (!sysfs_p->ksysfs_pid_flex_array) {
		ccode = -ENOMEM;
		goto err_free_flex_array;
	}

	ccode = flex_array_prealloc(sysfs_p->ksysfs_pid_flex_array, 0,
								PID_EL_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_pid_flex_array;
	}

	/* now queue the kernel thread work structures */
	CONT_INIT_WORK(&sysfs_p->work, run_sysfs_probe);
	__SET_FLAG(sysfs_p->flags, PROBE_HAS_WORK);
	CONT_INIT_WORKER(&sysfs_p->worker);
	CONT_QUEUE_WORK(&sysfs_p->worker,
					&sysfs_p->work);
	kthread_run(kthread_worker_fn, &sysfs_p->worker,
				"kernel-sysfs");
	return sysfs_p;

err_free_pid_flex_array:
	flex_array_free(sysfs_p->ksysfs_pid_flex_array);
err_free_flex_array:
	flex_array_free(sysfs_p->ksysfs_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}
STACK_FRAME_NON_STANDARD(init_sysfs_probe);



