#!/bin/bash

make clean; make all
sudo insmod ./controller.ko ps_repeat=10000 ps_timeout=10
sudo cat /var/log/messages | grep kernel-ps
#sudo lsmod | grep controller
#read -n 1 -s -r -p "Press any key to continue"
sudo rmmod controller
