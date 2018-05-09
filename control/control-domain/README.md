# Xen Dom0 Sensor

### Motivation: Why sense from Dom0?

Sensing from Dom0 provides several benefits:
* We can verify that DomU sensors are reporting accurate information
* We can monitor a DomU but hide the details of our monitor from malware resident in that domain
* We can set hook points in the DomU OS without the OS's knowledge. Thus, we can circumvent Microsoft's [Kernel Patch Protection - Wikipedia](https://en.wikipedia.org/wiki/Kernel_Patch_Protection)
* Using hypervisor support, we can effectively monitor DomU kernels from the Dom0 usermode, thus gaining kernel monitoring while foregoing some of its challenges

### Overview
Sensors in this realm run in Dom0 usermode. They will rely on the hypervisor (Xen) to detect events of interest and report or correlate them as directed. This appraoch is known as Virtual Machine Introspection (VMI) and has been a field of research for several years. See [Virtual Machine Introspection - Xen](https://wiki.xenproject.org/wiki/Virtual_Machine_Introspection). The field is relatively mature, at least inasfar as the building blocks are already in place.  As such, it is unlikely that we will need to interact directly with the primitives provided by Xen. However, for reference, those primitives are provided in these files:

* `/usr/include/xenguest.h`
* `/usr/include/xenctrl.h`
* `/usr/include/xen/xen.h`
* `/usr/include/xen/io/blkif.h`

These files provide Xen's VMI primitives, to include:

* Domain info, e.g. CPU, memory usage
* Domain context (register contents)
* Scheduler control
* Memory control
  * Mapping
  * Notification on access
  * Redirection (access of one physical page is forwarded to another)

### State of R&D

Intel's Virtualization Technology (VT) and Extended Page Table (EPT) technologies have made VMI faster and thus more practical against live targets. Two recent additions provide powerful capabilities to analysis: the `#VE` virtualization exception which allows for detection of an EPT permission violation, and the `vmfunc` instruction which allows for quick switching of a vCPU from one EPT to another.

Xen makes use of these technologies with its Hardware-assisted Virtual Machine (HVM) unprivileged domains ("DomU's"). Xen 4.6 and later support Intel's capability to support multiple EPTs in hardware, i.e. registration and fast switching between views of memory at the hypervisor level. This Xen feature is called "alternate physical to machine" (memory translation), or "altp2m". It only works with HVM DomU's.

Using this configuration (VMX + Xen + EPT), Xen's Dom0 can monitor a DomU's activities without modifying the DomU's OS. In fact, most, if not all, of the sensing techniques that might be used within a DomU can be implemented in the Dom0 monitor. Intel's recent hardware features can minimize the overhead of monitoring. 

Moreover, these hardware features enable seemless, covert virtual to physical page remapping underneath a DomU. This could be used in future mitigation techniques.

Existing research and development on the usage of VMI for security and monitoring purposes follows. It is sufficiently mature that new research or sensors need not start at ground level, as long as requisite technologies are enabled in the target environment.

* [DRAKVUF™ Black-box Binary Analysis System](https://drakvuf.com) - virtualization based agentless black-box binary analysis system
* [LibVMI](https://github.com/libvmi/libvmi)
* [Volatility: An advanced memory forensics framework](https://github.com/volatilityfoundation/volatility/)
* [The Bitdefender virtual machine introspection library is now on GitHub | Xen Project Blog](https://blog.xenproject.org/2015/08/04/the-bitdefender-virtual-machine-introspection-library-is-now-on-github/)
* [Virtual machine introspection: towards bridging the semantic gap | Journal of Cloud Computing | Full Text](https://journalofcloudcomputing.springeropen.com/articles/10.1186/s13677-014-0016-2)
* [Performance profiling of virtual machines](https://dl.acm.org/citation.cfm?id=1952686)
* [Research Overview: Virtualization-Based 
Malware Defense](https://www.csc2.ncsu.edu/faculty/xjiang4/csc501/readings/lec23-vm.pdf) - dated but relevant discussion on detection of malware from VMM
* [XPDS15 - Virtual Machine Introspection with Xen 0821](https://www.youtube.com/watch?v=k0BVFyyuvRA)
* [Protecting Critical Files Using Target-Based Virtual Machine Introspection Approach](https://www.jstage.jst.go.jp/article/transinf/E100.D/10/E100.D_2016INP0009/_article)

[LibVMI](https://github.com/libvmi/libvmi) provides primitives for virtual machine introspection, namely DomU virtual memory access and callbacks upon memory access and certain register modification. While it supports altp2m, it does not require it so it is suitable for paravirtualized DomU's.

 [Volatility](https://github.com/volatilityfoundation/volatility/) is a completely open collection of tools, implemented in Python, for the extraction of digital artifacts from RAM samples, thus offering visibility into the runtime state of the system. The framework is intended to introduce people to the techniques and complexities associated with extracting digital artifacts from volatile memory samples and provide a platform for further work into this exciting area of research.

[DRAKVUF™ ](https://drakvuf.com) is of particular interest for our sensor work. It is intended to analyze malware in a running VM, and already makes use of other VMI techniques: it is built on LibVMI and Volatility. Its core features and extensions focus on Windows, since that's where most malware has been historically. Augmenting these features for Linux should be a relatively light lift. Existing features include:
* Agentless start of binary execution (launch a process in the virtual machine from outside the VM).
* Agentless monitoring of Windows internal kernel functions (i.e. syscalls, disk writes, process launches).
* Guest multi-vCPU support.
* Tracing heap allocations.
* Tracing files being accessed.
* Extracting files from memory before they are deleted.
* Tracing UDP and TCP connections
* Cloning of analysis VMs via copy-on-write memory and disk.

[DRAKVUF™ ](https://drakvuf.com) requires Xen's altp2m, which in turn only works on HVM DomU's. It uses this feature to map one virtual memory page to different physical memory pages based on the type of access, to hide inserted breakpoints from malware and/or the guest kernel's read accesses. Without this feature, the hiding of memory alterations has an inherent race condition.

### Path Forward for Sensors

Given the open-source work already put into malware detection, which heavily overlaps with the hypervisor-based sensors that Two Six plans to develop, it makes the most sense to build on the corpus of available work rather than build it up again.

[DRAKVUF™ ](https://drakvuf.com) most closely matches the technological needs of the planned sensor work. However, it's notable that [DRAKVUF™ ](https://drakvuf.com) relies on features not available in standard Amazon EC2 instances, namely HVM and altp2m. We should investigate [Amazon EC2 Dedicated Hosts](https://aws.amazon.com/ec2/dedicated-hosts) to see whether the those features are available. If they are, we should use Dedicated Hosts to maximize the capabilities of our sensors while leveraging existing work.

If that's not feasible, two options are obvious:
* Implement sensors using [LibVMI](https://github.com/libvmi/libvmi). Features would have to be ported from [DRAKVUF™ ](https://drakvuf.com).
* Modify [DRAKVUF™ ](https://drakvuf.com) to run against paravirtualized DomUs. This would degrade the ability of the tool to hide itself.

Either of these options would require further investigation. 

### Notes / Conclusions

* Without [Amazon EC2 Dedicated Hosts](https://aws.amazon.com/ec2/dedicated-hosts) there is _no path forward_ to provide sensing underneath `Windows 10` VMs and there is a degraded path forward for `Linux` VMs.
* Leveraging  [DRAKVUF™ ](https://drakvuf.com) is the most direct and rapid path forward for hypervisor-based sensing. Other technologies can be leveraged but they don't coincide with the problem set as well.
* Testing should be done to verify that [Amazon EC2 Dedicated Hosts](https://aws.amazon.com/ec2/dedicated-hosts) can support [DRAKVUF™ ](https://drakvuf.com).
