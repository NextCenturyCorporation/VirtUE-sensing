# Either use 1480 or older 1715 or newer windows server core else
# Visual Studio Build Tools will not install
FROM microsoft/windowsservercore:10.0.14393.1480
#FROM microsoft/windowsservercore:10.0.14393.1715
 
# base system setup and requirements
WORKDIR C:/app

ENV PYTHONUNBUFFERED=0
ENV TEMP c:/users/containeradministrator/appdata/local/temp

SHELL ["powershell", "-NoProfile", "-ExecutionPolicy Bypass", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

LABEL Description="Python" Vendor="Python Software Foundation" Version="3.6.4"

RUN Invoke-WebRequest -Uri "https://aka.ms/vs/15/release/vs_BuildTools.exe" -OutFile c:/vs_BuildTools.exe

RUN c:/vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.MSBuildTools --includeRecommended

RUN  wget https://www.python.org/ftp/python/3.6.4/python-3.6.4.exe -OutFile c:\python-3.6.4.exe 
#RUN Start-Process c:\python-3.6.4.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1 TargetDir=c:\Python364 CompileAll=1' -Wait 
#RUN Remove-Item c:\python-3.6.4.exe -Force

# REQUIERMENTS.TXT install and run
#RUN mkdir C:/app/requirements
#COPY requirements/ C:/app/requirements/
#RUN c:/python364/scripts/pip install -r C:/app/requirements/requirements_master.txt

# SENSOR LIBRARIES
#RUN mkdir C:/app/sensor_libraries
#COPY sensor_libraries/ C:/app/sensor_libraries/
#WORKDIR C:/app/sensor_libraries
#RUN ./install.ps1

# Run the pip install script for required OS support in sensors
#RUN C:/app/sensor_libraries/pip_install.ps1

# KERNEL MODULES
#RUN mkdir C:/app/kmods
#COPY kmods/ C:/app/kmods
#WORKDIR C:/app/kmods
#RUN ./install.ps1

# SENSORS
#RUN mkdir c:/opt
#RUN mkdir c:/opt/sensors
#COPY sensors/ c:/opt/sensors/

# RUN SCRIPTS
#RUN mkdir /opt/sensor_startup
#COPY sensor_startup/ /opt/sensor_startup/

# Service components
#COPY dropper.ps1 /tmp/dropper.ps1
#COPY run.ps1 C:/app

#CMD ["powershell", "C:/app/run.ps1"]
