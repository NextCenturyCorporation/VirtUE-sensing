@echo off
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass

%POWERSHELL% %SystemDrive%\opt\sensor_startup\run_tasklist_sensor.ps1
