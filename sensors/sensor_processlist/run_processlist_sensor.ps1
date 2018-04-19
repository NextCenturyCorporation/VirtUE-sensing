$host.ui.RawUI.WindowTitle = "Running ProcessList Sensor . . ."
Write-Output $host.ui.RawUI.WindowTitle

cd c:\opt\sensors\processlist
if(Test-Path .\certs\rsa_key) { Remove-Item -Force .\certs\rsa_key }
if(Test-Path .\certs\rsa_key.pub) { Remove-Item -Force .\certs\rsa_key.pub }

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\processlist\sensor_processlist.py --public-key-path c:\opt\sensors\processlist\certs\rsa_key.pub --private-key-path c:\opt\sensors\processlist\certs\rsa_key --ca-key-path c:\opt\sensors\processlist\certs\ --api-host sensing-api.savior.internal --sensor-port 11021" -WindowStyle Minimized -WorkingDirectory c:\opt\sensors\processlist
