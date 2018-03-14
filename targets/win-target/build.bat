@echo on

REM base system setup and requirements
SET WORKDIR=C:\app
SET TEMP=%SystemDrive%\Temp
SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.4

MKDIR %WORKDIR%
MKDIR %TEMP%


powershell Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile %TEMP%\vs_BuildTools.exe
%TEMP%\vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --includeRecommended
del /F /Q %TEMP%\vs_BuildTools.exe

powershell Invoke-WebRequest -Uri "https://www.python.org/ftp/python/%PYTHONVER%/python-%PYTHONVER%.exe" -OutFile %TEMP%\python-%PYTHONVER%.exe 
%TEMP%\python-%PYTHONVER%.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1 TargetDir=%TEMP%\Python%PYTHONVER% CompileAll=1' -Wait 
del /F /Q %TEMP%\python-%PYTHONVER%.exe

# REQUIREMENTS.TXT install and run
MKDIR %SystemDrive%\app\requirements
PUSHD %SystemDrive%\app
XCOPY /S /F /V requirements\ %SystemDrive%\app\requirements/
%SystemDrive%\Python%PYTHONVER%\scripts\pip.exe install -v -r %SystemDrive%\app\requirements\requirements_master.txt
POPD

# SENSOR LIBRARIES
MKDIR %SystemDrive%\app\sensor_libraries
PUSHD %SystemDrive%\app
XCOPY /S /F /V sensor_libraries\ %SystemDrive%\app\sensor_libraries\
PUSHD %SystemDrive%\app\sensor_libraries
powershell .\install.ps1
POPD
POPD

# Run the pip install script for required OS support in sensors
powershell %SystemDrive%\app\sensor_libraries\pip_install.ps1

# SENSORS
MKDIR %SystemDrive$\opt
MKDIR %SystemDrive$\opt\sensors\
PUSHD %SystemDrive$\opt
XCOPY /S /F /V sensors\ %SystemDrive%\opt\sensors\
POPD

# RUN SCRIPTS
MKDIR %SystemDrive%\opt\sensor_startup
PUSHD %SystemDrive%\opt
XCOPY /S /F /V sensor_startup\ %SystemDrive%\opt\sensor_startup\
POPD

# Service components
COPY /Y dropper.ps1 %TEMP%\dropper.ps1
COPY /Y run.ps1 %SystemDrive%\app

powershell %SystemDrive%\app\run.ps1
