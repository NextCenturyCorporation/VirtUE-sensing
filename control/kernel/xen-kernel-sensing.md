# Xen Kernel Sensing
## Why Probe the Xen Kernel?
Probing from within the Xen kernel can provide information that may act as a _canary_ when compared to similar information obtained from probes running in a privileged Xen Domain kernel. It may indicate a compromised Domain 0, for example.
While most probing may be more efficiently done via privileged domain libraries, a Xen kernel probing capability is beneficial while also being relatively minimal.
### Watching the Watchers
Most Xen guest debuggers and rootkits will make changes to a guest's vmx or svm structure in the Xen Kernel. _libvmi_ and others will change items in the vmx or svm structure such as the page directory pointer tables, and the page directories themselves. Xen's exception handling tables are also a target of both debuggers and rootkits. They will make these changes using privileged hypercalls (usually from dom0).

It is impossible for the Xen kernel to know the difference between an introspection library and a rootkit. However, in-kernel probes can record information and emit records of these changes to a trusted evaluator via an encrypted hypercall channel. The difference between the expected behavior of a debugger or introspection library, and the actual behavior, can be diagnostic of a rootkit or compromised dom0.
## General Approach
Xen kernel sensing will share the same approach and much of the same code as linux kernel sensing. That is, native kernel code accessing native kernel data structures and symbols.
> Much of the Xen kernel code is directly copied from or derived from the Linux 2.6 kernel, although some components including the scheduler are unique to Xen
> Aside from specific types of linux probes (e.g., _lsof_), probably 85% of the code will be shared between the linux kernel sensor and Xen sensor source trees.

### Similarities
1. A sensor runs as a (Xen) kernel thread and manages one or more _probes_. The sensor (parent) and probes (children) have a _parent-child relationship._
2. Each probe provides a specific type of instrumentation and also runs as a separate thread, but is subject to control by the sensor thread.

	3. starting, stoping, rescheduling, changing the probe level, retrieving records, and so on according to the JSON control protocol.
3. The sensor provides communication between the probes and user space using the JSONL kernel sensing message protocol.

	4. See [https://github.com/twosixlabs/savior/blob/master/control/kernel/messages.md](_messages.md_)
	4. Supporting the same request commands and reply responses, which as of now is:

`static uint8_t *table[] = {
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
	};`

4. The user space interface on both platforms will be script-enabled via the JSONL format of the command-response protocol. Pushing a JSONL string into the kernel via either a  domain socket (linux) or hypercall (Xen) will emit a string JSONL response object.

5. Both kernels will use TLS to engage in mutually encrypted communications. Both the Linux and Xen kernels will use the TLS protocol that is currently upstream in the linux kernel. <https://elixir.bootlin.com/linux/v4.16-rc7/source/net/tls/tls_main.c>. This implementation is mostly self-contained and the linux-specific code is all located in the socket options for TLS. Instead of socket options, Xen will use a new hypercall to exchange certificates and begin a mutually encrypted session.

### Differences
1. The Sensor and probes will be _built-in_ to the Xen kernel, rather than built seperately as loadable modules.
2. The set of probes to run in the Xen kernel will be different from linux and limited to Xen kernel resources.
3. Sensor and probe commands and responses will be achieved via a new _privileged hypercall_, built in to the Xen Kernel. Usually this means only _Dom 0_ processes may invoke the sensing protocol, but it may be invoked also from a separate privileged domain, the sole purpose of which is to support kernel sensing.
> A linux System Call and a Xen Privileged hypercall are analogous and implemented (mostly) the same way on each platform.

### Probes for the Xen Kernel

1. _kernel-ps_, which presents a _ps command_ view from within the kernel of running processes or threads. In the Xen case, the output will correspond to running Xen domains and Xen kernel-internal threads. It may be useful to find hidden or obscured running Domains, or may find visible domains with unexpected privileges.
2. checksums on the contents of important Xen symbols, which could detect the installation of trampolines (_jumps_) within the Xen kernel.
3. Changes to Xen kernel page tables. page table entries (_ptes_) will change frequently, but changes to Xen kernel first-level page tables will be predictable and associated with specific events. A probe can detect unpredicted changes to the first-level page table.
4. Changes to page directory pointers in Xen' HVM _vmx_ or _svm_ structures, which may indicate that the guest is being monitored by a dom0 component.
5.  Chages to Xen kernel exception handlers. Xen exception handling should not change during runtime, and detecting changes should be relatively simple for an in-kernel probe.
