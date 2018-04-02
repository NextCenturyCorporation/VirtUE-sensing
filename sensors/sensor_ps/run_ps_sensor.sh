#!/usr/bin/env bash

python /opt/sensors/ps/sensor_ps.py --public-key-path /opt/sensors/ps/certs/rsa_key.pub --private-key-path /opt/sensors/ps/certs/rsa_key --ca-key-path /opt/sensors/ps/certs/ --api-host sensing-api.savior.internal --sensor-port 11002
