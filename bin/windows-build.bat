@echo off
@ECHO Configuring execution environment . . .
SET WORKDIR=%SystemDrive%\app
SET TEMP=%SystemDrive%\SaviorTemp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.5
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
@ECHO Installing WinVirtUE Service Files
XCOPY /Y /E /F sensor_service\WinVirtUE\*.* %SystemDrive%\WinVirtUE
@ECHO Installing Sensor Libraries ... Part 2
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
CD ..\..
RMDIR /q /s  .\app
POPD

RMDIR /Q /S %TEMP%
copy /y c:\Python%PYTHONVER%\Lib\site-packages\pywin32_system32\pywintypes36.dll c:\Python%PYTHONVER%\lib\site-packages\win32

SET PYTHONPATH=%SystemDrive%\
python -m WinVirtUE install
sc config WinVirtue start= auto
python -m WinVirtUE start

@ECHO POP back to .\savior
POPD

@ECHO Disabling driver signing enforcement (WinVirtUE driver is UNSIGNED)
bcdedit -set testsigning       on
bcdedit -set nointegritychecks on

@ECHO System must be restarted
