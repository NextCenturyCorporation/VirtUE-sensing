@echo off
pushd c:\opt\sensors\tasklist
del /f certs\rsa_key 
python c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\certs\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\certs\rsa_key --ca-key-path c:\opt\sensors\tasklist\certs\ --api-host sensing-api.savior.internal --sensor-port 11020
popd
