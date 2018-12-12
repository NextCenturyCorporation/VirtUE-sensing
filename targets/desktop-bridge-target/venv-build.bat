REM For use on Windows only
REM Run on the User Desktop machine
REM Downloads right version of Python and virtualenv support

@echo off
@ECHO Configuring execution environment . . .

SET VENVDIR=%HOMEPATH%\python-venv-desktop-bridge-sensor

SET TEMP=%SystemDrive%\SaviorTemp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.5
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

MKDIR %TEMP%

@ECHO Download and Install Visual Studio 2017 Build Kit w/2015 Components . . .
REM %POWERSHELL% Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile %TEMP%\vs_BuildTools.exe 
REM %TEMP%\vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest --includeRecommended 
REM DEL /F /Q %TEMP%\vs_BuildTools.exe

REM @ECHO Download and Install python . . . 
REM %POWERSHELL% [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;Invoke-WebRequest -Uri "https://www.python.org/ftp/python/%PYTHONVER%/python-%PYTHONVER%.exe" -OutFile %TEMP%\python-%PYTHONVER%.exe 
REM %TEMP%\python-%PYTHONVER%.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1 TargetDir=%SystemDrive%\Python%PYTHONVER% CompileAll=1' -Wait 
REM DEL /F /Q %TEMP%\python-%PYTHONVER%.exe
REM SET PATH=%SystemDrive%\Python%PYTHONVER%\Scripts;%SystemDrive%\Python%PYTHONVER%;%PATH%
python -m pip install --upgrade pip
python -m pip install virtualenv

REM Now build up the virtual environment

virtualenv %VENVDIR%

XCOPY /Y /E /F sensor\bridge.py %VENVDIR%
XCOPY /Y /E /F run-venv.bat     %VENVDIR%

REM While we're activated, this script is ignored. So call all the
REM things needed in one command

%VENVDIR%\Scripts\activate && python -m pip install .\common_libraries\sensor_wrapper --upgrade && deactivate
