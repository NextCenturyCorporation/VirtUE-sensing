# Spin up our sensors
echo "Starting Sensors"
c:/opt/sensor_startup/run_sensors.ps1

# Now start our service
c:/tmp/dropper.ps1
