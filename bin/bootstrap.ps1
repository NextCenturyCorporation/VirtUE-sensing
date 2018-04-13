# On the target machine, open a priviliged command prompt, execute
# "notepad bootstrap.ps1"  (excluding double-quotes).  Copy and paste this
# files' contents into the notepads window on the target machine; save
# and exit notepad to write out this file on the target machine
# To run this script, copy and paste the text between <# and #> Below
<# powershell -NoProfile -ExecutionPolicy ByPass -File .\bootstrap.ps1 #>

Write-Output "Downloading python 2.7, git and chrome"

mkdir c:\temp

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Write-Output "Downloading Python . . . "
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/2.7.14/python-2.7.14.msi" -OutFile c:\TEMP\python-2.7.14.msi
Write-Output "Installing Python .  . ."
Start-Process -FilePath C:\Windows\System32\msiexec.exe -Wait -ArgumentList "/i", "c:\TEMP\python-2.7.14.msi", "/passive", "/qb", "TARGETDIR=c:\Python-2.7.14"
Write-Output "Removing Python installer .  . ."
Remove-Item c:\TEMP\python-2.7.14.msi -Force
$Env:PATH="c:\Python-2.7.14\Scripts;c:\Python-2.7.14;" + $Env:PATH

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
# Comment out or remove this branch checkout when its merged
git checkout enh-win10-handles-list-sensor

