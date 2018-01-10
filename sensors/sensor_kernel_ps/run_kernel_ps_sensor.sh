#!/usr/bin/env bash

python /opt/sensors/kernel_ps/sensor_kernel_ps.py --public-key-path /opt/sensors/kernel_ps/certs/rsa_key.pub --private-key-path /opt/sensors/kernel_ps/certs/rsa_key --ca-key-path /opt/sensors/kernel_ps/certs/ --api-host api --sensor-port 11030
