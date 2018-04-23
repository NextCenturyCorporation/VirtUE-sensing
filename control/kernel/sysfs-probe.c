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
 * caller holds a reference cont on struct task AND
 * holds a lock on p->lock
 **/

/**
 * as a demonstration for this early version of the sysfs probe,
 * all paths are under /proc, but this is not a restriction, just
 * a demonstration
 **/
int sysfs_read_data(struct kernel_sysfs_probe *p,
					struct task_struct *t,
					int *start,
					char *path,
					uint64_t nonce)
{

	static struct kernel_sysfs_data ksysfsd;
	mm_segment_t old_fs;
	int ccode = 0;
	struct file *f;

/**
 * get a read file handle, then get the stat struct, then
 * use the file size to allocated a read buffer, then read the file
 **/
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	memset(&ksysfsd, 0x00, sizeof(struct kernel_sysfs_data));

	ksysfsd.fd = IMPORTED(sys_open)(path, O_RDONLY, 0644);
	if (ksysfsd.fd > 0) {
		ccode = vfs_fstat(ksysfsd.fd, &ksysfsd.stat);
		if (ccode) {
			goto err_exit;
		}
		if (ksysfsd.stat.size <= 0) {
			ccode = -ENODATA;
			goto err_exit;
		}
		ksysfsd.data_len = ksysfsd.stat.size;
		ksysfsd.data = kzalloc(ksysfsd.data_len + 1, GFP_KERNEL);
		if (ZERO_OR_NULL_PTR(ksysfsd.data)) {
			ccode = -ENOMEM;
			goto err_exit;
		}

		/**
		 * we have the file handle, file size, file buffer, now
		 * get a reference to the struct file *
		 **/
		f = fget(ksysfsd.fd);
		if (f) {
			loff_t pos = 0;
			ccode = kfs_read(f, ksysfsd.data, ksysfsd.data_len, &pos);
			/**
			 * put back the reference to struct file *
			 **/
			fput(f);
		} else {
			kfree(ksysfsd.data);
			ksysfsd.data = NULL;
			ccode = -EBADF;
		}
		ksysfsd.nonce = nonce;
		ksysfsd.index = *start;
		ksysfsd.pid = t->pid;
		ksysfsd.ccode = ccode;
		if (*start <  LSOF_ARRAY_SIZE) {
			flex_array_put(p->ksysfs_flex_array,
						   *start,
						   &ksysfsd,
						   GFP_ATOMIC);
			(*start)++;
		} else {
			printk(KERN_INFO "lsof flex array over-run\n");
			return -ENOMEM;
		}
	}
err_exit:
	set_fs(old_fs);
	return ccode;
}

int
sysfs_for_each_pid(struct kernel_sysfs_probe *p, int count, uint64_t nonce)
{
	int index, ccode = 0, file_index = 0;
	struct task_struct *task;
	unsigned long flags;
	pid_el *pid_el_p;
	static char sysfs_path[MAX_DENTRY_LEN + 1];
	static char *format = "/proc/%d/mounts";

	if (!spin_trylock_irqsave(&p->lock, flags)) {
		return -EAGAIN;
	}
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
			printk(KERN_INFO "array indexing error in lsof_for_each_pid\n");
			spin_unlock_irqrestore(&p->lock, flags);
			return -ENOMEM;
		}
	}
	spin_unlock_irqrestore(&p->lock, flags);
	return file_index;
}




int
kernel_sysfs(struct kernel_sysfs_probe *p, int c, uint64_t nonce)
{
	int count;

	count = build_pid_index((struct probe *)p, p->ksysfs_pid_flex_array, nonce);
	count = sysfs_for_each_pid(p, count, nonce);

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
	printk(KERN_INFO "%s %llx %d\n", tag, nonce, count);
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
 **/
	probe_struct->print(probe_struct, "kernel-sysfs", nonce, ++count);
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
	if (!sysfs_p) {
		return ERR_PTR(-ENOMEM);
	}

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



