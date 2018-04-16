#!/usr/bin/env bash

## DEVELOPMENT ONLY
##
## Runs the docker container that hosts the NFS sensor. For testing
## when NFS traffic cannot be generated across the VIF of
## interest. Reads packets from the given PCAP file and emits messages
## to the sensing API server.
##
## DEVELOPMENT ONLY


_container=virtue-savior/nfs-sensor-target:latest

if (( $# != 1 )); then
    echo "Usage: $0 file.pcap"
    exit 1
fi

_real=$(readlink -f $1)
_dir=$(dirname $_real)
_base=$(basename $_real)

_mapdir=/pcap
_map_pcap=$_mapdir/$_base

sudo docker run --network=host \
     -v $_dir:$_mapdir \
     -e NFS_SENSOR_PCAP_FILE=$_map_pcap \
     $_container
