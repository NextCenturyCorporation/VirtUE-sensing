#!/bin/sh

./bin/dockerized-run.sh stream --username root --filter-log-level debug --follow --since "100 minutes ago" "$@"