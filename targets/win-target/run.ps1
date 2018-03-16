echo "Starting Sensors"
Start-Process -NoNewWindow -FilePath powershell.exe -ArgumentList 'c:\opt\sensor_startup\run_sensors.ps1'
