#!/usr/bin/env bash

# Spin up our sensors
echo "Starting NFS sensor"

#python /opt/sensors/standalone_nfs_tap.py --iface vif11.0

# Move to yml file
export CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

_api=172.17.0.7
_api=api
python /opt/sensors/sensor_nfs.py                                    \
       --public-key-path  /opt/sensors/nfs_sensor/certs/cert.pem     \
       --private-key-path /opt/sensors/nfs_sensor/certs/cert-key.pem \
       --ca-key-path      /opt/sensors/nfs_sensor/certs/             \
       --api-host $_api --sensor-port 11040
