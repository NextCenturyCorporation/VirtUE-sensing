@echo off
set SENSORNAME=processlist
set PORT=11021
pushd c:\opt\sensors\%SENSORNAME%
del /f certs\rsa_key 
python c:\opt\sensors\%SENSORNAME%\sensor_%SENSORNAME%.py --public-key-path c:\opt\sensors\%SENSORNAME%\certs\rsa_key.pub --private-key-path c:\opt\sensors\%SENSORNAME%\certs\rsa_key --ca-key-path c:\opt\sensors\%SENSORNAME%\certs\ --api-host sensing-api.savior.internal --sensor-port %PORT%
popd
