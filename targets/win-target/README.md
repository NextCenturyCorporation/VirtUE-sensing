# Windows Sensors - Windows User Space Sensor Installation and Run Instructions

# Bootstrap a Windows 2016 Server AWS Instance
1) With an AWS instance in an RDP window ready to boot strap open the [savior\bin\bootstrap.ps1](../../bin/bootstrap.ps1) in this repo and **follow the instructions at the top of the `bootstrap.ps1` file.**
2) Executing this powershell file will install git which is required for installing the rest of the system.

# Building Windows Sensors From Scratch
1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows installed.

2. from the \savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd

python bin\install_sensors.py
```
4. Installing the Sensor Driver
This process requires installing an unsigned driver in windows, necessitating several UAC prompts and a reboot.

Unzip the <filename> in a convenient directory, right click on the `WinVirtUE.inf` file and select `Install`.

5. From the same savior directory execute windows build batch file:
```Cmd
bin\windows-build.bat
```
6. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least module to compile required native code.
7. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
8. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
9. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.

# Updating Sensors (if required)
1. Stop all sensors by killing all their running windows
2. From the .\savior directory, pull all changes
```Cmd
git pull -v
```
3. Execute the windows update script
```Cmd
bin\windows-update.bat
```


# Basic Windows Sensors
1. Process Creation and Destruction.  

2. Module load (and optional signature analysis)

3. Threads creation and destruction

4. Registry Modification

5. Security Log
