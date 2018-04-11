echo "Running Sensor TaskList . . ."

cd c:\opt\sensors\tasklist
Remove-Item -Force .\certs\rsa_key
Remove-Item -Force .\certs\rsa_key.pub

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\certs\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\certs\rsa_key --ca-key-path c:\opt\sensors\tasklist\certs\ --api-host sensing-api.savior.internal --sensor-port 11020" -WindowStyle Minimized -WorkingDirectory c:\opt\sensors\tasklist
