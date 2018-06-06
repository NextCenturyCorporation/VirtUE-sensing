Windows VirtUE Driver
Version 0.1.0.1

# Build Environment Requirements
0. Install latest git for windows
1. Install python 3.6.X ( latest version )
2. Install Visual Studio 2017 (either versions)
3. Install Win32 Software Developers Kit (SDK)
4. Install Latest Windows Driver Developers Kit (DDK)


# How To Build and Run
<<<<<<< HEAD
0. Open the WinVirtUE.sln solution file, select either Debug or Release profiles and rebuild.
=======
0. Open the WinVirtUE.sln soluation file, select either Debug or Release profiles and rebuild.  Debug is recommended for the purposes herein.  
0.5 Optional:  Look at the savior/control/win-kernel/WinVirtUE/config.h file and locate the LOG_MODULE definition.  Configure that so that only those subsystems you wish to view activity on will output debug/status information to the debugger console - recompile.
>>>>>>> master
1. Copy/Paste the x64\$(Configuration)\WinVirtUE directory to the VM's desktop
2. Right click and select install the WinVirtUE.inf file
3. Open a administrators command prompt and execute the following command:
```Cmd
sc start WinVirtUE
```
The driver, if compiled in debug mode ( recommended ) will stop at an embedded debug command.  At this point enter the following command into the windbg.exe command windows:
``` windbg.exe
ed kd_default_mask 0xff
```
This command will enable all detail levels for the debugger and provide the most verbose ouput.  Press the F5 button to continue past these points.

4. Verify that the driver is loaded and running by executing these two commands:
```Cmd
C:\> driverquery
...

C:\> sc queryex WinVirtUE
...
```
Output of driverquery should show at the last position that the driver is loaded.  The output of the sc queryex command should show that the service started.


# Kernel Sensor to User Comms

## How it Works
Basically the probe starts collecting data by activating specific system callbacks; during the callback context the probe en-queues the collected data onto an interlocked queue and returns.  The consumer side is woken up  (if it's connected) and then transmits it to the listening user space program.  If there is no connection, then the queue is treated like a circular queue only keeping the last 'n'  (currently 8192) elements.  The user  space program (python in this example) unpacks the data, converts it and then simply prints it to standard out.  When the user space program sends it's response, the entry de-queued by the kernel consumer thread is freed.  If no response occurs within the time-out period, the entry is pushed back on to the head of the queue and a wait ensues.  The cycle is repeated until tear-down.

## How to run it
1. Build, install and run the driver as mentioned above.  For clarity and ease, bulid the driver and load it with windbg attached so that various operations can be used.  This can be done in a variety of different ways, I recommand using VMware and VirtualKD 3.0 and later.
2. Ensure that the target instance has python 3.6.4 installed. Earlier versions will probably not work, later versions might work.  I've only tested with 3.6.4. 
3. Execute the ntfltmgr.py file by
```Cmd
python .\ntfltmgr.py
```
4. The python program itself will output in json format the probe data as collected by the WinVirtUE.sys driver.  Currently this data is confined to loaded module data probe.  The output from the driver (assuming the debugger is attached and active) should show the driver connecting to the python program, continuously sending data and waiting for a response, and then finally disconnecting from the python
program when it exits.

#  Kernel Comms Design Notes
## Design Approach
0. Utilizes the Filter Manager Communications Port to simplify kernel to user space communications
1. Code such that it simplifies the nominal Inversion of Control approach using the DeviceIoControl function call interface
2. User space program needs only utilize a simple open, read/dispatch, write/reply, close model to process kernel sourced commands.
3. The kernel space driver will simply insert data into a interlocked queue. A separate service kernel thread will drain the queue transmitting the queued data to the listening user space program.
4. No polling is required on either end.  All operations are event driven with specific state transitions to handle open/close/read and write operations as required.
## Why was it done this way?
One of the more complicated and confusing aspects of sending data from the kernel to a waiting user space program (service, application, etc) is nominal approach of sending data from the kernel to user space utilizing an 'Inversion of Control' (IOC) approach and the DeviceIoControl IRP.  This approach is a simplified higher-level abstraction of this communications mechanism.  Furthermore, the function FilterGetMessage on the user space side includes a simple async I/O approach.  When implemented, this mechanism should be compatible (subject to additional research) with the curio/async python design that we've utilized thus far.  In practice, it is hoped that each call to FilterGetMessage will immediately return so that curio queue processing will continue.  All message retrieval I/O and protocol response will take place on some arbitrary thread pool thread.  This non-blocking approach should (hopefully) work well with the curio and other python async I/O libraries.

# Kernel Probes
## Design Approach
0. The probes are written in C++ to make it more convenient to manage common functionality
1. All probes are derived from the AbstractVirtueProbe class.  This abstract class requires that all probes define Enable(), Disable(), State(), and Mitigate as instance methods.  Currently, the Mitigate function is unused and is there only as a placeholder
2. The serialization mechanism utilized is Windows queued semaphores that are optimized for high contention operations.  
3. The ProbeDataQueue operates very similar to a circular buffer queue in that once the maximum number of entries are exceeded, the oldest queue entry is discarded.  
4. The ProbeDataQueue interacts with Probe classes (ImageLoadProbe, ProcessCreateProbe) which act as queues producers.  The queues consumer is a kernel thread that waits for connection and queue entries.  When the queue is not empty and there is a connection, the filter sends probe data to the listening user space service or program.
5. Upon receiving the probe data, the user space program first unpacks the header, replies with the proper response, and then decodes the remainder of the probe data.  The decoded data is then converted into a python namedtuple and then finally into json data consumable by the Savior API.
##  Why was it done this way?
Although more up-front work is required, this approach I believe will (and does) simplify writing new probes and transmitting data to user space.  By creating a a simple abstract class, probe development should be pretty straightforward.  Each probes data type that it transmits will have it's first field of type PROBE_DATA_HEADER.  This ensure that the required data is in place for the Filter Communications subsystem.  These same data structures will be created on the python side utilizing the SaviorStucture base class, and a required implementation of the build method.
## How to make it work
0. Follow the instructions in the 'How to Verify Comms' section above.
1. When the ntfltmgr.py file is execute, you should view image load, process creation and destruction events.

## Known Issues
* Stopping and restarting the user level program might not result in a valid connection.  This bug will be written up and assigned in a future milestone.

