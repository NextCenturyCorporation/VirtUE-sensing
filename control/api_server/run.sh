#!/usr/bin/env bash

# get certificates
python3 /app/tools/get_certificates.py --cfssl-host cfssl --hostname sensing-api.savior.internal -d /app/certs

# make sure the database is up
echo "Checking for API Server database"
mix ecto.create

echo "Checking for API Server migrations"
mix ecto.migrate

# run the server
mix phx.server