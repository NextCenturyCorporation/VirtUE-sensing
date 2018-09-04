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

# Updating Sensors (If Required)
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

6. Notify when unsigned WSH files are loaded (powershell, active state, et)

7. Monitor registry keys associated with loading dll's. 

8. Analyze VAD tree, XOrderModuleList, and E/IAT to look for hidden modules and other inconsistencies.

9. TCP/IP Port Utilization by Process.  Track when/which processes bind to ports

10. Explorer/edge webview DLL usage (embedded web)

11. audio/microphone/video taps.  Includes mixers, filters and etc?

12. Samba mounts/activity  (all volume mount/dismount)

13. tool/software installation (MSI tracking). How to detect an xcopy style installation?  Perhaps:  If a .exe is copied, and does not track to a formal install then flag this as an xcopy install?  Might need additional checks?

14. Device installation/removal 

15. policy vs probe - that a probe is active for a specific 'thing' should be different from the policy.  Perhaps we are only interested in the last 'n' things, making that 'n' a policy statement that can be sent down by configuration?

16. Create a network filter driver that can view each net connection, downloads, URL's, and etc. Timestamp these events for proper correlation on module loads, process creations and etc.

17. Monitor the creation/modification of win32 and driver shims through the various shim databases (.sdb).  This is another used, but rather obscure, user attack vector. 
Win32 Shim Information: https://www.geoffchappell.com/studies/windows/win32/apphelp/sdb/index.htm?tx=54 
Kernel Shim Information: https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/kshim/drvmain.htm?tx=52,53,56,59,67&ts=0,1555

