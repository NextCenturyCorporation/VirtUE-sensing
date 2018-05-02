Windows VirtUE Driver
Version 0.1.0.1

# Build Environment Requirements
0. Install latest git for windows
1. Install python 3.6.X ( latest version )
2. Install Visual Studio 2017 (either versions)
3. Install Win32 Software Developers Kit (SDK)
4. Install Latest Windows Driver Developers Kit (DDK)


# How To Build and Run
0. Open the WinVirtUE.sln soluation file, select either Debug or Release profiles and rebuild.
1. Copy/Paste the x64\$(Configuration)\WinVirtUE directory to the VM's desktop
2. Right click and select install the WinVirtUE.inf file
3. Open a administrators command prompt and execute the following command:
```Cmd
sc start WinVirtUE
```
4. Verify that the driver is loaded and running by executing these two commands:
```Cmd
C:\> driverquery
...

C:\> sc queryex WinVirtUE
...
```
Output of driverquery should show at the last position that the driver is loaded.  The output of the sc queryex command should show that the service started.
