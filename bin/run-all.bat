@echo off

@ECHO Running All Sensors . . .
SET POWERSHELL=powershell -NoProfile -ExecutionPolicy Bypass 
%POWERSHELL% %SystemDrive%\app\run.ps1
