# virtue
VirtUE Program


# Running the Sensing Architecture

## Start Kafka

```bash
cd control/logging
./start.sh
```

See [Start Kafka](control/logging/README.md#start-kafka-docker) for more info.


## Start the Sensing API

```bash
cd control/api_server
mix phoenix.server
```

See [Sensing API - Running](control/api_server/README.md) for more info.


## Start the Dummy sensor

```bash
cd sensors/dummy
. ./venv/bin/activate
python lsof_sensory.py --public-key-path ./cert/rsa_key.pub --private-key-path ./cert/rsa_key
```

See []() for more info.

## Start the Topic Consumer

Make sure to replace **$UUID** with the proper identifier for the newly started sensor, which can
be found by [listing the active Kafka topics](control/logging/README.md#list-topics) or by checking
the log messages produced when starting the Dummy sensor.

```bash
cd control/logging/kafka_2.12-1.0.0
./bin/kafka-console-consumer.sh --bootstrap-server localhost:9092 --topic $UUID --from-beginning
```

See [Start a consumer](contro/logging/README.md) for more info.