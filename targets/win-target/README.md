# Windows Sensors - Windows User Space Sensor Installation and Run Instructions

# Bootstrap a Windows 2016 Server AWS Instance
1) With an AWS instance in an RDP window ready to boot strap open the [savior\bin\bootstrap.ps1](../../bin/bootstrap.ps1) in this repo and **follow the instructions at the top of the `bootstrap.ps1` file** from an **elevated command prompt**. To launch a command prompt with admin rights, type `cmd` in the Windows Start menu, then either right click and select `Run as Administrator` or you can press `ctrl/command+shift+enter`.
2) Executing this powershell file will install git, required for installing the rest of the system.

# Building Windows Sensors From Scratch
1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows installed.

2. Install the Sensor Driver
Check the [Releases Page](https://github.com/twosixlabs/savior/releases) and scroll down to find the Windows Driver download. Download the zip file and extract it, then `right click` on `WinVirtUE.inf` and select `Install`. 

3. From an elevated command prompt in the \savior folder, run the windows build batch to install Python and some necessary build tools. Running this script will also put Windows into test mode, allowing the sensor driver to load at boot time.
```Cmd
bin\windows-build.bat
```
4. from the \savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd
python bin\install_sensors.py
```

5. Reboot the machine to allow the driver to load, then from the \savior folder run to start the Sensor Service (will appear in `services.msc` as Windows Winvirtue Service. Note that starting the Sensor Service **requires** the Sensor Driver to be both installed and started. You can check the status of the driver by running `sc query winvirtue` at a command prompt.
```Cmd
bin\windows-update.bat
```

6. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least one module to compile required native code.
7. Python 3.6.5 will be installed next, and you may need to click through some of the UAC and python installation menu prompts.
8. Python requirements will be installed after the the python installer exits.  There is at least one required python package that needs the VS build enviornment, notably the http package.
9. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.

# Updating Sensors (if required)
1. Stop all sensors by stopping the Winvirtue Service, either from `services.msc` or killing it from Task Manager.
2. From the \savior directory, pull all changes
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

5. Process List Validation
