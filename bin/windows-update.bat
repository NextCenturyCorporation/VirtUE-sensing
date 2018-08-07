@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=%SystemDrive%\app
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
XCOPY /Y /E /F requirements\*.* %SystemDrive%\app\requirements\
%SystemDrive%\Python%PYTHONVER%\scripts\pip.exe install -r %SystemDrive%\app\requirements\requirements_master.txt

@ECHO Installing Sensor Libraries ... Part 1
MKDIR %SystemDrive%\app\sensor_libraries
XCOPY /Y /E /F sensor_libraries\*.* %SystemDrive%\app\sensor_libraries\
@ECHO Installing WinVirtUE Service Files
XCOPY /Y /E /F sensor_service\WinVirtUE\*.* %SystemDrive%\WinVirtUE
@ECHO Installing Sensor Libraries ... Part 2
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
CD ..\..
RMDIR /Q /S  .\app
POPD

RMDIR /Q /S %TEMP%

SET PYTHONPATH=%SystemDrive%\
python -m WinVirtUE install
python -m WinVirtUE start

@ECHO POP back to .\savior
POPD
