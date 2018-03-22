# Xen Kernel Sensing
## Why Probe the Xen Kernel?
Probing from within the Xen kernel can provide information that may act as a _canary_ when compared to similar information obtained from probes running in a privileged Xen Domain kernel. It may indicate a compromised Domain 0, for example.
While most probing may be more efficiently done via privileged domain libraries, a Xen kernel probing capability is beneficial while also being relatively minimal.
## General Approach
Xen kernel sensing will share the same approach and much of the same code as linux kernel sensing. That is, native kernel code accessing native kernel data structures and symbols.
> Much of the Xen kernel code is directly copied from or derived from the Linux 2.6 kernel, although some components including the scheduler are unique to Xen
> Aside from specific types of linux probes (e.g., _lsof_), probably 85% of the code will be shared between the linux kernel sensor and Xen sensor source trees.

### Similarities
1. A sensor runs as a (Xen) kernel thread and manages one or more _probes_. The sensor (parent) and probes (children) have a _parent-child relationship._
2. Each probe provides a specific type of instrumentation and also runs as a separate thread, but is subject to control by the sensor thread.

	3. starting, stoping, rescheduling, changing the probe level, retrieving records, and so on according to the JSON control protocol.
3. The sensor provides communication between the probes and user space using the JSONL kernel sensing message protocol.

	4. See _messages.md_
	4. Supporting the same request commands and reply responses, which as of now is:
	
``static uint8_t *table[] = {
		"discovery",
		"off",
		"on",
		"increase",
		"decrease",
		"low",
		"default",
		"high",
		"adversarial",
		"reset",
		"records"
	};``
	
4. The user space interface on both platforms will be script-enabled via the JSONL format of the command-response protocol. Pushing a JSONL string into the kernel via either a  domain socket (linux) or hypercall (Xen) will emit a string JSONL response object.

### Differences
1. The Sensor and probes will be _built-in_ to the Xen kernel, rather than built seperately as loadable modules.
2. The set of probes to run in the Xen kernel will be different from linux and limited to Xen kernel resources.
3. Sensor and probe commands and responses will be achieved via a new _privileged hypercall_, built in to the Xen Kernel. Usually this means only _Dom 0_ processes may invoke the sensing protocol, but it may be invoked also from a separate privileged domain, the sole purpose of which is to support kernel sensing.
> A linux System Call and a Xen Privileged hypercall are analogous and implemented (mostly) the same way on each platform.
