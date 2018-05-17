@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=%SystemDrive%:\app
SET TEMP=%SystemDrive%\SaviorTemp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

MKDIR %WORKDIR%
MKDIR %TEMP%
MKDIR %SystemDrive%\WinVirtUE

@ECHO Download and Install Visual Studio 2017 Build Kit w/2015 Components . . .
%POWERSHELL% Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile %TEMP%\vs_BuildTools.exe 
%TEMP%\vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest --includeRecommended 
DEL /F /Q %TEMP%\vs_BuildTools.exe

@ECHO Download and Install python . . . 
%POWERSHELL% [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;Invoke-WebRequest -Uri "https://www.python.org/ftp/python/%PYTHONVER%/python-%PYTHONVER%.exe" -OutFile %TEMP%\python-%PYTHONVER%.exe 
%TEMP%\python-%PYTHONVER%.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1 TargetDir=%SystemDrive%\Python%PYTHONVER% CompileAll=1' -Wait 
DEL /F /Q %TEMP%\python-%PYTHONVER%.exe
SET PATH=%SystemDrive%\Python%PYTHONVER%\Scripts;%SystemDrive%\Python%PYTHONVER%;%PATH%
python -m pip install --upgrade pip

@ECHO Go to the windows target directory from .\savior
PUSHD targets\win-target

@ECHO Installing REQUIREMENTS.TXT Install and run . . . 
MKDIR %SystemDrive%\app\requirements
XCOPY /Y /S /F /V requirements\*.* %SystemDrive%\app\requirements\
%SystemDrive%\Python%PYTHONVER%\scripts\pip.exe install -r %SystemDrive%\app\requirements\requirements_master.txt

@ECHO Installing Sensor Libraries ... Part 1
MKDIR %SystemDrive%\app\sensor_libraries
XCOPY /Y /S /F /V sensor_libraries\*.* %SystemDrive%\app\sensor_libraries\
@ECHO Installing Sensor Libraries ... Part 2
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
CD ..\..
RMDIR /q /s  .\app
POPD

@ECHO Installing All Sensors
PUSHD .\sensor_service
@ECHO Installing Service Components
XCOPY /E /Y /F WinVirtUE\*.* %SystemDrive%\WinVirtUE
@ECHO Create the sensors zip file archive
%POWERSHELL% .\update_sensor_zip.ps1
POPD

@ECHO Download the handles.exe from SysInternals/MS 
@ECHO *** NOTE: This files URI could be moved without warning ***
%POWERSHELL% Invoke-WebRequest -Uri "https://download.sysinternals.com/files/Handle.zip" -OutFile %TEMP%\Handle.zip
%POWERSHELL% Expand-Archive -Force %TEMP%\Handle.zip -DestinationPath %SystemDrive%\WinVirtUE

@ECHO Agree to the license on the dialog box
%SystemDrive%\WinVirtUE\handle.exe > nul:

RMDIR /Q /S %TEMP%

PUSHD %SystemDrive%\WinVirtUE
SET PYTHONPATH=%CD%\sensors.zip
python .\service_winvirtue.py install
python .\service_winvirtue.py start

@ECHO POP back to .\savior
POPD
