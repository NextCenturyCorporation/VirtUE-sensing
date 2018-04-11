echo "Running Sensor ProcesList . . ."

cd c:\opt\sensors\handlelist
Remove-Item -Force .\certs\rsa_key
Remove-Item -Force .\certs\rsa_key.pub

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\handlelist\sensor_handlelist.py --public-key-path c:\opt\sensors\handlelist\certs\rsa_key.pub --private-key-path c:\opt\sensors\handlelist\certs\rsa_key --ca-key-path c:\opt\sensors\handlelist\certs\ --api-host sensing-api.savior.internal --sensor-port 11022" -WindowStyle Minimized -WorkingDirectory c:\opt\sensors\handlelist

