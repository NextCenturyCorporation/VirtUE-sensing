#!/usr/bin/env bash

#### PRODUCTION USE ####
# Might need --sensor-advertised-host A.B.C.D
python /opt/sensors/bcc/sensor_bcc.py --public-key-path /opt/sensors/bcc/certs/rsa_key.pub --private-key-path /opt/sensors/bcc/certs/rsa_key --ca-key-path /opt/sensors/bcc/certs/ --api-host sensing-api.savior.internal --sensor-port 11005 --sensor-host 0.0.0.0 --sensor-advertised-port 11005


#### DEVELOPMENT USE ONLY ####
#VERBOSE= python3 /root/sensor_bcc/sensor_bcc.py --public-key-path /root/sensor_bcc/certs/rsa_key.pub --private-key-path /root/sensor_bcc/certs/rsa_key --ca-key-path /root/sensor_bcc/certs/ --api-host sensing-api.savior.internal --sensor-port 11005 --sensor-host 0.0.0.0 --sensor-advertised-host 3.80.108.247 --sensor-advertised-port 11005

