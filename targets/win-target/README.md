Windows Sensors - Dummy Sensor Installation and Run Instructions
Mark Sanderson
mark.sanderson@twosixlabs.com

# Differences And Caveats
## Docker Issues
For now, Docker *will not* be used to containerize the Windows Sensors.  The Windows 10 Docker product is still fairly immature, and it is not clear if we'll be able to take advantage of it's functionality in the future.  There were a number of issues including random virtual network malfunctions and etc that prevented Docker from being useful.
## Cert Directory
The certs directory keys are now populated properly using RSA from pycryptodome library. 
## curio
curio 0.9 appears to be comptable (at least partially) with windows.  This will be proven out as time goes on.

# Building and running the Windows Dummy Sensor

1. Ensure that you have a virtual machine running Windows 10 x64 w/git for windows installed.
2. Log on to the windows 10 target, and clone this respository.
3. Switch the branch to 'enh-create-win10-dummy-sensor':
```Cmd
git checkout enh-win10-basic-sensors
```
4. from the savior subdirectory on the virtual machine execute the sensor installation/staging script
```Cmd
c:\python27\python bin\install_sensors.py
```
5. From the same savior directory execute windows build batch file:
```Cmd
bin\windows-build.bat
```
5. Visual Studio 2017 build environment with VS 2015 components will be the first requirements to be installed.  VS 2017 is required by at least module to compile required native code.
6. Python 3.6.4 will be installed next, and you will need to be there to click through some of the UAC and python installation menu prompts.  The choices should be obvious, let me know if there is something unexpected in the installer prompting.
7. Python requirements will be installed after the the python installer exits.  There are at least one required python packages that require the VS build enviornment, notably the http package.
8. After the prerequisites are installed, then the build script will create target environment almost completely modeled on the Linux model.  Since there is no docker container running, sensor installation is handled statically.
9. The last notable step to occur is the executable of the tasklist sensor.  This sensor is based (loosely) on the lsof dummy sensor.  It does not function, it merely starts up, and attempts repeatedly to connect to api services.
10. After spending 30 seconds or so failing to connect to the API, the dummy sensor will terminate.

# Basic Windows Sensors
1. Process Creation and Destruction.  

2. Module load (and optional signature analysis)

3. Threads creation and destruction

4. Registry Modification

5. Security Log

# Sensor Research Ideas
0.Look at recent AV patent filings for inspiration.  (Thanks Matt!) and look at Volitility and how it analyzes memory and etc.

1. Analyze driver object utilzation in an effort to determine if any are used in the driver object clone style attack.   This attack has been used for creating keyloggers that have not beeen easily detected by the PatchGuard and AV's.

2. Analyze thread creation parameters.  If a thread entry start address falls outside of known module addresses, then notify possible thread injection attack.  This should be doable for both kernel and user modules(dll's).

3. Analyze module loading to notify if unusual pathing or unsigned dll's/modules are loaded in a process.  

4. Notify when alternate file streams are utilized

5. Notify when system files are modified (or an attempt is made) when installation/updates are not running

6. Notify when unsigned WSH files are loaded (powershell, active state, etc)

7. Monitor registry keys associated with loading dll's. 

8. Analyze VAD tree, XOrderModuleList, and E/IAT to look for hidden modules and other inconsistencies.

9. TCP/IP Port Utilization by Process

10. Explorer/edge webview DLL usage (embedded web)

11. audio/microphone/video taps.  Includes mixers, filters and etc?

12. Samba mounts/activity  (all volume mount/dismount)

13. tool/software installation (MSI tracking). How to detect an xcopy style installation?  Perhaps:  If a .exe is copied, and does not track to a formal install then flag this as an xcopy install?  Might need additional checks?

14. Device installation/removal 
