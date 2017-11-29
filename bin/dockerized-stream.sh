#!/bin/sh

docker run --net=host -it virtue-savior/virtue-security:latest python /usr/src/app/virtue-security stream --api-host 0.0.0.0 --api-port 4000 --username root --filter-log-level debug --follow --since "100 minutes ago"