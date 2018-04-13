@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=C:\app
SET TEMP=%SystemDrive%\Temp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

MKDIR %WORKDIR%
MKDIR %TEMP%

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
POPD

@ECHO Installing All Sensors
MKDIR %SystemDrive%\opt
MKDIR %SystemDrive%\opt\sensors\
XCOPY /Y /S /F /V sensors\*.* %SystemDrive%\opt\sensors\

@ECHO Installing Sensor Startup Scripts
MKDIR %SystemDrive%\opt\sensor_startup
XCOPY /Y /S /F /V sensor_startup\*.* %SystemDrive%\opt\sensor_startup\

@ECHO Installing Service Components
COPY /Y run.ps1 %SystemDrive%\app
@ECHO Download the handles.exe from SysInternals/MS 
@ECHO *** NOTE: This files URI could be moved without warning ***
%POWERSHELL% Invoke-WebRequest -Uri "https://download.sysinternals.com/files/Handle.zip" -OutFile %TEMP%\Handle.zip
%POWERSHELL% Expand-Archive -Force %TEMP%\Handle.zip -DestinationPath %SystemDrive%\opt\sensors\handlelist

@ECHO Agree to the license on the dialog box
%SystemDrive%\opt\sensors\handlelist\handle.exe > nul:

@ECHO POP back to .\savior
POPD

@ECHO Opening Port 11020-11022 in the firewall for sensor communications
netsh advfirewall firewall add rule name="Open Port 11020" dir=in action=allow protocol=TCP localport=11020
netsh advfirewall firewall add rule name="Open Port 11021" dir=in action=allow protocol=TCP localport=11021
netsh advfirewall firewall add rule name="Open Port 11022" dir=in action=allow protocol=TCP localport=11022
