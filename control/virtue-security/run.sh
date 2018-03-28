#!/usr/bin/env bash

echo "Getting Client Certificate"
python /usr/src/app/tools/get_certificates.py --cfssl-host sensing-ca.savior.internal --hostname api -d /usr/src/app/certs --quiet

echo "Running virtue-security"
python /usr/src/app/virtue-security --public-key-path /usr/src/app/certs/cert.pem "$@" --ca-key-path /usr/src/app/certs/ca.pem --private-key-path /usr/src/app/certs/cert-key.pem