#!/usr/bin/env bash

##
## Starts the NFS sensor from within the docker container
##

_dsensor=/opt/sensor


# Spin up our sensor
echo " [bash] Starting NFS sensor"
echo " [bash] env NFS_SENSOR_SNIFF_INTERFACE: \"$NFS_SENSOR_SNIFF_INTERFACE\""
echo " [bash] env NFS_SENSOR_STANDALONE     : \"$NFS_SENSOR_STANDALONE\""
echo " [bash] env NFS_SENSOR_PCAP_FILE      : \"$NFS_SENSOR_PCAP_FILE\""

# Matches /docker-compose.yml
export CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

if [ -z "$NFS_SENSOR_SNIFF_INTERFACE" && -z "$NFS_SENSOR_PCAP_FILE" ]
then
    echo "Environment variables NFS_SENSOR_SNIFF_INTERFACE and NFS_SENSOR_PCAP_FILE undefined!"
    echo "I don't know where to read packets from."
    exit 1
fi


if [ -n "$NFS_SENSOR_STANDALONE" ]
then
    echo "NFS sensor starting in standalone mode"
    # Useful for debugging only....
    python /opt/sensor/standalone_nfs_tap.py --iface $NFS_SENSOR_SNIFF_INTERFACE
else
    echo "NFS sensor will communicate with API sensing server"

    # N.B. the keys indicate files, but they don't have to exist. It's
    # where the keys are written.

    if [ -n "$NFS_SENSOR_PCAP_FILE" ]
    then
       echo "NFS sensor is parsing PCAP file $NFS_SENSOR_PCAP_FILE"
       python $_dsensor/sensor_nfs.py                     \
              --pcap $NFS_SENSOR_PCAP_FILE                \
              --sensor-port 11040                         \
              --ca-key-path      $_dsensor/certs          \
              --public-key-path  $_dsensor/certs/cert.pem \
              --private-key-path $_dsensor/certs/cert-key.pem
    else
        echo "NFS sensor is sniffing on $NFS_SENSOR_SNIFF_INTERFACE"
        python $_dsensor/sensor_nfs.py                     \
               --iface $NFS_SENSOR_SNIFF_INTERFACE         \
               --sensor-port 11040                         \
               --ca-key-path      $_dsensor/certs          \
               --public-key-path  $_dsensor/certs/cert.pem \
               --private-key-path $_dsensor/certs/cert-key.pem
    fi
fi
