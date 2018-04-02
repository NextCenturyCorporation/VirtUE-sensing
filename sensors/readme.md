# Integrating Sensors - Certificate and Registration Cycles

Sensors that connect to the Sensing API must go through two workflows; the [Certificate Cycle](../CERTIFICATES.md) and the [Registration Cycle](../SENSORARCH.md). The Certificate Cycle is used to retrieve a public/private key pair from the Sensing API, which is required for all further interactions with the Sensing infrastructure. The Registration Cycle is how individual sensors register with the API and retrieve run time configuration.

# Configuring Sensors

# Installing Sensor Configurations

A simple command line tool is included with the Sensing API for finding and installing sensor configurations. Every sensor should have a `sensor_configurations.json` file defined that in turn defines which files contain configuration data for each sensor. See the [lsof sensor configuration directory](https://github.com/twosixlabs/savior/tree/master/sensors/sensor_lsof/config) for an example of defining configurations for a sensor.

The configuration tool is located in the `bin` directory, and should be run from the top level of the project. 

You can list which configurations are currently installed:

```bash
> ./bin/load_sensor_configurations.py list 
Sensor Configuration Tool
ps
  % os(linux) context(virtue)
    [            off / latest              ] - format(    json) - last updated_at(2018-02-01T20:35:33.251710)
    [            low / latest              ] - format(    json) - last updated_at(2018-02-01T20:35:33.328210)
    [        default / latest              ] - format(    json) - last updated_at(2018-02-01T20:35:33.173672)
    [           high / latest              ] - format(    json) - last updated_at(2018-02-01T20:35:33.397889)
    [    adversarial / latest              ] - format(    json) - last updated_at(2018-02-01T20:35:33.471361)
```

You can also install new sensor configurations. Let's try installing the configurations for the `kernel-ps` sensor:

```bash
> ./bin/load_sensor_configurations.py install -d ./sensors/sensor_kernel_ps/
Sensor Configuration Tool
% searching for configurations in [./sensors/sensor_kernel_ps/]
Installing os(linux) context(virtue) name(kernel-ps)
  = 5 configuration variants
  % installing component name(kernel-ps)
    = exists
  % install configuration level(default) version(latest)
    = exists
  % install configuration level(off) version(latest)
    = exists
  % install configuration level(low) version(latest)
    = exists
  % install configuration level(high) version(latest)
    = exists
  % install configuration level(adversarial) version(latest)
    = exists
```

All of the configurations already existed. Let's update them instead:

```bash
> ./bin/load_sensor_configurations.py install -d ./sensors/sensor_kernel_ps/ --update
Sensor Configuration Tool
% searching for configurations in [./sensors/sensor_kernel_ps/]
Installing os(linux) context(virtue) name(kernel-ps)
  = 5 configuration variants
  % installing component name(kernel-ps)
    == exists / updating
    Component name(kernel-ps) os(linux) context(virtue) installed successfully
    = updated
  % install configuration level(default) version(latest)
    == exists / updating
    Configuration level(default) version(latest) installed successfully
    = updated
  % install configuration level(off) version(latest)
    == exists / updating
    Configuration level(off) version(latest) installed successfully
    = updated
  % install configuration level(low) version(latest)
    == exists / updating
    Configuration level(low) version(latest) installed successfully
    = updated
  % install configuration level(high) version(latest)
    == exists / updating
    Configuration level(high) version(latest) installed successfully
    = updated
  % install configuration level(adversarial) version(latest)
    == exists / updating
    Configuration level(adversarial) version(latest) installed successfully
    = updated
```

That's all there is too it. You can find more command line options with the `--help` flag.

# Deploying Sensors

When deployed to the AWS environment, we use a port mapping mechanism to route data between the Sensing API, Sensing Infrastructure, and Virtue embedded sensors. Each Linux based sensor is run as a Xen guest, which has an IP address from the Dom 0/host DHCP server, which is separate from the AWS IP address space for the VPC. Because of this, the Sensing API cannot directly route traffic to Sensors. 

To get around this, the Dom0 will forward ports from the Dom0 interface into the DHCP network. Sensors will get certificates based on the Dom0 address and hostname (eventually individual hostnames that all route to the Dom0 IP address). The sensors need to be aware of how they'll be receiving data, and how to match up ports and addresses.

Each Virtue, therefor, should have a `/home/user/ports.properties` file, which looks like:

```ini
#
#localport=exteralport
hostname=ip-10-0-5-21.ec2.internal
11001=12001
11002=12002
11003=12003
```

This file lays out the `hostname` of the Dom0, which will become the hostnamr the Sensor advertises to the API. The numeric listings are a map of Virtue local ports to Dom0 ports. It can be read as "local port 11001 will receive forwarded data from Dom0 port 12001" for the first line.

This also necessitates that Sensors don't try and claim the same local port. To solve _this_ part of the problem, the various sensors are hard coded to use addresses, starting with 11001. 

## Sensor Port Assignments

Sensors in the Linux `context` (inside Virtues) will use the following local ports when running:

 - `lsof` sensor: `11001`
 - `ps` sensor: `11002`
