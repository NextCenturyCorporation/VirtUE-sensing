
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

We're going to start the API using a `docker-compose.yml` file located in the root of the project repository. This compose file defines, among other things, the PostGRES database that the API uses to store run time sensor meta data. 

The first time we run the API with `docker-compose up`, this database will get initialized. The PostGRES initialization will be successful, but the `docker-compose` log will quickly fill with error messages (heavily pruned output):

```bash
> docker-compose up
Starting savior_kafka_1 ... 
Starting savior_api_server_postgres_1 ... 
Starting savior_dropper_callback_1 ... 
Starting savior_cfssl_1 ... done
Starting savior_api_1 ... done
Starting savior_target_1_1 ... done
Attaching to savior_kafka_1, savior_dropper_callback_1, savior_api_server_postgres_1, savior_cfssl_1, savior_api_1, savior_target_1_1
api_server_postgres_1  | The files belonging to this database system will be owned by user "postgres".
api_server_postgres_1  | This user must also own the server process.
api_server_postgres_1  | 
api_server_postgres_1  | The database cluster will be initialized with locale "en_US.utf8".
...
api_server_postgres_1  | creating collations ... ok
api_server_postgres_1  | creating conversions ... ok
api_server_postgres_1  | creating dictionaries ... ok
...
api_1                  | ** (DBConnection.ConnectionError) connection not available because of disconnection
api_1                  |     (db_connection) lib/db_connection.ex:934: DBConnection.checkout/2
api_1                  |     (db_connection) lib/db_connection.ex:750: DBConnection.run/3
api_1                  |     (db_connection) lib/db_connection.ex:1141: DBConnection.run_meter/3
...
api_server_postgres_1  | LOG:  database system was shut down at 2018-02-19 20:43:14 UTC
api_server_postgres_1  | LOG:  MultiXact member wraparound protections are now enabled
api_server_postgres_1  | LOG:  database system is ready to accept connections
api_server_postgres_1  | LOG:  autovacuum launcher started
```

Stop the `docker-compose` with `ctrl-c`, and tear down the compose environment with `docker-compose down`. At this point most of the infrastructure has failed to start, but our PostGRES database has been primed, and a new directory (`./pgdata`) will be in the root directory of your checked out Savior repository.

We'll restart the Sensing APi again with `docker-compose up`, and we'll get further, with our API and supporting infrastructure starting, but our sensors reporting errors trying to register with the API (logs again heavily pruned):

```bash
> docker-compose up
Starting savior_kafka_1 ... 
Starting savior_api_server_postgres_1 ... 
Starting savior_dropper_callback_1 ... done
Starting savior_api_server_postgres_1 ... done
Starting savior_api_1 ... done
Starting savior_target_1_1 ... done
Attaching to savior_kafka_1, savior_dropper_callback_1, savior_cfssl_1, savior_api_server_postgres_1, savior_api_1, savior_target_1_1
cfssl_1                | 2018/02/19 20:48:58 [INFO] Initializing signer
target_1_1             | Starting Sensors
...
target_1_1             | Starting lsof(version=1.20171117)
target_1_1             | Sensor Identification
target_1_1             | 	sensor_id  == 0ea7e0c0-1933-46d3-aca9-5b10af4f221c
...
target_1_1             |   @ Waiting for Sensing API
target_1_1             |   ! Exception while waiting for the Sensing API (HTTPConnectionPool(host='api', port=17141): Max retries exceeded with url: /api/v1/ready (Caused by NewConnectionError('<urllib3.connection.HTTPConnection object at 0x7f10021f9208>: Failed to establish a new connection: [Errno 111] Connection refused',)))
target_1_1             |     ~ retrying in 0.50 seconds
...
api_1                  | [info] == Running ApiServer.Repo.Migrations.CreateConfigurations.change/0 forward
api_1                  | [info] create table configurations
api_1                  | [info] create index configurations_component_version_index
api_1                  | [info] create index configurations_component_version_level_index
...
api_1                  | [info] Running ApiServer.Endpoint with Cowboy using http://0.0.0.0:17141
api_1                  | [info] Running ApiServer.Endpoint with Cowboy using https://:::17504
...
target_1_1             | Couldn't register sensor with Sensing API
target_1_1             |   status_code == 400
target_1_1             | {"timestamp":"2018-02-19 20:49:21.122936Z","msg":"sensor(id=0ea7e0c0-1933-46d3-aca9-5b10af4f221c) failed registration with invalid field (default configuration = Cannot locate default configuration for lsof)","error":true}
```

