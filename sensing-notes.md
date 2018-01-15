
# Kernel Sensing Notes
## Definition of Kernel Sensing
### Kernel Sensing involves monitoring a target from wthin a kernel.  The monitoring target may be:
 
1. The kernel itself.
2. One or more applications being run by the kernel. 

### Kernel Sensing May occur from within two contexts:

1. From within a Virtue Kernel, such as a from within unikernel or from within a traditional kernel such as Linux.
   
         This includes a tradition model of monitoring a user-space executable from within a kernel; also it may include monitoring a process from within a unikernel, when that process may not have a separate memory space (for example a unikernel that shares a single memory space with the process it is running).  
         
             
2. From within a hypervisor, such as from within KVM or Xen, which is running the Virtue kernel in (1) within a virtual machine.
   
         This involves reaching into Virtue kernel memory from within the hypervisor. Today on cloud services this is a controversial policy because it sometimes violates the privacy of the virtual machine owner.  
         
        Sensing from within the hypervisor is made more difficult because the hypervisor may not know what kernel is running from within the virtual machine. 
        
        Libraries and tools exist to make sensing from within the hyperisor easier to write and maintain: 

		* <http://libvmi.com> 
	  
		  Sensing from within the hypervisor could be a single entry point for rootkit penetration of every virtual machine being run by that hypervisor.
		     
			At the same time, sensing from within the hypervisor can be a powerful vantagepoint for detecting attacks on virtual machines that they otherwise would be unable to detect themselves.
    
## Ideas for Kernel Sensing
1. From within the hypervisor, monitor changes to the Virtue Kernel’s memory.
	1. There are specific areas to target that should not change, or where change should be entirely predictable.
	2. From within the Virtue kernel, monitor executable images for changes that should not occur. (elf section changes, offset table poisoning, symbol offsets in memory, etc.)
	3. VMWare used to translate sections of executable kernels in-memory and cache those translations to change the normal execution of the kernel. This can be used both as an attack and as a preventative method.
1. From within the Virtue Kernel, monitor important data structures, including:
	2. Process list
	3. Open Files
	4. Executable file signatures at rest and within memory.
	4. All items from the “within the hypervisor” list apply as well from within the kernel, including binary executable images (which is elf format with all kernels we are considering for Virtue.)
5. Heuristic analysis, from within the hypervisor, from within the Virtue kernel, or both.
	1. When mapping a file into memory:
		* Determine ways of executing a file, such as through a bytecode engine or other virtual machine or runtime library
		* Monitor changes to a file when read by a Bytecode engine. Some will be predictable, others may indicate an attack such as translating or changing bytecodes, or invasive changes to the bytecode engine.
		*  Hook the opening of a file to check for specific known attacks such as GOT poisoning. 
 1. By scanning of file contents
	 2. look for "suspicious" file contents, as defined by a database of known malware characteristics, possibly aided by machine learning
	 3. Using error correction techniques to provide cross-validation (one file can verify another file - because we stored a signature of the union of both files). This is not a well-formed idea at this point.
	 4. An executable that uses the ptrace system call is probably a rootkit if it is not present on a list of know “good” debuggers. A rootkit does not want to allow itself to be reverse-engineered or otherwise inspected, and if it uses ptrace on itself it is probably doing so only to prevent another (p)tracer. This is a common way to prevent reverse engineering at runtime.

## Ideas for Further Applications

1. ptrace jailer 
	2. use PTRACE_SYSCALL to locate sysenter   

			sysenter is used in modern Linux kernels instead of interrupt 0x80 because it is much faster. This can be found in the linux-gate marked as vdso within the map file of a process. In grsec all of the base addresses of memory maps are blank, and therefore getting the address from that file is useless. Here is what we are looking for:
```assembly
fffe420 <__kernel_vsyscall>:
ffffe420: 51 push %ecx
ffffe421: 52 push %edx
ffffe422: 55 push %ebp
ffffe423: 89 e5 mov %esp,%ebp
ffffe425: 0f 34 sysenter
```  
			google "modern day elf runtime infection”  
  
  
2. use Git-like teck to generate and check signatures for a Virtue read-only or executable file system.
	3. Create snapshots of read-only and executable files for forensic purposes
	4. Monitor changes to exe and read-only file signatures to detect an attack on files-at-rest

3. Use an elf procedure linkage table as the model for a procedure patching interface. It would not add new functionality, only organize symbols and instructions more clearly and safely. Live patching of Virtues would be more straightforward. (as would live infecting)  

4. Avoid restarting infrastructure by using a live-patching interface to provide critical updates while the infrastructure is running.  

5. In virtue kernels, define the function prelude/prologue to have a function linkage table (like a .so) for future live updates. Leave room in the prelude/prologue for jumps/returns to and from updated code.

<https://lwn.net/Articles/734765/>   

	* lazy migration of functions
