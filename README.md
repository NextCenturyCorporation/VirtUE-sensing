
The Two Six Labs private repository for work on the Next Century led SAVIOR project, implementing a solution to the IARPA VirtUE Program.

Two Six Labs is responsible for the Sensing and Response in the SAVIOR archirtecture, including dynamic sensing and actuation of Linux, Windows, and Unikernels. The Sensing and Response work is roughly broken into:

 - Sensing API
 - Sensing Infrastructure
 - Linux Kernel Sensing
 - Windows Kernel Sensing
 - Unikernel Sensing
 - Linux User Space Sensors

Refer to the individual readmes linked above, or use the **Getting Started** section below. For more information on the run time tools for interacting with a running Sensing environment, see the [bin tools](bin/readme.md) readme.

# Getting Started with the Sensing API

The API for registering, controlling, and actuating sensors within Virtues is containerized, along with all of the supporting infrastructure for running the API. The various tools for working with the API from the command line are also embedded in Docker containers. During development Virtues are nothing more than Docker containers with our sensors (and possibly exploits/test code) installed and configured. 

In the deployed Virtue environment these Virtues are stand-alone virtual machines, with the appropriate sensors installed and configured. While this changes the method of integrating sensors from that used in development, the lifecycle of sensors, and sensor interaction with the Sensing API, is the same.


## Starting the API

Refer to either of the following guides for running the API:

 - [Development Mode](dev.md) - run the API locally for development of API and sensor capabilities
 - [Production Mode](swarm.md) - run the Sensing API on a multi-node Docker Swarm environment on AWS

## Installing Sensor Configurations

Every sensor defines a [set of configuration files](sensors/readme.md#configuring-sensors) that define sensor characteristics at different _observation levels_. The _observation levels_ define how intrusive a sensor is, and range from **off** to **adversarial**.



## Interacting with the Sensor API

See the various `dockerized-` commands in the `bin` directory for interacting with the running Sensing API via the [virtue-security](control/virtue-security) tool:

 - [dockerized-inspect.sh](bin/readme.md#dockerized-inspectsh) - View data about the running sensors
 - [dockerized-observe.sh](bin/readme.md#dockerized-observesh) - Change the sensor observation level
 - [dockerized-run.sh](bin/readme.md#dockerized-runsh) - Pass through for any `virtue-security` command
 - [dockerized-stream.sh](bin/readme.md#dockerized-streamsh) - Stream log data from the API



`updated 2018-05-01T16:34:00EST`
