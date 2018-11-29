# Desktop Bridge Sensor - Overview

The Desktop Bridge sensor forwards sensor data to the sensing API
infrastructure. It accepts in inbound TCP/IP connection on a given
port, reads JSON sensing data from it, and forwards it to the API.

# Requirements

This sensor is built as a docker container and thus requires docker.

The JSON data sent to the sensor must be seperated by newlines, that
is, each JSON document must end in a newline character, although it
can also contain several newlines as well. This agreement speeds up
the parsing of the inbound JSON data.

# Build

To build the sensor's container, navigate to the `desktop-bridge-target' directory in a shell and run:

```bash
> ./build.sh
```

# Run

## Standalone

It may be useful to run the sensor while it isn't hooked up to the
rest of the API infrastructure, for instance, during development or
testing. To do so, run:

```bash
> LISTEN_PORT=0.0.0.0:5555 STANDALONE=1 VERBOSE=1 ./runme.sh
```

Then write JSON sensing data to the sensor by connecting to any
interface on the local machine on port 5555 (as shown).


## Production

The sensor can be started in production (non-standalone) mode, meaning
that it will forward JSON messages to the sensing infrastructure. To
launch it, run:

```bash
> LISTEN_PORT=0.0.0.0:5555 VERBOSE=1 ./runme.sh
```

And interact with it in the same way as in the Standalone config.


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
* API_MODE experimental - don't use (yet)