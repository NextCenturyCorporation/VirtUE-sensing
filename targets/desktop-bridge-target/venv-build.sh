#!/usr/bin/env bash

##
## Builds python3.6 virtualenv for desktop bridge sensor on Linux.
##
## On Ubuntu 16.04, these packages are required:
##   * python3.6
##   * python3.6-dev
##


# For debugging
#set -x


_pypath=/usr/bin/python3.6 # override with PYPATH
_venv_path=~/.python-venv-desktop-bridge-sensor

if [ -n "$PYPATH" ]
then
    _pypath=$PYPATH
fi

# Install virtualenv
pip3 install virtualenv
if [ $? -ne 0 ]; then
    echo "command failed"
    exit $?
fi

virtualenv -p $_pypath $_venv_path
if [ $? -ne 0 ]; then
    echo "command failed"
    exit $?
fi

source $_venv_path/bin/activate

pip3 install -r requirements/requirements_master.txt
if [ $? -ne 0 ]; then
    echo "command failed"
    exit $?
fi

pushd common_libraries
sh ./install.sh
if [ $? -ne 0 ]; then
    echo "command failed"
    exit $?
fi
popd

cp sensor/bridge.py $_venv_path/
cp run-venv.sh      $_venv_path/

deactivate
