#!/bin/bash

make clean; make all
sudo insmod ./controller.ko ps_repeat=1 ps_timeout=1
read -n 1 -s -r -p "Press any key to continue"
echo ""

sudo dmesg | grep kernel-ps
sudo lsmod | grep controller
echo ""
read -n 1 -s -r -p "Press any key to continue"
echo ""
sudo rmmod controller
