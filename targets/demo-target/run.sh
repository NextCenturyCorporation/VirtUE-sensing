#!/usr/bin/env bash

# Spin up our sensors
echo "Starting Sensors"
/bin/bash /opt/sensor_startup/run_sensors.sh

# Now start our service
/bin/bash /tmp/dropper.sh