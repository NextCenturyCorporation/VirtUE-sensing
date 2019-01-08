# Desktop Bridge Sensor - Overview

The Desktop Bridge sensor forwards sensor data to the sensing API
infrastructure. It accepts in inbound TCP/IP connection on a given
port, reads JSON sensing data from it, and forwards it to the API. The
tool is to be used via shell: either the Linux command line shell, or
the Windows command prompt.

# Requirements

This sensor is built as a docker container and thus requires docker.

The JSON data sent to the sensor must be seperated by newlines, that
is, each JSON document must end in a newline character, although it
can also contain several newlines as well. This agreement speeds up
the parsing of the inbound JSON data.

# Build

This capability can be build into a Python vitual environment or a Docker container.

To build the sensor's container, navigate to the `desktop-bridge-target' directory in a shell and run:

```bash
> ./docker-build.sh
```

To build into a Python virtual environment, run:

```bash
> ./venv-build.sh
```

or on Windows:

```bash
> ./venv-build.bat
```


# Run

The sensor can be run in standalone or production mode. Standalone
mode emits messages only to stdout, whereas production mode sends
messages to the API server. Standalone mode is useful for debugging
but should not be used in production.

To run in standalone mode, the environment variable STANDALONE must be
defined (and must not evaluate to a false value). Enable standalone
mode with:

```bash
> export STANDALONE=1
```

or on Windows:
```bash
> set STANDALONE=1
```

The sensor will listen on a port for an incoming connection, and will
read JSON messages off that connection. The port must be specified via
the LISTEN_PORT variable, for instance:

```bash
> export LISTEN_PORT=0.0.0.0:5555
```

Verbose logging can be enabled via the VERBOSE variable, e.g.
```bash
> export VERBOSE=1
```


To run the tool via Python virtual environment, use either
`venv-run.sh` or `venv-run.bat`. To run it via Docker (Linux only) use
`docker-run.sh`.


# Options

To avoid confusion with command-line arguments used by the sensor
wrapper, the bridge sensor uses environment variables to define its
behavior. These environment variables are passed to the runme.sh
script:

* LISTEN_PORT=a.b.c.d:xxxx the interface and port the sensor should
  listen on for sensing data
* VERBOSE if non-empty, enables verbose logging
* STANDALONE if non-empty, runs in standalone mode; otherwise runs in
  production mode