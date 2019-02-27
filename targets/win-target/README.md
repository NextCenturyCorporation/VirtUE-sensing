# Windows Sensors - Windows User Space Sensor Installation and Run Instructions

## Commandline Installation
1. Spin up an AWS instance using the `ami-05d864f01373c854a` AMI, this is a clean Windows 10 machine with SSH Host installed using the `vrtu` key for access.

2. Connect to the new instance via
`ssh -i <path/to/vrtu> virtue-admin@<ipaddress>`

3. Once connected, runthe following commands to install git and python. Note that the later batch files **depend** on Python 3.6.5 being located in `c:\Python 3.6.5`.

```cmd
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
refreshenv
choco install git -y
choco install python3 --version 3.6.5 --install-arguments="TargetDir=C:\Python3.6.5 InstallAllUsers=1 PrependPath=1 CompileAll=1" --force -y
refreshenv
```

4. Download this repo to the virtue-admin directory
```cmd
cd %userprofile%
git clone https://twosix-savior:6c87282451a82996c229c068ca34cea8b5b9088b@github.com/twosixlabs/savior.git
```

5. Enter the /savior subdirectory and run
```Cmd
cd ./savior
python bin\install_sensors.py
```

6. Execute the windows build batch
```Cmd
bin\windows-build.bat
```

7. Next, our Windows Sensor Driver must be installed. Copy `Savior-Win-Driver-20181205-02.zip` (or the latest version) to the machine via `scp`, then RDP to the machine, unzip the file, enter the folder and right click on the WinVirtUE.inf and select `Install`. Windows will warn it is an unsigned driver, accept and allow it to install, then reboot the machine.

9. After rebooting, execute the windows update batch
```Cmd
bin\windows-update.bat
```

10. Verify the driver and sensor are active, either with the following command line queries, or by checking `services.msc` and looking for `Windows Winvirtue Service` in the list. The service should be `Started` and the driver should be loaded, the service cannot start without the driver present.
```Cmd
scquery winvirtue
scquery "WinVirtUe Service"
```

# Bootstrap a Windows 2016 Server AWS Instance
1) With an AWS instance in an RDP window ready, open `savior/bin/windows-prep.bat` and follow the instructions to execute it.
2) Executing this batch file will install git and python 3.6.x, and clone this repo, required steps for installing the rest of the system.

# Building Windows Sensors From Scratch
1. Ensure that you have a virtual machine running Windows 10 x64 with git for windows and python 3.6.x installed.
2. Log on to the windows 10 target, and clone this respository.
```Cmd
git checkout master
```
3. from the savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd
c:\Python-2.7.14\python.exe bin\install_sensors.py
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
