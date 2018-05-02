$host.ui.RawUI.WindowTitle = "Running KernelProbe Sensor . . ."
Write-Output $host.ui.RawUI.WindowTitle

cd c:\opt\sensors\kernelprobe
if(Test-Path .\certs\rsa_key) { Remove-Item -Force .\certs\rsa_key }
if(Test-Path .\certs\rsa_key.pub) { Remove-Item -Force .\certs\rsa_key.pub }

Start-Process -FilePath python.exe -ArgumentList "c:\opt\sensors\kernelprobe\sensor_kernelprobe.py --public-key-path c:\opt\sensors\kernelprobe\certs\rsa_key.pub --private-key-path c:\opt\sensors\kernelprobe\certs\rsa_key --ca-key-path c:\opt\sensors\kernelprobe\certs\ --api-host sensing-api.savior.internal --sensor-port 11020" -WindowStyle Minimized -WorkingDirectory c:\opt\sensors\kernelprobe
