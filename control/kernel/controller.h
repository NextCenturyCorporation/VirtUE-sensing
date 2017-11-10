/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/

/* the kernel itself has dynamic trace points, they 
 *  need to be part of the probe capability.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <kernel/trace/trace.h>

/* prototype probe routine */
int (*probe)(uint64_t flags, uint8_t *buf) = NULL;
#define DEFAUT_PROBE_DATA 1024
struct probe_s {
	uint8_t *id;
	spinlock_t lock;
	uint64_t flags, timeout, repeat;
	int (*probe)(uint64_t, uint8_t *);
	uint8_t data[DEFAUT_PROBE_DATA];
};

int register_probe(uint64_t flags,
				   int (*probe)(uint64_t, uint8_t *),
				   int delay, int timeout, int repeat);
int unregister_probe(uint8_t *probe_id);

/* allocates and returns a buffer with probe data */
uint8_t *list_probes(int level, uint8_t *filter);
int set_probe_level(int level, uint8_t *filter);
/* retrieve the probe to the client or set to the server, */
/* the probe data structure */
int retrieve_probe(uint8_t *probe_id,
				   uint8_t *results_buf,
				   int buf_size);
uint64_t update_probe(uint8_t *probe_id,
					  uint8_t *update,
					  int update_size);
