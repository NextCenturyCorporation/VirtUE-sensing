#!/bin/sh

echo "[dockerized-run]"
docker run -e "CFSSL_SHARED_SECRET=de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f" --network="savior_default" --rm -it virtue-savior/virtue-security:latest bash /usr/src/app/run.sh "$@"