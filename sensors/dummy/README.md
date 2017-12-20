The Dummy sensor is a reference design for a wrapper around a command line sensor data stream. The wrapper
handles communications with the Sensing API, as well as message formatting and streaming to Kafka. For the
reference design, the output of a locally run `lsof` command is used as the message stream.

The `lsof` command generates approximately 10,000+ messages every 15 seconds on a moderately busy MacBook,
which is a decent volume for exercising the logging and sensing infrastructure in a development environment.

# Setup & Installation

The sensor wrapper code makes heavy use of Python 3 asynchronous features, and should be run on Python 3.5
or higher. Setup the virtual environment for the sensor with:

```bash
virtualenv --python=Python3 ./venv
. ./venv/bin/activate
pip install -r requirements.txt
./gen_cert.sh
```

# Running the Dummy sensor

The Dummy sensor cannot run in stand-alone mode - it requires an active Sensing API and Kafka instance. See
[Running the Sensing Architecture](../../README.md) for more information on running the components of the
sensing infrastructure.

The Dummy sensor can be run with:

```bash
python lsof_sensor.py --public-key-path ./cert/rsa_key.pub --private-key-path ./cert/rsa_key
```

# Sensor Communications

The reference implementation wrapper code handles the communications exchanges with the Sensing API and
the Kafka log streamer.

## RSA Keys

Each sensor uses a public/private RSA 4096bit key pair for encrypting connections to external services,
as well as using the public key as part of the authentication and sensor registration process. The Sensing
API is used to acquire both the Savior Root CA Public Key, as well as the sensor specific public/private key
pair. 

The workflows used for key acquisition and sensor registration are documented in the [Certificate Workflow](https://github.com/twosixlabs/savior/blob/master/CERTIFICATES.md) 
and [Sensor Lifecycle](https://github.com/twosixlabs/savior/blob/master/SENSORARCH.md). These two cycles
are used in the `main` method of the Sensor wrapper.


## Sensor Registration / Deregistration

When a sensor starts it must register itself with the Sensing API. As part of the registration process
the Sensing API will verify that it can reach the sensor via a side channel HTTP request. The reference
implementation maintains an asynchronous HTTP server which handles the side channel registration, among
other actuation requests from the Sensing API.

Each sensor is uniquely identified in the Sensing API by a combination of Sensor ID and Public Certificate. During
the side channel HTTP request initiated by the Sensing API during registration, the sensor verifies that 
it is being correctly called, and correctly identified by its Sensor ID.

When a sensor shuts down, either unexpectedly, by user intervention, or by other tear down processes, it
must make a best effort to _deregister_ itself with the Sensing API. This is done using the same Sensor ID
and Public Certificate pair used during registration. Sensors that are unable to deregister, for whatever
reason, will have their registration record automatically removed after the Sensing API fails to receive
a heartbeat message for more than 15 minutes.

## Heartbeat

TBD

## Sensor Actuation

Sensor actuation requests are received by the reference implementation over an asynchronous HTTP
server. This server is run as a series of _coroutines_, and should be used to manage the run time
of the underlying sensor.

## Message Streaming

Messages in the sensing infrastructure are streamed to sensor specific _topics_ in Kafka. The _topic_ and
a list of _bootstrap servers_ is received by the sensor as part of the sensor Registration process. The
format and contents of the messages streamed to Kafka is still _to be determined_.
 
# Development Notes

 - The asynchronous components of the reference implementation are managed by 
    the [curio](https://github.com/dabeaz/curio) Python library.
 - The `gen_cert.sh` script currently creates RSA public private keys, not x509 CSRs and certificates, this
    is to enable quicker development before the Sensing Certificate Authority is operational.