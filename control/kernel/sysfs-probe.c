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


int
kernel_sysfs(struct kernel_sysfs_probe *p, int c, uint64_t nonce)
{
	DMSG();
	return 0;
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

	if (sysfs_p->ksysfs_pid_array) {
		flex_array_free(sysfs_p->ksysfs_pid_array);
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

	sysfs_p->ksysfs_pid_array =
		flex_array_alloc(PID_EL_SIZE, PID_EL_ARRAY_SIZE, GFP_KERNEL);
	if (!sysfs_p->ksysfs_flex_array) {
		ccode = -ENOMEM;
		goto err_free_flex_array;
	}

	ccode = flex_array_prealloc(sysfs_p->ksysfs_pid_array, 0,
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
	flex_array_free(sysfs_p->ksysfs_pid_array);
err_free_flex_array:
	flex_array_free(sysfs_p->ksysfs_flex_array);
err_exit:
	/* if the probe has been initialized, need to destroy it */
	return ERR_PTR(ccode);
}



