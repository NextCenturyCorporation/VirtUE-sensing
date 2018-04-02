@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=C:\app
SET TEMP=%SystemDrive%\Temp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

@ECHO Go to the windows target directory from .\savior
PUSHD targets\win-target

@ECHO Installing REQUIREMENTS.TXT Install and run . . . 
XCOPY /Y /S /F /V requirements\*.* %SystemDrive%\app\requirements\
%SystemDrive%\Python%PYTHONVER%\scripts\pip.exe install -r %SystemDrive%\app\requirements\requirements_master.txt

@ECHO Installing Sensor Libraries ... Part 1
XCOPY /Y /S /F /V sensor_libraries\*.* %SystemDrive%\app\sensor_libraries\
@ECHO Installing Sensor Libraries ... Part 2
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
POPD

@ECHO Installing All Sensors
XCOPY /Y /S /F /V sensors\*.* %SystemDrive%\opt\sensors\

@ECHO Installing Sensor Startup Scripts
XCOPY /Y /S /F /V sensor_startup\*.* %SystemDrive%\opt\sensor_startup\

@ECHO Installing Service Components
COPY /Y run.ps1 %SystemDrive%\app

@ECHO POP back to .\savior
POPD

@ECHO Running Sensors . . .
%POWERSHELL% %SystemDrive%\app\run.ps1
