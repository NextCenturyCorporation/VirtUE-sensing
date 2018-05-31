# On the target machine, open a priviliged command prompt, execute
# "notepad bootstrap.ps1"  (excluding double-quotes).  Copy and paste this
# files' contents into the notepads window on the target machine; save
# and exit notepad to write out this file on the target machine
# To run this script, copy and paste the text between <# and #> Below
<# powershell -NoProfile -ExecutionPolicy ByPass -File .\bootstrap.ps1 #>

Write-Output "Downloading python 2.7, git and chrome"

mkdir c:\temp

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Write-Output "Download and Install python . . . "
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.6.4/python-3.6.4.exe" -OutFile c:\TEMP\python-3.6.4.exe
Write-Output "Installing Python .  . ."
Start-Process -FilePath C:\Windows\System32\msiexec.exe -Wait -ArgumentList "/i", "c:\TEMP\python-3.6.4.exe", "/quiet",  "TARGETDIR=c:\Python-3.6.4", "InstallAllUsers=1", "PrependPath=1"
Write-Output "Removing Python installer .  . ."
Remove-Item c:\TEMP\python-3.6.4.exe -Force
$Env:PATH="c:\Python-3.6.4\Scripts;c:\Python-3.6.4;" + $Env:PATH
python -m pip install --upgrade pip

Write-Output "Downloading git . . . "
Invoke-WebRequest -Uri "https://github.com/git-for-windows/git/releases/download/v2.16.2.windows.1/Git-2.16.2-64-bit.exe" -OutFile c:\TEMP\Git-2.16.2-64-bit.exe
Write-Output "Installing git .  . ."
Start-Process -FilePath c:\TEMP\Git-2.16.2-64-bit.exe -Wait -ArgumentList "/SILENT", "/COMPONENTS=icons,ext\reg\shellhere,assoc,assoc_sh" 
Write-Output "Removing git installer .  . ."
Remove-Item c:\TEMP\Git-2.16.2-64-bit.exe -Force
$Env:PATH="C:\Program Files\Git\cmd;" + $Env:PATH

Write-Output "Clone, fetch and checkout savior . . . "
git clone -v https://github.com/twosixlabs/savior.git
cd ./savior
git fetch -v origin


