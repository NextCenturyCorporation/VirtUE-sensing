#!/usr/bin/env bash

set -x

# Given to us for free...
_sniff=vif3.0


_image=virtue-savior/nfs-sensor-target:latest

_container=virtue-savior-nfs-sensor-target

_net=savior_default

# Remove old containers

#docker container ls -a | grep $_container | \
#    awk '{ print $1 }' | xargs docker container rm


# Build image
docker build -t $_image .

exit 0

# Build container --network=$_net
docker create \
       --name $_container $_image /bin/bash

# Connect the container to one network
docker network connect $_net $_container

# Run container, connecting it to host network as well

set +x
echo " *** Now run:"
echo "docker run -e NFS_SENSOR_SNIFF_INTERFACE=vif1.0  --network=host $_container"
echo " ***"

echo " *** Now run:"
echo "docker start $_container"
echo " ***"



###
#$ docker create --network=network1 --name container_name containerimage:latest
#$ docker network connect network2 container_name
#$ docker start container_name
