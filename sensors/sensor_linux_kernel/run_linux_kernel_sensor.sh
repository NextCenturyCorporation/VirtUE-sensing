#!/usr/bin/env bash

python /opt/sensors/linux-kernel/sensor_linux-kernel.py --public-key-path /opt/sensors/linux-kernel/certs/rsa_key.pub --private-key-path /opt/sensors/linux-kernel/certs/rsa_key --ca-key-path /opt/sensors/linux-kernel/certs/ --api-host sensing-api.savior.internal --sensor-port 11006
