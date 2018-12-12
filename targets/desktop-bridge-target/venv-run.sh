#!/usr/bin/env bash

set -x

##
## Runs desktop bridge sensor in python3.6 virtualenv.
##
## On Ubuntu 16.04, these packages are required:
##   * python3.6
##   * python3.6-dev
##

_pypath=/usr/bin/python3.6 # override with PYPATH
_venv_path=~/.python-venv-desktop-bridge-sensor
_sensor=./bridge.py # relative to $_venv_path

export CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

echo " [bash] Starting desktop listening bridge"
echo " [bash] env LISTEN_PORT : \"$LISTEN_PORT\""
echo " [bash] env STANDALONE  : \"$STANDALONE\""
echo " [bash] env VERBOSE     : \"$VERBOSE\""
echo " [bash] env PYPATH      : \"$PYPATH\""


if [ -n "$PYPATH" ]
then
    _pypath=$PYPATH
fi

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

pushd $_venv_path

virtualenv -p $_pypath $_venv_path
source bin/activate

if [ -n "$STANDALONE" ]
then
    echo " [bash] Desktop listening bridge starting in standalone mode"
    export STANDALONE
    python $_sensor
else
    echo " [bash] Desktop listening bridge starting in production mode"
    
    # N.B. the keys indicate files, but they don't have to exist. It's
    # where the keys are written.

    mkdir certs
    python $_sensor                            \
           --ca-key-path      ./certs          \
           --public-key-path  ./certs/cert.pem \
           --private-key-path ./certs/cert-key.pem
fi

deactivate
popd
