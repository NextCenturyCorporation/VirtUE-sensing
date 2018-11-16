#!/bin/sh

./bin/dockerized-run.sh stream --all-sensors --filter-log-level debug --follow --since "100 minutes ago" "$@"
