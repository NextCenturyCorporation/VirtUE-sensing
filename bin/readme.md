The `bin` directory contains tools and scripts for building and interacting with the Sensing API and it's infrastructure. These scripts are designed to be called from the root directory of the repository, so call them like this:

```bash
./bin/dockerized-inspect.sh
```

not like:

```bash
cd bin
./dockerized-inspect.sh
```

While a handful of the scripts may act normally when called in the `bin` directory, others will fail with strange errors.

The following scripts from the `bin` directory are documented, and used during various phases of development and deployment of the Sensing and Response tools:

 - [add_target.sh](#add_targetsh) - Add one or more new Virtues to a running Sensing environment.
 - [clear_network.sh](#clear_networksh) - Remove containers from a running Sensing environment that aren't part of the Sensing infrastructure.
 - [dockerized-build.sh](#dockerized-buildsh) - Build the `virtue-security` CLI tool and stand-alone Virtue images.
 - [dockerized-inspect.sh](#dockerized-inspectsh) - Run the **inspect** command on a running Sensing environment. 
 - [dockerized-run.sh](#dockerized-runsh) - Run any of the `virtue-security` commands on a running Sensing environment.
 - [dockerized-stream.sh](#dockerized-streamsh) - **stream** live log messages from a running Sensing environment.
 - [install_sensors.py](#install_sensorspy) - Install sensors and associated files/data in defined Virtue targets.
 - [load_sensor_configurations.py](#load_sensor_configurationspy) - Load the configuration files for one or more sensors into a running Sensing API.
 - [update_tools.sh](#update_toolssh) - Install various support tools into multiple directories.

# add_target.sh

Add one or more instances of the [demo-target](../targets/demo-target/) Virtue to a running Sensing API. Call the script with the names to use for the new Virtues:

```bash
./bin/add_target.sh demo_1 demo_2 demo_3
```

This would add three new Virtues (containers) to the Sensing environment, named `demo_1`, `demo_2`, and `demo_3` respectively. These containers aren't managed by the Compose file used to create the Sensing environment, so you need to use the `clear_network.sh` script (see below) to remove these containers before you can completely tear down a Sensing environment.

This script is especially useful when running a demo or debugging a Virtue and it's sensors, as you can wait for the API to stabalize and log traffic to calm down, before adding new Virtues. 

Use the [dockerized-build.sh](#dockerized-build.sh) script to build the **demo-target** Docker image used for these Virtues before adding targets.

# clear_network.sh

Remove any containers from the `savior_default` nework that aren't managed by the `docker-compose.yml` file that defines the Sensing environment. Run without any parameters:

```bash
./bin/clear_network.sh
```

This will remove any containers from the `savior_default` network whose name doesn't begin with `savior_`. Use in combination with the `add_target.sh` script to add and remove Virtue instances from a running Sensing environment.

# dockerized-build.sh

Build supporting tools for the Sensing API. Some of our tools aren't automatically built when starting a Sensing environment via `docker-compose`, so you can build them with:

```bash
./bin/dockerized-build.sh
```

In particular this will build the [virtue-security](../control/virtue-security/) command line tool and the [demo-target](../targets/demo-target/) Virtue. The **demo-target** _is_ built by Docker when a new Sensing environment is started, but the built image isn't tagged in a useful way. By explicity building and tagging the **demo-target** image, we make it easier to develop, build, and add Virtues to a Sensing environment.

# dockerized-inspect.sh

Run a pre-written **inspect** command via the `virtue-security` command line tool against a running Sensing environment. This script utilizes the `virtue-security` Docker image built via the [dockerized-build.sh](#dockerized-build.sh) script.

This script will query the running Sensing environment for metadata about any Virtues for the `root` user.

```bash
./bin/dockerized-inspect.sh
```

This script is a proxy to the `dockerized-run.sh` script, and is equivalent to the `dockerized-run.sh` command:

```bash
./bin/dockerized-run.sh inspect --username root
```

# dockerized-run.sh

Use the `virtue-security` Docker image to act on the running Sensing environment. Any and all commands supported by [virtue-security](https://github.com/twosixlabs/savior/tree/master/control/virtue-security) can be run using this command, just beware that because the actual `virtue-security` tool is inside a Docker image, any command that require paths to specific files will fail. 

Use this script by replacing the `virtue-security` portion of any [virtue-security](../control/virtue-security/) command with `./bin/dockerized-run.sh`, for example:

```bash
virtue-security inspect --username root
```

becomes:

```bash
./bin/dockerized-run.sh inspect --username root
```

Normal command line argument quoting and escaping should function normally, and is handled by both the `dockerized-run.sh` script and the [run.sh](https://github.com/twosixlabs/savior/blob/master/control/virtue-security/run.sh) script in the `virtue-security` Docker image.

## Observe Action

Use the base `dockerized-run.sh` command to execute **observe** actions in the Sensing API:

```bash
./bin/dockerized-run.sh observe --username roo --level adversarial
```

This is in lieu of a stand-alone `dockerized-observe.sh` script.

# dockerized-stream.sh

Streaming sensor data from the Sensing API is a multi-step process, thankfully managed by the `virtue-security` **stream** action. A pre-built streaming command is defined in the `dockerized-stream.sh` script, and can be run with:

```bash
./bin/dockerized-stream.sh
```

This is equivalent to the `virtue-security` command:

```bash
virtue-security stream --username root --filter-log-level debug --follow --since "100 minutes ago" "$@"
```

Some useful **stream** parameters:

 - `--follow`: Stay connected to the API and retrieve new sensor data as it is reported.
 - `--since`: Start the sensor data stream at a certain point in time.
 - `--filter-log-level`: Only stream sensor data at or above a specific log level (Options are `everything`, `debug`, `info`, `warning`, `error`, and `event`. Default is `everything`).
 - `--username`: Stream data from any sensors monitoring Virtues belonging to a specific user.
 - `--sensor-id`: Stream data from a specific sensor, identified by the sensor UUID.
 
# install_sensors.py

Target Virtues (like the [demo-target](../targets/demo-target) Virtue) define their required sensors in a [target.json](https://github.com/twosixlabs/savior/blob/master/targets/demo-target/target.json) file, under the `sensors` key. 

Each sensor defines a [sensor.json](https://github.com/twosixlabs/savior/blob/master/sensors/sensor_lsof/sensor.json) file, which in turn describes the files, modules, and packages required for the sensor. 

The `install_sensors.py` command enumerates all of the Targets defined by `target.json` files, and all of the Sensors defined by `sensor.json` files, matching up definitions and requirements to make sure all required sensors exist. 

With these definitions in hand, the `install_sensors.py` command will copy files into place in the source directory of each Target, build necessary support files (like a common `requirements.txt`, and `apt-get install` scripts), and copy support libraries into each Target. Then, based on specific settings in the `target.json`, and specific commands in the Target **Dockerfile**, the target _Virtue_ is buildable via a standard Docker workflow.

## Installing Sensors

Run the sensor installer directly from the repository root:

```bash
> ./bin/install_sensors.py 
Running install_sensors
  wrapper(./sensors/wrapper)
  sensors(./sensors)
  targets(./targets)
  kernel_mods(./)

Finding Sensors
  + ps-sensor (version 1.0)
  + lsof-sensor (version 1.0)
  + kernel-ps-sensor (version 1.0)

Finding kernel modules
  + kernel-ps (version 2018-01-09)

Finding Targets
  + site-visit-demo-target

Installing 1 sensors in target [site-visit-demo-target]
  ~ Preparing directories
    - removing [sensors_directory] (./sensors)
    + creating [sensors_directory] (./sensors)
    - removing [requirements_directory] (./requirements)
    + creating [requirements_directory] (./requirements)
    - removing [startup_scripts_directory] (./sensor_startup)
    + creating [startup_scripts_directory] (./sensor_startup)
    - removing [library_directory] (./sensor_libraries)
    + creating [library_directory] (./sensor_libraries)
    - removing [kernel_mod_directory] (./kmods)
    + creating [kernel_mod_directory] (./kmods)
  + installing Sensor Wrapper library
    + library files
    + requirements.txt file
  + installing lsof-sensor (version 1.0)
    + run time files
    + run time directories
    + startup script
    + requirments.txt files
  + Creating requirements_master.txt to consolidate required libraries
  + Building support library install script
    % Scanning for pip install targets
    + Writing install script
  + Building kernel module install script
  + Writing install file
  + Building sensor startup master script
    + 1 startup scripts added
  + Finding apt-get requirements for installed sensors
    = Found 1 required libraries
```

In this example the installer found 3 sensors, 1 kernel module, and 1 target. The target defined a single sensor requirement (the `lsof-sensor`), which the installer copied into the target, along with support libraries, requirements files, and run time scripts.

## Development Workflow

Working with sensors and `install_sensors.py` necessitates a certain workflow when building and debugging sensors:

 1. Create/update the `sensor.json` for the sensor under development
 2. Make sure sensor is in the required sensor list in the Target's `target.json`
 3. Develop/alter sensor
 4. Run `./bin/install_sensors.py` to install updated sensor code into Target
 5. Run `./bin/dockerized-build.sh` to build Target Docker image
 6. Add target to Sensing environment with `./bin/add_target.sh <target_name>`
 7. Debug
 8. Remove target from Sensing environment with `./bin/clear_network.sh`
 9. Goto 3


# load_sensor_configurations.py

Every sensor defines a set of configuration files that cover all of the observation levels required of each sensor. The set of config files for a sensor is defined in a `sensor_configurations.json` file. 

Before a running sensor can successfully register with the Sensing API, a configuration must exist for that sensor. The configuration files can be loaded into a fresh instance of the Sensing API via the `load_sensor_configurations.py` tool:

```bash
> ./bin/load_sensor_configurations.py
```

By default the command will scan the entire repository for sensor configuration definitions. If you want to limit where the command looks for configurations, use the `--directory` or `-d` flag:

```bash
>  ./bin/load_sensor_configurations.py  --directory sensors/sensor_lsof/
Sensor Configuration Tool
% searching for configurations in [sensors/sensor_lsof/]
Installing os(linux) context(virtue) name(lsof)
  = 5 configuration variants
  % installing component name(lsof)
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

## Listing Loaded Configurations

The `load_sensor_configurations.py` tool can take an optional action parameter - in fact it uses a default implied **install** action in the above examples. The **list** action let's us inspect the Sensing API to find currently loaded configurations:

```bash
> ./bin/load_sensor_configurations.py list
Sensor Configuration Tool
ps
  % os(linux) context(virtue)
    [            off / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:13.702450)
    [            low / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:13.768332)
    [        default / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:13.609205)
    [           high / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:13.837523)
    [    adversarial / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:13.908156)
lsof
  % os(linux) context(virtue)
    [            off / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.132824)
    [            low / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.224955)
    [        default / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.049705)
    [           high / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.293504)
    [    adversarial / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.365097)
kernel-ps
  % os(linux) context(virtue)
    [            off / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.573557)
    [            low / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.638121)
    [        default / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.502159)
    [           high / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.703862)
    [    adversarial / latest              ] - format(    json) - last updated_at(2018-02-19T21:01:14.772859)
```

Here we have three sensors with loaded configurations, each sensor having 5 different configurations available. 

## Updating Sensor Configurations

To overwrite or update an existing configuration in the Sensing API, use the `--update` flag with the install action:

```bash
> ./bin/load_sensor_configurations.py  --directory sensors/sensor_lsof/ --update
Sensor Configuration Tool
% searching for configurations in [sensors/sensor_lsof/]
Installing os(linux) context(virtue) name(lsof)
  = 5 configuration variants
  % installing component name(lsof)
    == exists / updating
    Component name(lsof) os(linux) context(virtue) installed successfully
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

## Loading Remote APIs

The examples for `load_sensor_configurations.py` so far are implicitly targeting a Sensing API running on `localhost`. The following parameters can be used to control where, and how, to connect to a Sensing API instance:

 - `--api-host`: Hostname of the API. (Default is `localhost`)
 - `--api-port-https`: TLS Secured HTTP port for the API. (Default is `17504`)
 - `--api-port-http`: Standard HTTP port for the API. (Default is `17141`)
 - `--api-version`: Version of the API being contacted. (Default is`v1`)

# update_tools.sh

Some scripts and tools are shared by multiple components within the Sensing and Response repository. To try and make life a bit more sane, the `update_tools.sh` script should be used to control distribution of common code and tools throughout the repository.

Currently the only tool distributed in this manner is the [Certificate Tools](https://github.com/twosixlabs/savior/tree/master/tools/certificates) code, which is used by infrastructure bootstrap processes to acquire TLS Client and Server certificates directly from CFSSL, rather than using the Sensing API certificate endpoints.

Run this tool directly from the repository root:

```bash
> ./bin/update_tools.sh 
Updating Certificate tools
 ... [control/api_server/tools]
 ... [control/logging/tools]
 ... [control/virtue-security/tools]
```

