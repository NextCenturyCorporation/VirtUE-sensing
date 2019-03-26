
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

Copyright 2019 Two Six Labs

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Savior Project GPL 2.0
Copyright (C) 2019 Two Six Labs

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
