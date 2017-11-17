# Sensing Architecture
## Kernel, in-virtue controller, probes, sensors

### Controller
  - [ ] open domain socket, r/w messages in a persistent thread
  - [ ] init function initializes first work node, links it to the work list.
  - [ ] register probes with each having a timeout and repeat value; run probes as scheduled
  - [ ] parse jsonl defensively and handle all sensing API messages
  - [ ] kernel sensor handles all of the sensing API messages in JSON format and can be used as a driver for client API testing (more detail on the kernel sensor below.)

### Test Virtual World
  - [x] create fedora linux VM with kbuild and ksource
  - [ ] pull, build, load, and unload module as an automatic test
  - [ ] provide a place and means to store in-kernel test results for later analysis

### Kernel sensor

The in-kernel probe controller will serve also as a sensor for the kernel. It will run a number of internal probing functions that work together to form an opinion of the security state of the kernel.

  - [ ] fundamental probes.  

  These are the most basic self-checks the kernel may perform. timing, checksums, and verifying atomic truth assertions such as the existence of a file, or the bits in a page table entry.

  - [ ] tracing probe.

  A [stand-alone][1] probe that uses the kernel tracing infrastructure to monitor the execution of code located at specific addresses.

 - [ ] guest in(tro)spection probe (using libvmi)

 A stand-alone probe that has insight into the guest page tables and kernel symbols. This probe will be based upon libvmi, which provides informed access to important guest virtual machine symbols and memory contents.

[1] A kernel probe that is a kernel module and may be loaded separately from the kernel controller.

## Stage 2 coding parallelism
For planning purposes, assume that the Virtue team is adding two linux developers. What could three devs do in parallel working on the two six components of the VirtUE project? 

 - [ ] json message handling in both directions, kernel module to user space.
 - [ ] basic ontology definition and support in data store, predicate filtering
 - [ ] kernel sensor and probe development, libvmi.
