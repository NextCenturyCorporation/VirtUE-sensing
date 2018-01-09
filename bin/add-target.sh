#!/bin/bash
for var in "$@"
do
    docker run -d --rm  --network=savior_default --name $var virtue-savior/demo-target
done