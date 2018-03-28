echo "Running Sensor TaskList . . ."

Start-Process -FilePath c:\python3.6.4\python.exe -ArgumentList "c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\cert\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\cert\rsa_key --ca-key-path c:\opt\sensors\tasklist\cert\ --api-host api --sensor-port 11020"
