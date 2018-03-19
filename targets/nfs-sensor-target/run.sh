#!/usr/bin/env bash

##
## Starts the NFS sensor from within the docker container
##

# Spin up our sensors
echo " [bash] Starting NFS sensor"
echo " [bash] env NFS_SENSOR_SNIFF_INTERFACE: \"$NFS_SENSOR_SNIFF_INTERFACE\""
echo " [bash] env NFS_SENSOR_STANDALONE     : \"$NFS_SENSOR_STANDALONE\""

# Move to yml file which will put this in our env
export CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

if [ -z "$NFS_SENSOR_SNIFF_INTERFACE" ]
then
    echo "Environment variable NFS_SENSOR_SNIFF_INTERFACE undefined!"
    echo "I don't know which network interface to sniff."
    exit 1
fi


if [ -n "$NFS_SENSOR_STANDALONE" ]
then
    echo "NFS sensor starting in standalone mode"
    # Useful for debugging only....    
    python /opt/sensors/standalone_nfs_tap.py --iface $NFS_SENSOR_SNIFF_INTERFACE

else
    echo "NFS sensor starting in integrated mode"

    echo '127.0.0.1 api' >> /etc/hosts

    python /opt/sensors/sensor_nfs.py                   \
           --iface $NFS_SENSOR_SNIFF_INTERFACE          \
           --api-host api --sensor-port 11040           \
           --sensor-hostname localhost                  \
           --ca-key-path      /opt/sensors/nfs_sensor/certs          \
           --public-key-path  /opt/sensors/nfs_sensor/certs/cert.pem \
           --private-key-path /opt/sensors/nfs_sensor/certs/cert-key.pem

fi
