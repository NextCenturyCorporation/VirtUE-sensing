#!/usr/bin/env bash

# Spin up our sensors
echo "Starting NFS sensor"

python /opt/sensors/standalone_nfs_tap.py --iface vif11.0


#python /opt/sensors/nfs_sensor.py                         \
#       --public-key-path /opt/sensors/certs/rsa_key.pub   \
#       --private-key-path /opt/sensors/lsof/certs/rsa_key \
#       --ca-key-path /opt/sensors/lsof/certs/             \
#       --api-host api --sensor-port 11020
