#!/usr/bin/env bash

set -x

_image=virtue-savior/desktop-bridge-sensor-target:latest

# Build image
sudo docker build -t $_image .
