#!/usr/bin/env bash
make clean
make all
insmod ./controller.ko ps_repeat=10000 ps_timeout=10
