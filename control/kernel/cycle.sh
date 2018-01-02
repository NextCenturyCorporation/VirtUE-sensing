#!/bin/bash

make clean; make all
sudo insmod ./controller.ko
dmesg
sudo lsmod | grep controller
read -n 1 -s -r -p "Press any key to continue"
sudo rmmod controller
read -n 1 -s -r -p "Press any key to continue"
dmesg
