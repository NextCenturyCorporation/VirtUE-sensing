# Sensing Architecture
## Kernel, in-virtue controller, probes, sensors

### Controller
  - [ ] open domain socket, r/w messages in a persistent thread
  - [ ] register probes with each timeout/repeat, run probes as scheduled
  - [ ] parse jsonl defensively and handle all sensing API messages

### Test Virtual World
  - [x] create fedora linux VM with kbuild and ksource
  - [ ] pull, build, load, and unload module as an automatic test
  - [ ] provide a place and means to store in-kernel test results for later analysis

### Kernel sensor

The in-kernel probe controller will serve also as a sensor for the kernel. It will run a number of internal probing functions that work together to form an opinion of the security state of the kernel.

  - [ ] fundamental probes.  

  These are the most basic self-checks the kernel may perform. timing, checksums, and verifying atomic truth assertions such as the existence of a file, or the bits in a page table entry.

  - [ ] tracing probe.

  A stand-alone probe that uses the kernel tracing infrastructure to monitor the execution of code located at specific addresses
