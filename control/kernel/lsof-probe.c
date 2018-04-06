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

int lsof_filter(struct kernel_lsof_probe *p, void *data, size_t l)
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

int
kernel_lsof(struct kernel_lsof_probe *parent, int count, uint64_t nonce)
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


void *
destroy_kernel_lsof_probe(struct probe *probe)
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

struct kernel_lsof_probe *
init_kernel_lsof_probe(struct kernel_lsof_probe *lsof_p,
					   uint8_t *id, int id_len,
					   int (*print)(struct kernel_lsof_probe *,
									uint8_t *, uint64_t, int),
					   int (*filter)(struct kernel_lsof_probe *,
									 void *, size_t))
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
