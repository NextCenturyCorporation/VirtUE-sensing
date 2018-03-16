
Windows Sensors - Dummy Sensor Installation and Run Instructions
Mark Sanderson
mark.sanderson@twosixlabs.com

# For now, Docker will not be used to containerize the Windows Sensors.  This product is still fairly immature.   There were a number of issues including randomvirtual network malfunction and etc that prevented Docker from being useful.

# Building and running the Windows Dummy Sensor

1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows installed.
2. Log on to the windows 10 target, and clone this respository.
3. Switch the branch to 'enh-create-win10-dummy-sensor':
'''Cmd
"git checkout enh-create-win10-dummy-sensor"
4. from the savior\targets\win-target subdirectory on the virtual machine execute the build batch file:
'''Cmd
".\build.bat"
5. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.
6. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
7. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
8. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.
9. The last notable step to occur is the executable of the dummy sensor.  This sensor is based (loosely) on the lsof dummy sensor.  It does not function, it merely starts up, reports that there is a configuration error and attempts repeatedly to connect to api services.
10. After spending 30 seconds or so failing to connect to the API, the dummy sensor will terminate.
