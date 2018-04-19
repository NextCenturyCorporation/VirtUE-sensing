$host.ui.RawUI.WindowTitle = "Running TaskList Sensor . . ."
Write-Output $host.ui.RawUI.WindowTitle

cd c:\opt\sensors\tasklist
if(Test-Path .\certs\rsa_key) { Remove-Item -Force .\certs\rsa_key }
if(Test-Path .\certs\rsa_key.pub) { Remove-Item -Force .\certs\rsa_key.pub }

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\tasklist\sensor_tasklist.py --public-key-path c:\opt\sensors\tasklist\certs\rsa_key.pub --private-key-path c:\opt\sensors\tasklist\certs\rsa_key --ca-key-path c:\opt\sensors\tasklist\certs\ --api-host sensing-api.savior.internal --sensor-port 11020" -WindowStyle Minimized -WorkingDirectory c:\opt\sensors\tasklist
