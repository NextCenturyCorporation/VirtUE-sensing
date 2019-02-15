@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=%SystemDrive%\app
SET TEMP=%SystemDrive%\SaviorTemp
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 
SET WINVIRTUE=%SystemDrive%\WinVirtUE

MKDIR %WORKDIR%
MKDIR %TEMP%
MKDIR %WINVIRTUE%

@ECHO Download and Install Visual Studio 2017 Build Kit w/2015 Components . . .
%POWERSHELL% Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile %TEMP%\vs_BuildTools.exe 
%TEMP%\vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest --includeRecommended 
DEL /F /Q %TEMP%\vs_BuildTools.exe

@ECHO Installing ntfltmgr
pip install sensors\ntfltmgr

@ECHO Go to the windows target directory from .\savior
PUSHD targets\win-target

@ECHO Installing REQUIREMENTS.TXT Install and run . . . 
MKDIR %SystemDrive%\app\requirements
XCOPY /Y /S /F /V requirements\*.* %SystemDrive%\app\requirements\
pip install -r %SystemDrive%\app\requirements\requirements_master.txt

@ECHO Installing Sensor Libraries ... Part 1
MKDIR %SystemDrive%\app\sensor_libraries
XCOPY /Y /S /F /V sensor_libraries\*.* %SystemDrive%\app\sensor_libraries\
@ECHO Installing WinVirtUE Service Files
XCOPY /Y /E /F sensor_service\WinVirtUE\*.* %SystemDrive%\WinVirtUE
@ECHO Installing Sensor Libraries ... Part 2
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
CD ..\..
RMDIR /q /s  .\app
POPD

RMDIR /Q /S %TEMP%

PUSHD %WINVIRTUE%
sc config WinVirtue start=auto
python WinVirtUE\service_winvirtue.py --startup=auto install
sc config "WinVirtUE Service" depend=WinVirtUE
python WinVirtUE\service_winvirtue.py start
sc failure "WinVirtUE Service" reset=1 actions=restart/60000
POPD

@ECHO POP back to .\savior
POPD

@ECHO Disabling driver signing enforcement (WinVirtUE driver is UNSIGNED)
bcdedit -set testsigning       on
bcdedit -set nointegritychecks on

@ECHO System must be restarted
