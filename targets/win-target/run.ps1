echo "Starting Sensors"
Start-Process -FilePath powershell.exe -ArgumentList 'c:\opt\sensor_startup\run_sensors.ps1' -Wait
