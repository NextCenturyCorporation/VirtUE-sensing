#!/usr/bin/env bash

##
## Starts the desktop bridge sensor from within the docker container
##

_dsensor=/opt/sensor


# Figure out the advertised IP
_ip=`ip addr show eth0 | grep 'inet ' | awk '{print $2}' | cut -d / -f 1`

# Spin up our sensor
echo " [bash] Starting desktop listening bridge"
echo " [bash] env LISTEN_PORT : \"$LISTEN_PORT\""
echo " [bash] env STANDALONE  : \"$STANDALONE\""
echo " [bash] env VERBOSE     : \"$VERBOSE\""
echo " [bash] Discovered IP   : $_ip"


# Matches /docker-compose.yml
export CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

if [ -z "$LISTEN_PORT" ]
then
    echo " [bash] Environment variable LISTEN_PORT undefined!"
    exit 1
else
    export LISTEN_PORT
fi


if [ -n "$VERBOSE" ]
then
    echo " [bash] Verbose output enabled"
    #_verbose="--verbose"
    export VERBOSE
fi

if [ -n "$STANDALONE" ]
then
    echo " [bash] Desktop listening bridge starting in standalone mode"
    export STANDALONE
    python $_dsensor/bridge.py
else
    echo " [bash] Desktop listening bridge starting in production mode"
    
    # N.B. the keys indicate files, but they don't have to exist. It's
    # where the keys are written.

    python $_dsensor/bridge.py                         \
           --ca-key-path      $_dsensor/certs          \
           --public-key-path  $_dsensor/certs/cert.pem \
           --private-key-path $_dsensor/certs/cert-key.pem \
	   --sensor-host 0.0.0.0                       \
	   --sensor-advertised-host $_ip               \
	   --sensor-port 11005
fi
