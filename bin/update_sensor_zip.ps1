<# powershell -NoProfile -ExecutionPolicy ByPass -File .\bin\update_sensor_zip.ps1 #>


$host.ui.RawUI.WindowTitle = "Updating Windows VirtUE Sensors Zip File . . ."

Write-Output $host.ui.RawUI.WindowTitle

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$zipfile=$Env:SystemDrive + "/WinVirtUE/sensors.zip"

if(Test-Path $zipfile) { Remove-Item -Force $zipfile }

Compress-Archive -Path .\sensors\sensor_handlelist\sensor_handlelist.py -DestinationPath c:\WinVirtUE\sensors.zip -update

Compress-Archive -Path .\sensors\sensor_processlist\sensor_processlist.py -DestinationPath c:\WinVirtUE\sensors.zip -update

Compress-Archive -Path .\sensors\sensor_kernelprobe\sensor_kernelprobe.py -DestinationPath c:\WinVirtUE\sensors.zip -update
