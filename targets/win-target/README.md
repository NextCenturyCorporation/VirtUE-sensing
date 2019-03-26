# Windows Sensors - Windows User Space Sensor Installation and Run Instructions

## Commandline Installation
1. Spin up an AWS m4.large instance using the `ami-05d864f01373c854a` AMI, this is a clean Windows 10 machine with SSH Host installed using the `vrtu` key for access.

2. Connect to the new instance via
`ssh -i <PATH/TO/vrtu> virtue-admin@<ipaddress>`

3. Once connected, run the following commands to install git and python. Note that the later batch files **depend** on Python 3.6.5 being located in `c:\Python3.6.5`.

```cmd
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"```
```
```cmd
refreshenv
choco install git -y
```
```
choco install python3 --version 3.6.5 --install-arguments="TargetDir=C:\Python3.6.5 InstallAllUsers=1 PrependPath=1 CompileAll=1" --force -y
```
```
refreshenv
```

4. Download this repo to the virtue-admin directory
```cmd
cd %userprofile%
git clone https://twosix-savior:6c87282451a82996c229c068ca34cea8b5b9088b@github.com/twosixlabs/savior.git
```

5. Enter the /savior subdirectory and run the python sensor install script
```Cmd
cd savior
python bin\install_sensors.py
```

6. Execute the windows build batch to install necessary Visual Studio components, build the sensors, and configure the Winvirtue service to autostart
```Cmd
bin\windows-build.bat
```

7. Install the Windows Sensor Driver. The latest file is located on the [Releases Page](https://github.com/twosixlabs/savior/releases), scroll down to find the Windows Driver download. You can download v1.0-rc1-win via the following curl command. Note that because the driver is unsigned, it cannot be installed via commandline and must be done via RDP (step 8).
```Cmd
curl -O -J -L -H "Accept: application/octet-stream"  https://6c87282451a82996c229c068ca34cea8b5b9088b@api.github.com/repos/twosixlabs/savior/releases/assets/11033641
```

8. RDP into the Windows system, extract the zip file, enter the folder, right click on WinVirtUE.inf and select `Install`, then Yes to the prompt to allow the driver to install. **Reboot** the Windows system.

9. Verify the driver and sensor are active, either with the following command line queries, or by checking `services.msc` via RDP and looking for `Windows Virtue Service` in the list. The service should be `Started` and the driver should be loaded, the service cannot start without the driver present.
```Cmd
sc query winvirtue
sc query "winvirtue service"
```
Output should be
```
virtue-admin@DESKTOP-IO0F0I9 C:\Users\virtue-admin>sc query winvirtue

SERVICE_NAME: winvirtue
        TYPE               : 2  FILE_SYSTEM_DRIVER
        STATE              : 4  RUNNING
                                (STOPPABLE, NOT_PAUSABLE, IGNORES_SHUTDOWN)
        WIN32_EXIT_CODE    : 0  (0x0)
        SERVICE_EXIT_CODE  : 0  (0x0)
        CHECKPOINT         : 0x0
        WAIT_HINT          : 0x0

virtue-admin@DESKTOP-IO0F0I9 C:\Users\virtue-admin>sc query "winvirtue service"

SERVICE_NAME: winvirtue service
        TYPE               : 10  WIN32_OWN_PROCESS
        STATE              : 4  RUNNING
                                (STOPPABLE, NOT_PAUSABLE, ACCEPTS_SHUTDOWN)
        WIN32_EXIT_CODE    : 0  (0x0)
        SERVICE_EXIT_CODE  : 0  (0x0)
        CHECKPOINT         : 0x0
        WAIT_HINT          : 0x0
```

# Updating Sensors (if required)
1. Stop all sensors by killing the pythonservices.exe running winvirtue service (Task Manager, Advanced, Processes, pythonservices.exe, End Task)
2. From the .\savior directory, pull all changes
=======
# Windows User Space Sensor Installation and Run Instructions

## Bootstrap a Windows 2016 Server AWS Instance
1) With an AWS instance in an RDP window ready to boot strap open the [savior\bin\bootstrap.ps1](../../bin/bootstrap.ps1) in this repo and **follow the instructions at the top of the `bootstrap.ps1` file** from a command prompt.
2) Executing this powershell file will install git, required for installing the rest of the system. You will need to accept windows install prompts and log into git to clone the savior repo.

## Building Windows Sensors From Scratch
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

5. Reboot the machine to allow the driver to load, then from the \savior folder run `windows-update.bat` to start the Sensor Service (will appear in `services.msc` as Windows Winvirtue Service). Note that starting the Sensor Service **requires** the Sensor Driver to be both installed and started. You can check the status of the driver by running `sc query winvirtue` at a command prompt.
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


## Windows Sensors Monitor:
1. Process Creation and Destruction.  

2. Module load (and optional signature analysis)

3. Threads creation and destruction

4. Registry Modification

5. Process List Validation
