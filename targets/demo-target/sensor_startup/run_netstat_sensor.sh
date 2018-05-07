#!/usr/bin/env bash

python /opt/sensors/netstat/sensor_netstat.py --public-key-path /opt/sensors/netstat/certs/rsa_key.pub --private-key-path /opt/sensors/netstat/certs/rsa_key --ca-key-path /opt/sensors/netstat/certs/ --api-host sensing-api.savior.internal --sensor-port 11004
