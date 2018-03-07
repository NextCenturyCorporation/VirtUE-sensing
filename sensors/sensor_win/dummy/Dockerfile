FROM microsoft/iis

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

WORKDIR C:/App

RUN Write-Host 'Downloading python-3.6.4.exe' ; \
    $ExeFile = $env:Temp + '\python-3.6.4.exe' ; \
    (New-Object Net.WebClient).DownloadFile('https://www.python.org/ftp/python/3.6.4/python-3.6.4.exe', $ExeFile); \
    Write-Host 'Installing python-3.6.4.exe' ; \
Start-Process $ExeFile -ArgumentList '/quiet' -NoNewWindow -Wait


COPY  hello.py c:/App



