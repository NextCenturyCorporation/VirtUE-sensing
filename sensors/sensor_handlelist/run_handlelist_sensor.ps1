echo "Running Sensor ProcesList . . ."

pushd c:\opt\sensors\handlelist\certs
Remove-Item -Force .\rsa_key
Remove-Item -Force .\rsa_key.pub
popd

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\handlelist\sensor_handlelist.py --public-key-path c:\opt\sensors\handlelist\certs\rsa_key.pub --private-key-path c:\opt\sensors\handlelist\certs\rsa_key --ca-key-path c:\opt\sensors\handlelist\certs\ --api-host sensing-api.savior.internal --sensor-port 11022" -WindowStyle Normal
