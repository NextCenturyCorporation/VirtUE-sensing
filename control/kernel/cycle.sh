#!/bin/bash

make clean; make all
sudo insmod ./controller.ko ps_repeat=1 ps_timeout=1
read -n 1 -s -r -p "Press any key to continue"
sudo cat /var/log/messages | grep kernel-ps
sudo lsmod | grep controller
read -n 1 -s -r -p "Press any key to continue"
sudo rmmod controller
