#!/usr/bin/env bash

set -x

_image=virtue-savior/nfs-sensor-target:latest

# Build image
docker build -t $_image .
