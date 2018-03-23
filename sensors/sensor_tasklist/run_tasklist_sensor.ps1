echo "Running Sensor TaskList . . ."

Start-Process -FilePath python.exe -ArgumentList ".\gen_key.py" -Wait

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\certs\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\certs\rsa_key --ca-key-path c:\opt\sensors\tasklist\certs\ --api-host api --sensor-port 11020" -WindowStyle Normal
