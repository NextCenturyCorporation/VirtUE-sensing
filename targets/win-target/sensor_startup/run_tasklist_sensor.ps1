echo "Running Sensor TaskList . . ."

Start-Process -FilePath python.exe -ArgumentList "%SystemDrive%\opt\sensors\tasklist\sensor_tasklist.py --public-key-path %SystemDrive%\opt\sensors\tasklist\cert\rsa_key.pub --private-key-path %SystemDrive%\opt\sensors\tasklist\cert\rsa_key --ca-key-path %SystemDrive%\opt\sensors\tasklist\cert\ --api-host api --sensor-port 11020" -NoNewWindow 
