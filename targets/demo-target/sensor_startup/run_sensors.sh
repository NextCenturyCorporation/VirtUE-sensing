#!/bin/bash
/opt/sensor_startup/run_lsof_sensor.sh &
/opt/sensor_startup/run_netstat_sensor.sh &
/opt/sensor_startup/run_ps_sensor.sh &
/opt/sensor_startup/run_tcpdump_sensor.sh &
