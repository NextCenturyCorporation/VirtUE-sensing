#!/usr/bin/env bash


# Get certificates
python3 /var/tools/get_certificates.py --cfssl-host cfssl --hostname kafka -d $KAFKA_HOME/certs

# Build the certs into a java trust store and certificate store

## 1. Pack everything into a PKCS12 format
openssl pkcs12 -export -inkey $KAFKA_HOME/certs/cert-key.pem -in $KAFKA_HOME/certs/cert.pem -certfile $KAFKA_HOME/certs/ca.pem -out $KAFKA_HOME/certs/kafka.server.pfx -passout pass:

## 2. Build the public/private key store
keytool -importkeystore -deststorepass kafka_key_store -destkeystore $KAFKA_HOME/certs/server.keystore.jks -srckeystore $KAFKA_HOME/certs/kafka.server.pfx -srcstoretype PKCS12 -alias 1 -srcstorepass ""

## 3. Build the trust store with the root CA certificate
keytool -keystore $KAFKA_HOME/certs/server.truststore.jks -alias CARoot -import -file $KAFKA_HOME/certs/ca.pem -deststorepass kafka_trust_store -noprompt

# Run the server
supervisord -n