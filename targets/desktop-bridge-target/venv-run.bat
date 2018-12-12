REM For use on Windows only
REM Run on the User Desktop machine
REM Switches to the Python virtual env for the desktop bridge sensor and runs it
REM Behavior is affected by environment variables (see below)

SETLOCAL ENABLEEXTENSIONS

@ECHO off
@ECHO Configuring execution environment . . .

SET VENVDIR=%HOMEPATH%\python-venv-desktop-bridge-sensor
SET CFSSL_SHARED_SECRET="de1069ab43f7f385d9a31b76af27e7620e9aa2ad5dccd264367422a452aba67f"

SET PYTHONUNBUFFERED=0
SET PYTHONVER=3.6.5

SET _sensor=%VENVDIR%\bridge.py
SET _args=

@ECHO  [cmd] Staring desktop listening bridge
@ECHO  [cmd] env LISTEN_PORT : "%LISTEN_PORT%"
@ECHO  [cmd] env STANDALONE  : "%STANDALONE%"
@ECHO  [cmd] env VERBOSE     : "%VERBOSE%"
@ECHO  [cmd] env PYPATH      : "%PYPATH%"

IF NOT DEFINED LISTEN_PORT (
   @ECHO  [cmd] Environment variable LISTEN_PORT undefined!
   GOTO :eof
)

IF DEFINED VERBOSE (
    @ECHO   [cmd] Verbose output enabled
)

IF NOT DEFINED STANDALONE (
    SET _args=--sensor-port 11050 --ca-key-path ./certs --public-key-path ./certs/cert.pem --private-key-path ./certs/cert-key.pem
)

PUSHD %VENVDIR%
.\Scripts\activate && ECHO %cd% && python --version && python %_sensor% %_args% && deactivate
POPD
