# Using eBPF and the BCC tools to Monitor Linux kernels

There are several toolsets for accessing Linux kernel kprobes, a good summary is [Linux Tracing Systems](https://jvns.ca/blog/2017/07/05/linux-tracing-systems/).

My reccomendation is to use the [BCC](https://github.com/iovisor/bcc) frontend to the kernel [eBPF tracing interface](https://lwn.net/Articles/740157/). This is based upon the following reasons:

* the large number of pre-built tools providing access to the entire range of kernel instrumentation
* the use of Python as a programming platform, while embedding C code to program the eBPF.
* the active development and support of the BCC toolset tracks the equally active development of eBPF in the Linux kernel.

My experience with BCC and eBPF has been positive. The greatest challenge is to build a recent version of the BCC tools, newer than what is included with most Linux distributions. This entailed building the LLVM compiler from scratch. From beginning to end this took me less than an hour for fedora 28.

I used eBPF to do an analysis of sheduling performance for Amazon c5 instances as they ran up to 16000 virtual machines. I was able to quickly create histograms of scheduling response as load increased. There were more than a hundred BCC tools I could have used to further explore performance and to analyze data.
### Kernel requirements

All Linux distribution kernels I've used in the past several years have kprobes built into the kernel. However, they do not have kprobes activated by default. Activation requires mounting the kernel debug filesystem, which makes kprobes available via a sysfs-like interface. Although this filesystem is called "debug" it is more accurately described as "instrumentaition."

Once mounting this filesystem, the BCC tools "just work." There are literally hundreds of ready probes with built in data analysis tools.
