#!/bin/bash

make clean; make all
sudo insmod ./controller.ko
read -n 1 -s -r -p "Press any key to continue"
echo ""
#sudo cat /var/log/messages | grep kernel-ps
sudo dmesg | grep kernel-ps
sudo lsmod | grep controller
echo ""
read -n 1 -s -r -p "Press any key to continue"
echo ""
sudo rmmod controller
