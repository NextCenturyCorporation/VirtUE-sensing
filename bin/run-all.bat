@echo off

@ECHO Opening Port 11020 - 11022 in the firewall for sensor communications
netsh advfirewall firewall add rule name="Open Port 11020" dir=in action=allow protocol=TCP localport=11020
netsh advfirewall firewall add rule name="Open Port 11021" dir=in action=allow protocol=TCP localport=11021
netsh advfirewall firewall add rule name="Open Port 11022" dir=in action=allow protocol=TCP localport=11022

@ECHO Running All Sensors . . .
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 
%POWERSHELL% %SystemDrive%\app\run.ps1
