#!/usr/bin/env bash

python /opt/sensors/lsof/sensor_lsof.py --public-key-path /opt/sensors/lsof/certs/rsa_key.pub --private-key-path /opt/sensors/lsof/certs/rsa_key --ca-key-path /opt/sensors/lsof/certs/ --api-host api