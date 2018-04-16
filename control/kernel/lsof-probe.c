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

/* default lsof filter */

/**
 * default, dummy filter, for development and test
 * hard-wired to filter out all processes with a
 * pid > 1, which means systemd
 **/
int
lsof_pid_filter(struct kernel_lsof_probe *p,
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
lsof_uid_filter(struct kernel_lsof_probe *p,
				struct kernel_lsof_data *d,
				void *cmp)
{
	kuid_t *user_id  = (kuid_t *)cmp;

	return (d->user_id.val == user_id->val) ? 1 : 0;
}

int
lsof_all_files(struct kernel_lsof_probe *p,
				   struct kernel_lsof_data *d,
				   void *cmp)
{
	return 1;
}

int
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
		if (klsof_p) {
			if (klsof_p->nonce != nonce) {
				break;
			}
			printk(KERN_INFO "%s uid: %d pid: %d %s\n",
				   tag,
				   klsof_p->user_id.val,
				   klsof_p->pid_nr,
				   klsof_p->dpath + klsof_p->dp_offset);
		} else {
			printk(KERN_INFO "array indexing error in print lsof\n");
			spin_unlock_irqrestore(&parent->lock, flags);
			return -ENOMEM;
		}
	}
	spin_unlock_irqrestore(&parent->lock, flags);
	if (index == LSOF_ARRAY_SIZE) {
		return -ENOMEM;
	}
	return index;
}



struct task_struct *get_task_by_pid_number(pid_t pid)
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
kernel_lsof(struct kernel_lsof_probe *parent, int count, uint64_t nonce)
{
	int index = 0, fd_index = 0;
	struct task_struct *task;
	static struct kernel_lsof_data klsofd;
	static struct lsof_filter filter = {{0}, -1};

	unsigned long flags;

	if (!spin_trylock_irqsave(&parent->lock, flags)) {
		return -EAGAIN;
	}

	rcu_read_lock();
	for_each_process(task) {
		task_lock(task);
		filter.pid = task->pid;
		klsofd.nonce = nonce;
		klsofd.index = index;
		klsofd.user_id = task_uid(task);
		klsofd.pid_nr = task->pid;

		if (parent->filter == lsof_pid_filter) {
/**
 * TODO: pid is hard-coded to 1. Make it a configurable parameter
 *       as is, we only look for files opened by PID 1 (systemd)
 *       make it configurable via sysfs
 **/
			static pid_t process_id = 1;
			if (! lsof_pid_filter(parent, &klsofd, &process_id))
				goto unlock_task;
		} else if (parent->filter == lsof_uid_filter){
/**
 * TODO: user id is hard-coded to 0. Make it a configurable parameter
 *       as is, we only look for files opened by root (uid 0)
 *       make it configurable via sysfs
 **/
			kuid_t user_id = {0};

			if (! lsof_uid_filter(parent, &klsofd, &user_id))
				goto unlock_task;
		} else {
			if (! parent->filter(parent, &klsofd, NULL))
				goto unlock_task;;
		}

		klsofd.files = task->files;

		assert(klsofd.files);

		klsofd.files_table = files_fdtable(klsofd.files);
		while(klsofd.files_table != NULL &&
			  klsofd.files_table->fd[fd_index] != NULL) {

			klsofd.file = get_file(klsofd.files_table->fd[fd_index]);
			klsofd.files_path = klsofd.files_table->fd[fd_index]->f_path;
			klsofd.dp = d_path(&klsofd.files_path,
							   klsofd.dpath, MAX_DENTRY_LEN - 1);

			fput_atomic(klsofd.file);
			klsofd.dp_offset = (klsofd.dp - &klsofd.dpath[0]);
			klsofd.dp = (&klsofd.dpath[0] + klsofd.dp_offset);

			if (index <  LSOF_ARRAY_SIZE) {
				flex_array_put(parent->klsof_data_flex_array,
							   index,
							   &klsofd,
							   GFP_ATOMIC);
				index++;
			} else {
				printk(KERN_INFO "lsof flex array over-run\n");
				index = -ENOMEM;
				task_unlock(task);
				goto unlock_out;
			}
			fd_index++;
		}

	unlock_task:
		task_unlock(task);
	}

unlock_out:
	rcu_read_unlock();
	spin_unlock_irqrestore(&parent->lock, flags);
	return index;
}

