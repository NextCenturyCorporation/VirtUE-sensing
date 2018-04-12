Windows Sensors - Windows User Space Sensor Installation and Run Instructions
Mark Sanderson
mark.sanderson@twosixlabs.com

``` NOTE:
In order to run the sensors from a pre-built AWS image, execute bin\run-all.bat from the c:\users\administrator\savior directory
```

# Differences And Caveats
## Docker Issues
For now, Docker *will not* be used to containerize the Windows Sensors.  The Windows 10 Docker product is still fairly immature, and it is not clear if we'll be able to take advantage of it's functionality in the future.  There were a number of issues including random virtual network malfunctions and etc that prevented Docker from being useful.
## Cert Directory
The certs directory keys are now populated properly using RSA from pycryptodome library. 
## curio
curio 0.9 appears to be comptable (at least partially) with windows.  This will be proven out as time goes on.

# Building and running the Windows Dummy Sensor

1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows and python 2.7 installed.
2. Log on to the windows 10 target, and clone this respository.
```Cmd
git checkout master
```
3. from the savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd
bin\install_sensors.py
```
5. From the same savior directory execute windows build batch file:
```Cmd
bin\windows-build.bat
```
5. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least module to compile required native code.
6. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
7. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
8. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.
9. Finally, execute all the of the sensors by running bin\run-all.bat  All sensors will appear as minimized windows.
