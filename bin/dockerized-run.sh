#!/bin/bash

echo "[dockerized-run]"

NETWORK=

if [ "$API_MODE" == "dev" ]; then
    echo "[running in DEVELOPMENT mode]"
    NETWORK="--network=apinet"
fi



docker run $NETWORK -e "CFSSL_SHARED_SECRET=de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f" --rm -it virtue-savior/virtue-security:latest bash /usr/src/app/run.sh "$@"
