#!/usr/bin/env bash

# get certificates
python3 /app/tools/get_certificates.py --cfssl-host cfssl --hostname api -d /app/certs

# run the server
mix phx.server