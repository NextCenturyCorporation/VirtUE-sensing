#!/usr/bin/env bash

##
## Runs the docker container that hosts the NFS sensor. For production
## use.
##

_nfs_iface=vif-nfs
_container=virtue-savior/nfs-sensor-target:latest

sudo docker run --network=host -e NFS_SENSOR_SNIFF_INTERFACE=$_nfs_iface $_container
