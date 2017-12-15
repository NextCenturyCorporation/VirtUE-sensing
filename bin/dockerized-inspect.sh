#!/bin/sh

docker run --network="savior_default" -it virtue-savior/virtue-security:latest python /usr/src/app/virtue-security inspect --username root