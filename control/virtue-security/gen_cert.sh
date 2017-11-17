#!/bin/sh
mkdir -p cert
openssl genrsa -out cert/rsa_key 4096 -config tls_cert.conf
openssl rsa -in cert/rsa_key -outform PEM -pubout -out cert/rsa_key.pub