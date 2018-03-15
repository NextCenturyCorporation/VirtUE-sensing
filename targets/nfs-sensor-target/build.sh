#!/usr/bin/env bash

docker build -t virtue-savior/nfs-sensor-target-template:latest .

docker create --network=savior_default \
       --name virtue-savior-nfs-sensor-target \
       virtue-savior/nfs-sensor-target-template:latest

##docker network connect sa virtue-savior-nfs-sensor-target
