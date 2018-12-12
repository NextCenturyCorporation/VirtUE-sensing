#!/usr/bin/env bash

set -x

##
## Builds python3.6 virtualenv for desktop bridge sensor on Linux.
##
## On Ubuntu 16.04, these packages are required:
##   * python3.6
##   * python3.6-dev
##

_pypath=/usr/bin/python3.6 # override with PYPATH
_venv_path=~/.python-venv-desktop-bridge-sensor

if [ -n "$PYPATH" ]
then
    _pypath=$PYPATH
fi

# Install virtualenv
pip3 install virtualenv

virtualenv -p $_pypath $_venv_path

source $_venv_path/bin/activate

pip install -r requirements/requirements_master.txt

pushd common_libraries
sh ./install.sh
popd

cp sensor/bridge.py $_venv_path/
cp run-venv.sh      $_venv_path/

deactivate
