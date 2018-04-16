Windows Sensors - Windows User Space Sensor Installation and Run Instructions
Mark Sanderson
mark.sanderson@twosixlabs.com

# Differences And Caveats

## Docker Issues
For now, Docker *will not* be used to containerize the Windows Sensors.  The Windows 10 Docker product is still fairly immature, and it is not clear if we'll be able to take advantage of it's functionality in the future.  There were a number of issues including random virtual network malfunctions and etc that prevented Docker from being useful.

## curio
curio 0.9 appears to be comptable (at least partially) with windows.  This will be proven out as time goes on.

# Bootstrap a Windows 2016 Server AWS Instance
1) With an AWS instance in an RDP window ready to boot strap open the .\savior\bin\bootstrap.ps1 and follow the instructions at the top of the file.
2) Executing this powershell file will install git and python 2.7.x which is required for installing the rest of the system.

# Building Windows Sensors From Scratch
1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows and python 2.7 installed.
2. Log on to the windows 10 target, and clone this respository.
```Cmd
git checkout master
```
3. from the savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd
c:\Python-2.7.14\python.exe bin\install_sensors.py
```
4. From the same savior directory execute windows build batch file:
```Cmd
bin\windows-build.bat
```
5. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least module to compile required native code.
6. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
7. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
8. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.

# Updating Sensors
1. Stop all sensors by killing all their running windows
2. From the .\savior directory, pull all changes
```Cmd
git pull -v
```
3. Execute the windows update script
```Cmd
bin\windows-update.bat
```
# Running All Sensors
1. Ensure that no sensors are running.  
2. Run all sensors from the command line:
```Cmd
bin\run-all.bat
```
3. All three sensors, processlist, tasklist and handlelist sensors should start in their own minimized window.
