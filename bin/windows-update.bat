@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=C:\app
SET TEMP=%SystemDrive%\SaviorTemp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

MKDIR %WORKDIR%
MKDIR %TEMP%
MKDIR %SystemDrive%\WinVirtUE

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
