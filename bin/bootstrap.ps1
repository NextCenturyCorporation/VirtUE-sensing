# On the target machine, open a priviliged command prompt, execute
# "notepad bootstrap.ps1"  (excluding double-quotes).  Copy and paste this
# files' contents into the notepads window on the target machine; save
# and exit notepad to write out this file on the target machine
# To run this script, copy and paste the text between <# and #> Below
<# powershell -NoProfile -ExecutionPolicy ByPass -File .\bootstrap.ps1 #>

Write-Output "Downloading git"

mkdir c:\temp

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Write-Output "Downloading git . . . "
Invoke-WebRequest -Uri "https://github.com/git-for-windows/git/releases/download/v2.16.2.windows.1/Git-2.16.2-64-bit.exe" -OutFile c:\TEMP\Git-2.16.2-64-bit.exe
Write-Output "Installing git .  . ."
Start-Process -FilePath c:\TEMP\Git-2.16.2-64-bit.exe -Wait -ArgumentList "/SILENT", "/COMPONENTS=icons,ext\reg\shellhere,assoc,assoc_sh" 
Write-Output "Removing git installer .  . ."
Remove-Item c:\TEMP\Git-2.16.2-64-bit.exe -Force
$Env:PATH="C:\Program Files\Git\cmd;" + $Env:PATH

Write-Output "Clone savior repo . . . "
git clone -v https://github.com/twosixlabs/savior.git
cd ./savior


