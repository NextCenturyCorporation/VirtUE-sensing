#!/usr/bin/env bash

sleep 7

# get certificates
python3 /app/tools/get_certificates.py --cfssl-host cfssl --hostname api -d /app/certs

# make sure the database is up
echo "Checking for API Server database"
mix ecto.create

echo "Checking for API Server migrations"
mix ecto.migrate

# run the server
mix phx.server