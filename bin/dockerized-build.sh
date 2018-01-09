#!/bin/bash

echo "[building] virtue-security"
cd control/virtue-security && ./build.sh
cd ../..

echo "[building] demo-target"
cd targets/demo-target && ./build.sh