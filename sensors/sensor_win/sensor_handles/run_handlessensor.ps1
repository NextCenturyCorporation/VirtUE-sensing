#!/usr/bin/env bash

python /opt/sensors/handles/sensor_handles.py --public-key-path /opt/sensors/handles/certs/rsa_key.pub --private-key-path /opt/sensors/handles/certs/rsa_key --ca-key-path /opt/sensors/handles/certs/ --api-host api --sensor-port 11020
