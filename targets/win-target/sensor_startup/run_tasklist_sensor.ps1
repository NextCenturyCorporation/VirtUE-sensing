#!/usr/bin/env bash

python /opt/sensors/proc/sensor_proc.py --public-key-path /opt/sensors/proc/certs/rsa_key.pub --private-key-path /opt/sensors/proc/certs/rsa_key --ca-key-path /opt/sensors/proc/certs/ --api-host api --sensor-port 11020