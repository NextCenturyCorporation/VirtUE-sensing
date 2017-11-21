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
/*#include <trace/ftrace.h>*/
#include <linux/list.h>
#include <linux/socket.h>
#include <linux/kthread.h>
#include <linux/slab.h>


/* - - probe - -
 *
 * A probe is an active sensor. It is a function that runs as a kernel
 * task at regular intervals to obtain information or to check the
 * truth of various assertions.

 * It's expected that most sensors will be comprised of one or more
 * probes; that a sensor is not always an active probe and a probe is
 * not always a sensor.
 *
 * By defining a passive sensor as a special kind of probe--a
 * "passive probe"--we can say that a sensor is always also probe.
 *
 * A Sensor is also one or more passive or active probes.
 *
 * At the prototype stage, a sensor will have a fixed array of
 * un-ininitialized probes, and those probes will be initiailized as
 * they are registered with the kernel.
 */

#define PROBES_PER_SENSE 128

/* prototype probe routine */
int (*probe)(uint64_t flags, uint8_t *buf) = NULL;
#define DEFAUT_PROBE_DATA 1024
struct probe_s {
	uint8_t *id;
	spinlock_t lock;
	uint64_t flags, timeout, repeat;
	int (*probe)(uint64_t, uint8_t *);
	struct list_head *l;
	uint8_t data[DEFAUT_PROBE_DATA];
};

uint8_t *register_probe(uint64_t flags,
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

struct kernel_sensor {
	uint8_t *id;
	spinlock_t lock;
	uint64_t flags;
	struct list_head *l;
	struct kthread_worker kworker;
	struct kthread_work kwork;
	struct probe_s probes[PROBES_PER_SENSE];
};

uint8_t *register_sensor(struct kernel_sensor *s);
int unregister_sensor(uint8_t *sensor_id);
uint8_t *list_sensors(uint8_t *filter);
