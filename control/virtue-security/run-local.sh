#!/usr/bin/env bash

export CFSSL_SHARED_SECRET=de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f

mkdir certs

echo "Getting Client Certificate"
python ./tools/get_certificates.py --cfssl-host sensing-ca.savior.internal --hostname api -d certs --quiet

echo "Running virtue-security"
python virtue-security --public-key-path certs/cert.pem "$@" --ca-key-path certs/ca.pem --private-key-path certs/cert-key.pem