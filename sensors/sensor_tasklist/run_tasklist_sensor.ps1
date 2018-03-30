echo "Running Sensor TaskList . . ."

pushd c:\opt\sensors\tasklist
Start-Process -FilePath python.exe -ArgumentList ".\gen_key.py" -Wait
popd

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\certs\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\certs\rsa_key --ca-key-path c:\opt\sensors\tasklist\certs\ --api-host sensing-api.savior.internal --sensor-port 11020" -WindowStyle Normal
