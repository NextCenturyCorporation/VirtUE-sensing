#!/bin/bash

make clean; make all
sudo insmod ./controller.ko ps_repeat=2 ps_timeout=5
dmesg
sudo lsmod | grep controller
read -n 1 -s -r -p "Press any key to continue"
sudo rmmod controller
read -n 1 -s -r -p "Press any key to continue"
dmesg
