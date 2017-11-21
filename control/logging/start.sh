#!/usr/bin/env bash

KAFKA_ADVERTISED_HOST_NAME=`ifconfig | sed -En 's/127.0.0.1//;s/.*inet (addr:)?(([0-9]*\.){3}[0-9]*).*/\2/p' | head -n 1`

echo "Starting zookeeper on $KAFKA_ADVERTISED_HOST_NAME:2181"
echo "Starting kafka on $KAFKA_ADVERTISED_HOST_NAME:9092"

echo "  producer --broker-list = $KAFKA_ADVERTISED_HOST_NAME:9092"
echo "  consumer --bootstrap-server = $KAFKA_ADVERTISED_HOST_NAME:9092"

docker run --rm -d --name savior-kafka -p 2181:2181 -p 9092:9092 --env ADVERTISED_HOST=$KAFKA_ADVERTISED_HOST_NAME --env ADVERTISED_PORT=9092 spotify/kafka