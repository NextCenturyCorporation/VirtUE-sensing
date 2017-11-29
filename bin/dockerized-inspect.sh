#!/bin/sh

docker run --net=host -it virtue-savior/virtue-security:latest python /usr/src/app/virtue-security inspect --api-host 0.0.0.0 --api-port 4000 --username root