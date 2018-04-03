echo "Running Sensor TaskList . . ."

pushd c:\opt\sensors\processlist\certs
Remove-Item -Force .\rsa_key
Remove-Item -Force .\rsa_key.pub
popd

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\processlist\sensor_processlist.py --public-key-path c:\opt\sensors\processlist\certs\rsa_key.pub --private-key-path c:\opt\sensors\processlist\certs\rsa_key --ca-key-path c:\opt\sensors\processlist\certs\ --api-host sensing-api.savior.internal --sensor-port 11021" -WindowStyle Normal
