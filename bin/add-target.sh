#!/bin/bash
for var in "$@"
do
    docker run -d --rm  --network=apinet --name $var virtue-savior/demo-target
done