Now we have a running API, albeit one that can't register sensors, as it's lacking any sensor configuration data. Keep the environment running, and move to a new terminal window, where we'll install the existing sensor configurations.


## Installing Sensor Configurations

Every sensor defines a [set of configuration files](sensors/readme.md#configuring-sensors) that define sensor characteristics at different _observation levels_. The _observation levels_ define how intrusive a sensor is, and range from **off** to **adversarial**.

The configuration files for sensors are colocated with each sensor, and are defined, along with metadata, in `sensor_configurations.json` files. Using the `load_sensor_configurations.py` tool in the `./bin` directory, we'll find and install these configurations in our local running Sensing API instance (pruned output for brevity):

```bash
 ./bin/load_sensor_configurations.py install 
Sensor Configuration Tool
% searching for configurations in [./]
Installing os(linux) context(virtue) name(ps)
  = 5 configuration variants
  % installing component name(ps)
    Component name(ps) os(linux) context(virtue) installed successfully
    = created
	...
  % install configuration level(high) version(latest)
    Configuration level(high) version(latest) installed successfully
    = created
  % install configuration level(adversarial) version(latest)
    Configuration level(adversarial) version(latest) installed successfully
    = created
```

We used the default command line options of `load_sensor_configurations.py` to target our local API instance, and scanned all of the repository for configurations. See the documentation for [loading sensor configurations](bin/readme.md#load_sensor_configurationspy) for more command line options.

We can verify that our configurations loaded with the `list` option:

```bash
>  ./bin/load_sensor_configurations.py list
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

With the configurations loaded, we're ready to restart the Sensing API compose environment and see sensors in action. Go back to the terminal running the `docker-compose` environment, and `ctrl-c` to stop the compose, and then tear down the environment with `docker-compose down`.


## Interacting with the Sensor API

With our PostGRES database intialized and sensor configurations loaded, we can run a full Sensing API environment with a Virtue running sensors. Start up the environment. A ton of log messages will fly by, but will eventually slow down as the infrastructure stabalizes (pruned for brevity):

```bash
> docker-compose up
Creating savior_dropper_callback_1    ... done
Creating savior_api_server_postgres_1 ... done
Creating savior_cfssl_1               ... done
Creating savior_kafka_1               ... done
Creating savior_api_server_postgres_1 ... 
Creating savior_api_1                 ... done
Creating savior_api_1                 ... 
Creating savior_target_1_1            ... done
Attaching to savior_api_server_postgres_1, savior_cfssl_1, savior_dropper_callback_1, savior_kafka_1, savior_api_1, savior_target_1_1
...
api_1                  | [info] Sent 200 in 17ms
target_1_1             |   + pinned API certificate match
target_1_1             | Synced sensor with Sensing API
```

Until the entire environent is up and responding, different containers will report warnings and errors. So long as the sensors get to the point of logging a `Synced sensor with Sensing API`, everything is functioning normally.

To interact with the API we'll use the `virtue-security` command line interface, which we can build with the [dockerized-build.sh](bin/readme.md#dockerized-buildsh) command from the `bin` directory:

```bash
> ./bin/dockerized-build.sh 
[building] virtue-security
Sending build context to Docker daemon  29.12MB
Step 1/10 : FROM python:3.6
...
Successfully tagged virtue-savior/virtue-security:latest
[building] demo-target
Sending build context to Docker daemon  105.5kB
Step 1/27 : FROM python:3.6
...
Successfully tagged virtue-savior/demo-target:latest
```

With that we've successfully built two containers - the `virtue-security` container, which provides all of the run time dependencies and libraries for the `virtue-security` command, and the container for the `demo-target` Virtue.

Now our Virtue is running, and the API and infrastructure are running, so let's interact with the API. Switch to a new terminal, where we'll work from the root of the repository. Start by inspecting the running Virtues with the pre-built command [dockerized-inspect.sh](bin/readme.md#dockerized-inspectsh):

```bash
> ./bin/dockerized-inspect.sh 
[dockerized-run]
Getting Client Certificate
Running virtue-security
{
    "timestamp": "2018-02-19 21:13:58.529968Z",
    "targeting_scope": "user",
    "targeting": {
        "username": "root"
    },
    "sensors": [
        {
            "virtue_id": "54ea2464-0cc3-4785-b85f-33009ea6ea7d",
            "username": "root",
            "updated_at": "2018-02-19T21:13:27.015944",
            "sensor_id": "bf417f69-1cb9-46c9-a640-1c2eca28c949",
            "public_key": "-----BEGIN CERTIFICATE-----\nMI...cs=\n-----END CERTIFICATE-----\n",
            "port": 11020,
            "last_sync_at": "2018-02-19T21:13:27.015350Z",
            "kafka_topic": "582d9148-d0ff-4ce3-b9ff-a7ecda7b0025",
            "inserted_at": "2018-02-19T21:09:25.779244",
            "has_registered": true,
            "has_certificates": true,
            "configuration_id": 6,
            "component_id": 2,
            "address": "5fe46e57cdad"
        }
    ],
    "error": false
}
```

You can also stream the sensor data from any sensor observing the **root** user:

```bash
>  ./bin/dockerized-stream.sh 
{"timestamp":"2017-11-29T16:19:22.324018","sensor":"e56d0901-fa8e-4ad5-96c8-ee8581819d40","message":"COMMAND PID TID USER   FD   TYPE             DEVICE SIZE/OFF    NODE NAME\n","level":"debug"}
{"timestamp":"2017-11-29T16:19:22.324136","sensor":"e56d0901-fa8e-4ad5-96c8-ee8581819d40","message":"python    1     root  cwd    DIR               0,72     4096  332393 /usr/src/app\n","level":"debug"}
...
```

The `virtue-security stream` command will continue receiving messages until you force quit the command (`ctrl-c` or the Windows equivalent).

# Running the Sensor Architecture - Docker Swarm

On our AWS dev, test, and production machines, we run most of the API and related services on top of Docker Swarm. See the [running on docker swarm](swarm.md) instructions for more details. 

# Running the Sensing Architecture - Native

The instructions for running natively are likely buggy - unless absolutely
necessary, run the API locally via Docker.

Running the Sensing architecture natively outside of Docker is cumbersome:

 - container startup scripts handle partial automation of the Certificate Authority
 - Runtime options are codified on the command line controls of the `docker-compose` containers
 - DNS and service naming are handled via Docker networking
 
If you still want to try and run the components natively, the original and incomplete instructions
follow:

## Start Kafka

```bash
cd control/logging
./start.sh
```

See [Start Kafka](control/logging/README.md#start-kafka-docker) for more info.


## Start the Sensing API

**TODO**: Update with the environment variable to use when running native, so Phoenix knows how to address Kafka.

```bash
cd control/api_server
mix phoenix.server
```

See [Sensing API - Running](control/api_server/README.md) for more info.


## Start the Dummy sensor

With this script we're using the `virtue-security` command line interface to the Sensing API to issue an **inspect** command, which is returning a list of sensor metadata. The real command hidden behind the `dockerized-inspect.sh` command looks like:

```bash
virtue-security inspect --username root
```

With the `--username` flag we're asking the Sensing API to scope our action to any Virtue's used by the user with username `root`. 

Sensors within Virtues send their logs to a Kafka instance in the Sensing infrastructure - let's make sure that the sensors we have active are really logging by using the [dockerized-stream.sh](bin/readme.md#dockerized-streamsh) command (again, pruned for brevity):

```bash
> ./bin/dockerized-stream.sh 
[dockerized-run]
Getting Client Certificate
Running virtue-security
{"timestamp":"2018-02-19T21:35:28.728212","sensor_name":"lsof-1.20171117","sensor_id":"bf417f69-1cb9-46c9-a640-1c2eca28c949","message":"COMMAND PID TID USER   FD   TYPE             DEVICE SIZE/OFF    NODE NAME\n","level":"debug"}
...
{"timestamp":"2018-02-19T21:35:28.728302","sensor_name":"lsof-1.20171117","sensor_id":"bf417f69-1cb9-46c9-a640-1c2eca28c949","message":"bash      1     root  cwd    DIR              0,149     4096   66171 /usr/src/app/kmods\n","level":"debug"}
{"timestamp":"2018-02-19T21:35:28.728335","sensor_name":"lsof-1.20171117","sensor_id":"bf417f69-1cb9-46c9-a640-1c2eca28c949","message":"bash      1     root  rtd    DIR              0,149     4096   66167 /\n","level":"debug"}
```

It may take a few seconds for messages to start streaming, and you can stop the stream with `ctrl-c`. You'll notice that every log message is a properly formatted and encoded JSON object. All of the sensors stream log messages to Kafka in **jsonl** format, encoded as UTF-8. For the curious, the actual `virtue-security` command hidden behind `dockerized-stream.sh` is:

```bash
virtue-security stream --username root --filter-log-level debug --follow --since "100 minutes ago"
```

Which selects all Virtues belonging to the user `root`, filters the logs to receive messages of level `debug` and higher, starts by replaying the last 100 minutes of messages, and the `--follow` flag means the `virtue-security` command will stay connected to the API and stream new messages as the sensors log them.