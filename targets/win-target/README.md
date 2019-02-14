# Windows Sensors - Windows User Space Sensor Installation and Run Instructions

## Differences And Caveats

### Docker
Docker is not used for containerizing the Windows Sensors due to the immaturity of the Windows 10 Docker platform.

### Installation
This is *not* an unattended installation, several User Access Control and installation prompts must be handled manually, as well as installing the Sensor Driver.

## Commandline Installation
1. Spin up an AWS instance using the `ami-05d864f01373c854a` AMI, this is a clean Windows 10 machine with SSH Host installed for the `vrtu` key. This can be done via our Terraform script `location` or manually through the AWS GUI.
2. Connect to the new instance via
`ssh -i <path/to/key> virtue-admin@<ipaddress>`
3. `powershell`
4.
```powershell
iex (new-object net.webclient).downloadstring('https://get.scoop.sh')
scoop install git
scoop bucket add versions
scoop install python36
python bin\install_sensors.py
RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 Savior-Win-Driver-20181205-02\WinVirtUE.inf
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