void
run_klsof_probe(struct kthread_work *work)
{
	struct kthread_worker *co_worker = work->worker;
	struct kernel_lsof_probe *probe_struct =
		container_of(work, struct kernel_lsof_probe, work);
	static int count;
	uint64_t nonce;
	get_random_bytes(&nonce, sizeof(uint64_t));
	/**
	 * if another process is reading the lsof flex_array, kernel_lsof
	 * will return -EFAULT. therefore, reschedule and try again.
	 */
	probe_struct->lsof(probe_struct, count++, nonce);

	if (SHOULD_SHUTDOWN)
		return;

/**
 *  call print by default. But, in the future there will be other
 *  options, notably outputting in json format to a socket
 **/
	probe_struct->print(probe_struct, "kernel-lsof", nonce, ++count);
	probe_struct->repeat--;
	if (probe_struct->repeat > 0 &&  !SHOULD_SHUTDOWN) {
		int i;
		for(i = 0; i < probe_struct->timeout && !SHOULD_SHUTDOWN; i++) {
			sleep(1);
		}
		if (!SHOULD_SHUTDOWN)
			init_and_queue_work(work, co_worker, run_klsof_probe);
	}
	return;
}


void *
destroy_kernel_lsof_probe(struct probe *probe)
{
	struct kernel_lsof_probe *lsof_p = (struct kernel_lsof_probe *)probe;
	assert(lsof_p && __FLAG_IS_SET(lsof_p->flags, PROBE_KLSOF));

	if (__FLAG_IS_SET(probe->flags, PROBE_INITIALIZED)) {
		destroy_probe(probe);
	}


	if (lsof_p->klsof_pid_flex_array) {
		flex_array_free(lsof_p->klsof_pid_flex_array);
	}

	if (lsof_p->klsof_data_flex_array) {
		flex_array_free(lsof_p->klsof_data_flex_array);
	}

	memset(lsof_p, 0, sizeof(struct kernel_lsof_probe));
	return lsof_p;
}

struct kernel_lsof_probe *
init_kernel_lsof_probe(struct kernel_lsof_probe *lsof_p,
					   uint8_t *id, int id_len,
					   int (*print)(struct kernel_lsof_probe *,
									uint8_t *, uint64_t, int),
					   int (*filter)(struct kernel_lsof_probe *,
									 struct kernel_lsof_data *,
									 void *))
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
	 * they are passed on the command line, or read
	 * from sysfs
	 **/
	__SET_FLAG(lsof_p->flags, PROBE_KLSOF);

	lsof_p->timeout = lsof_timeout;
	lsof_p->repeat = lsof_repeat;

	lsof_p->_init = init_kernel_lsof_probe;
	lsof_p->_destroy = destroy_kernel_lsof_probe;
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
		flex_array_alloc(LSOF_PID_EL_SIZE, LSOF_PID_EL_ARRAY_SIZE, GFP_KERNEL);
	if (!lsof_p->klsof_pid_flex_array) {
		ccode = -ENOMEM;
		goto err_free_flex_array;
	}

	ccode = flex_array_prealloc(lsof_p->klsof_pid_flex_array, 0,
								LSOF_PID_EL_ARRAY_SIZE,
								GFP_KERNEL | __GFP_ZERO);
	if(ccode) {
		/* ccode will be zero for success, -ENOMEM otherwise */
		goto err_free_pid_flex_array;
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
err_free_pid_flex_array:
	flex_array_free(lsof_p->klsof_pid_flex_array);
err_free_flex_array:
	flex_array_free(lsof_p->klsof_data_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}
