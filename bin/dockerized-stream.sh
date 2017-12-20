#!/bin/sh

docker run --network="savior_default" -it virtue-savior/virtue-security:latest python /usr/src/app/virtue-security stream --username root --filter-log-level debug --follow --since "100 minutes ago"