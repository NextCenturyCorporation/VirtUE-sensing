Windows VirtUE Driver
Version 0.1.0.1

# Build Environment Requirements
0. Install latest git for windows
1. Install python 3.6.X ( latest version )
2. Install Visual Studio 2017 (either versions)
3. Install Win32 Software Developers Kit (SDK)
4. Install Latest Windows Driver Developers Kit (DDK)


# How To Build and Run
0. Open the WinVirtUE.sln soluation file, select either Debug or Release profiles and rebuild.
1. Copy/Paste the x64\$(Configuration)\WinVirtUE directory to the VM's desktop
2. Right click and select install the WinVirtUE.inf file
3. Open a administrators command prompt and execute the following command:
```Cmd
sc start WinVirtUE
```
4. Verify that the driver is loaded and running by executing these two commands:
```Cmd
C:\> driverquery
...

C:\> sc queryex WinVirtUE
...
```
Output of driverquery should show at the last position that the driver is loaded.  The output of the sc queryex command should show that the service started.

# How to Verify Comms
0. Follow the instructions above to create a windows kernel driver installation and install it
1. Copy the ntfltmgr.py file located at https://github.com/twosixlabs/savior/blob/enh-windows-kernel-user-comms/sensors/ntfltmgr/ntfltmgr.py to the target where the driver was installed
2. Ensure that the target instance has python 3.6.4 or greater installed.  
3. Execute the ntfltmgr.py file by
```Cmd
python .\ntfltmgr.py
```
The output from the driver (assuming the debugginer is attached and active) should show the driver connecting to the 
python program, continuously sending data and waiting for a response, and then finally disconnecting from the python
program when it exits.

#  Kernel Comms Design Notes
## Design Approach
0. Utilizes the Filter Manager Communications Port to simplify kernel to user space communications
1. Code such that it simplifies the nominal Inversion of Control approach using the DeviceIoControl function call interface
2. User space program needs only utilize a simple open, read/dispatch, write/reply, close model to process kernel sourced commands.
3. The kernel space driver will simply insert data into a interlocked queue. A separate service kernel thread will drain the queue transmitting the queued data to the listening user space program.
4. No polling is required on either end.  All operations are event driven with specific state transitions to handle open/close/read and write operations as required.
## Why was it done this way?
One of the more complicated and confusing aspects of sending data from the kernel to a waiting user space program (service, application, etc) is nominal approach of sending data from the kernel to user space utlizing an 'Inversion of Control' (IOC) approach and the DeviceIoControl IRP.  This approach is a simplified higher-level abstraction of this communications mechanism.  Furthermore, the function FilterGetMessage on the user space side includes a simple async I/O approach.  When implemented, this mechansim should be compatable (subject to additional research) with the curio/async python design that we've utlizied thus far.  In practice, it is hoped that each call to FilterGetMessage will immediately return so that curio queue processing will continue.  All message retrieval I/O and protocol response will take place on some arbitrary thread pool thread.  This non-blocking approach should (hopefully) work well with the curio and other python async I/O libraries.

