#!/usr/bin/env bash

python /opt/sensors/tcpdump/sensor_tcpdump.py --public-key-path /opt/sensors/tcpdump/certs/rsa_key.pub --private-key-path /opt/sensors/tcpdump/certs/rsa_key --ca-key-path /opt/sensors/tcpdump/certs/ --api-host sensing-api.savior.internal --sensor-port 11003