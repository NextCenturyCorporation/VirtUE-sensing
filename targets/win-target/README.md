
Windows Sensors - Dummy Sensor Installation and Run Instructions
Mark Sanderson
mark.sanderson@twosixlabs.com

# Differences And Caveats
## Docker Issues
For now, Docker *will not* be used to containerize the Windows Sensors.  The Windows 10 Docker product is still fairly immature, and it is not clear if we'll be able to take advantage of it's functionality in the future.  There were a number of issues including random virtual network malfunctions and etc that prevented Docker from being useful.
## Cert Directory
The Cert directores keys are populated statically from a Linux version of of openssl.  I could not find an 'official' version of openssl running on Windows 10 that would immediately meet my needs.  I will utilize other mechanisms to dynamically generate these keys at a later stage in the development process.
## curio
The curio.subprocess.Popen function fail to run with an attribute error indicating that the setblocking function is missing.  This is somewhat expected as the select/poll model that curio uses is not fully compatable with how windows expects things.  Obviously, some kind of work around must be put in place.

# Building and running the Windows Dummy Sensor

1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows installed.
2. Log on to the windows 10 target, and clone this respository.
3. Switch the branch to 'enh-create-win10-dummy-sensor':
```Cmd
git checkout enh-create-win10-dummy-sensor
```
4. from the savior\targets\win-target subdirectory on the virtual machine execute the build batch file:
```Cmd
.\build.bat
```
5. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least module to compile required native code.
6. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
7. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
8. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.
9. The last notable step to occur is the executable of the dummy sensor.  This sensor is based (loosely) on the lsof dummy sensor.  It does not function, it merely starts up, reports that there is a configuration error and attempts repeatedly to connect to api services.
10. After spending 30 seconds or so failing to connect to the API, the dummy sensor will terminate.

# Addition Notes About The curio Python Package
The curio package utilizes the select.select function within its kernel subsystem.  The in-kernel task co-routine **appears** to utilize select with 3 empty read/write/exception file descriptors.  This is not supported on windows, as per the 3.6.4 documentation hence:

'''
select.select(rlist, wlist, xlist[, timeout])
This is a straightforward interface to the Unix select() system call. The first three arguments are sequences of ‘waitable objects’: either integers representing file descriptors or objects with a parameterless method named fileno() returning such an integer:

rlist: wait until ready for reading
wlist: wait until ready for writing
xlist: wait for an “exceptional condition” (see the manual page for what your system considers such a condition)
Empty sequences are allowed, but acceptance of three empty sequences is platform-dependent. (It is known to work on Unix but not on Windows.) The optional timeout argument specifies a time-out as a floating point number in seconds. When the timeout argument is omitted the function blocks until at least one file descriptor is ready. A time-out value of zero specifies a poll and never blocks.
'''

Further information can be derived from the Windows select https://msdn.microsoft.com/en-us/library/ms740141(VS.85).aspx Note that the error 'WSAEINVAL' states it occurs when "The time-out value is not valid, or all three descriptor parameters were null."  Of course, calling select with 3 descriptor as empty is completely valid in the Linux/Unix Operating Systems.

We'll need to come up with some kind of approach to either eliminate curio from the wrapper or modify curio to work with windows select style I/O.
