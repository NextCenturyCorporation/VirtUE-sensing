


#!/bin/bash

make rebuild
sudo rm /var/run/kernel_sensorcomm
sudo insmod ./controller.ko ps_repeat=1 ps_timeout=1 ps_level=2 lsof_level=2 \
     lsof_repeat=1 lsof_timeout=1
read -n 1 -s -r -p "Press any key to continue"
echo ""

sudo dmesg | grep -i "lsof"
read -n 1 -s -r -p "Press any key to continue"
echo ""
sudo dmesg | grep -i "kernel-ps"
sudo lsmod | grep controller
#echo "unloading..."
#sudo rmmod controller
#sudo lsmod | grep controller

