#!/usr/bin/env bash

python /opt/sensors/tcpdump/sensor_tcpdump.py --public-key-path /opt/sensors/lsof/certs/rsa_key.pub --private-key-path /opt/sensors/lsof/certs/rsa_key --ca-key-path /opt/sensors/lsof/certs/ --api-host api --sensor-port 11003