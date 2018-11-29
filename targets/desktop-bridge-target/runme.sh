#!/usr/bin/env bash

##
## Runs the docker container that hosts the desktop bridge sensor.
##
## Call for production use:
## LISTEN_PORT=0.0.0.0:xxxx ./runme.sh
##
## Call for standalone use:
## LISTEN_PORT=0.0.0.0:xxxx STANDALONE=1 VERBOSE=1 ./runme.sh
##

_container=virtue-savior/desktop-bridge-sensor-target:latest
_default_listen=0.0.0.0:3434

if [ -z "$LISTEN_PORT" ]
then
    _listen=$_default_listen
else
    _listen=$LISTEN_PORT
fi

if [ -n "$VERBOSE" ]
then
    _verbose="-e VERBOSE=1"
fi

if [ -n "$STANDALONE" ]
then
    _standalone="-e STANDALONE=1"
fi

if [ "$API_MODE" == "dev" ]; then
    echo "[bash] Running in DEVELOPMENT mode"
    NETWORK="--network=apinet"
else
    NETWORK="--network=host"
fi

echo "[bash] Listening on $_listen"
#sudo docker run -it --rm --network=host -e LISTEN_PORT=$_listen $_verbose $_standalone $_container
sudo docker run -it --rm $NETWORK -e LISTEN_PORT=$_listen $_verbose $_standalone $_container
