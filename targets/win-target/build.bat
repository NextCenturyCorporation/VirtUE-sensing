@echo on

REM base system setup and requirements
SET WORKDIR=C:\app
SET TEMP=%SystemDrive%\Temp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 

MKDIR %WORKDIR%
MKDIR %TEMP%


%POWERSHELL% Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile %TEMP%\vs_BuildTools.exe 
%TEMP%\vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest --includeRecommended 
DEL /F /Q %TEMP%\vs_BuildTools.exe

%POWERSHELL% Invoke-WebRequest -Uri "https://www.python.org/ftp/python/%PYTHONVER%/python-%PYTHONVER%.exe" -OutFile %TEMP%\python-%PYTHONVER%.exe 
%TEMP%\python-%PYTHONVER%.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1 TargetDir=%SystemDrive%\Python%PYTHONVER% CompileAll=1' -Wait 
DEL /F /Q %TEMP%\python-%PYTHONVER%.exe

REM REQUIREMENTS.TXT install and run
MKDIR %SystemDrive%\app\requirements
XCOPY /Y /S /F /V requirements\*.* %SystemDrive%\app\requirements/
%SystemDrive%\Python%PYTHONVER%\scripts\pip.exe install -r %SystemDrive%\app\requirements\requirements_master.txt

REM SENSOR LIBRARIES
MKDIR %SystemDrive%\app\sensor_libraries
XCOPY /Y /S /F /V sensor_libraries\*.* %SystemDrive%\app\sensor_libraries\
PUSHD %SystemDrive%\app\sensor_libraries
%POWERSHELL% .\install.ps1
POPD

REM Run the pip install script for required OS support in sensors
%POWERSHELL% %SystemDrive%\app\sensor_libraries\pip_install.ps1

REM SENSORS
MKDIR %SystemDrive%\opt
MKDIR %SystemDrive%\opt\sensors\
XCOPY /Y /S /F /V sensors\*.* %SystemDrive%\opt\sensors\

REM RUN SCRIPTS
MKDIR %SystemDrive%\opt\sensor_startup
XCOPY /Y /S /F /V sensor_startup\*.* %SystemDrive%\opt\sensor_startup\

REM Service components
COPY /Y dropper.ps1 %TEMP%\dropper.ps1
COPY /Y run.ps1 %SystemDrive%\app

%POWERSHELL% %SystemDrive%\app\run.ps1
