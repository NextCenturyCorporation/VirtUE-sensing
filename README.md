# virtue
VirtUE Program


# Running the Sensing Architecture - Docker

The components of the Sensing API can be run as Docker containers to make demonstrations and tests easier
to execute in a wide variety of environments. The infrastructure services will all be automatically built
when the `docker-compose.yml` file is used to start the services, but the container for the `virtue-security`
tool needs to be built prior to use. All of the following commands are designed to be run from the repository
root.

Build `virtue-security`:

```bash
> ./bin/dockerized-build.sh
```

Once `virtue-security` is build and you're ready to run everything, spin up the infrastructure using `docker-compose`:

```bash
> docker-compose up
```

In another terminal you can run `virtue-security inspect` to see meta data on sensors observing the user **root**:

```bash
>  ./bin/dockerized-inspect.sh 
{
    "timestamp": "2017-11-29 16:18:20.174126Z",
    "targeting_scope": "user",
    "targeting": {
        "username": "root"
    },
    "sensors": [
        {
            "virtue": "57d2ebdd-a12b-48c4-9139-e041a671f838",
            "username": "root",
            "timestamp": "2017-11-29 16:16:29.777185Z",
            "sensor_name": "lsof-sensor-1.20171117",
            "sensor": "50003ef9-5fed-434d-96f9-48f36acb2bac",
            "public_key": "-----BEGIN PUBLIC KEY-----\nMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAvzXY/MhfQxaj4RfJAPHQ\nWLhOGmzkUGjj81nrrM6HCaVSCdeZDlI13j41EnCWO+NrYZ3s1/MBMbo6fEzHbHlp\nasked5sMvlReGErelttteCa2c62NAlQ2BIR5iuXZ1nn9WzcGVsQIV92exnl65pza\nftyf4rjrTHpWBDJtJJf+W2piASOyiPf98UxYqIGlaYI6TFpPobwmuEhzm00r2mwC\neqcqQiU1bKF3qiAhO4kMAjYV/GMqCje38EicTJ0FLLIFW6NZUcD7tjFB4hU3j8Nn\nC7+URvMgOnaCOiqiidzWzVIfpOsvYvFc9D+BXcmYYIJngs58kJdc9mO7eZs3d0D6\nLje9fmvf+PnLI2nKXxxxbkspOs+DQiuJPMT7yHohj67d2JDA2qU4F3VKxmu3Vwo7\nqNz3q5SEHK8BIC+MYE5lT26a26UQgObCm/Qmw8N769hhLszHWiFmJIMtLtXCAJDB\nou7CZveKsRsQFjwKTQKKKEhyNaDLh4wfNhgN8M5thKIXeEMwxQkEtey4/NqmIChy\nZ5RMK8/LpOEm5kDkCPvpg3jrQldiGHI30rxWFr7Ox7f1ofM6X6WY1+USYQ5n7nQB\nTHuf4CImw5YQl3zTcCTf1SGEaSpf0joa/W4jx3rFCC8YkrLzkUNgjRAdEv5B57fB\n893XY3WxTRahV3oi21noSGUCAwEAAQ==\n-----END PUBLIC KEY-----\n",
            "port": null,
            "kafka_topic": null,
            "address": "6cb32f5a1dc3"
        },
        {
            "virtue": "b77ac1e5-f135-4b0d-8129-571d902367de",
            "username": "root",
            "timestamp": "2017-11-29 16:16:29.808949Z",
            "sensor_name": "lsof-sensor-1.20171117",
            "sensor": "e56d0901-fa8e-4ad5-96c8-ee8581819d40",
            "public_key": "-----BEGIN PUBLIC KEY-----\nMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAvzXY/MhfQxaj4RfJAPHQ\nWLhOGmzkUGjj81nrrM6HCaVSCdeZDlI13j41EnCWO+NrYZ3s1/MBMbo6fEzHbHlp\nasked5sMvlReGErelttteCa2c62NAlQ2BIR5iuXZ1nn9WzcGVsQIV92exnl65pza\nftyf4rjrTHpWBDJtJJf+W2piASOyiPf98UxYqIGlaYI6TFpPobwmuEhzm00r2mwC\neqcqQiU1bKF3qiAhO4kMAjYV/GMqCje38EicTJ0FLLIFW6NZUcD7tjFB4hU3j8Nn\nC7+URvMgOnaCOiqiidzWzVIfpOsvYvFc9D+BXcmYYIJngs58kJdc9mO7eZs3d0D6\nLje9fmvf+PnLI2nKXxxxbkspOs+DQiuJPMT7yHohj67d2JDA2qU4F3VKxmu3Vwo7\nqNz3q5SEHK8BIC+MYE5lT26a26UQgObCm/Qmw8N769hhLszHWiFmJIMtLtXCAJDB\nou7CZveKsRsQFjwKTQKKKEhyNaDLh4wfNhgN8M5thKIXeEMwxQkEtey4/NqmIChy\nZ5RMK8/LpOEm5kDkCPvpg3jrQldiGHI30rxWFr7Ox7f1ofM6X6WY1+USYQ5n7nQB\nTHuf4CImw5YQl3zTcCTf1SGEaSpf0joa/W4jx3rFCC8YkrLzkUNgjRAdEv5B57fB\n893XY3WxTRahV3oi21noSGUCAwEAAQ==\n-----END PUBLIC KEY-----\n",
            "port": null,
            "kafka_topic": null,
            "address": "114cede6f773"
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


# Running the Sensing Architecture - Native

During development, it is recommended that you run the Sensing architecture natively, so code changes, bugs, and issues
aren't missed due to Docker caching and Docker network shims.

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

```bash
cd sensors/dummy
. ./venv/bin/activate
pip install -r requirements.txt
python lsof_sensory.py --public-key-path ./cert/rsa_key.pub --private-key-path ./cert/rsa_key
```

See [Sensory Documentation](sensors/dummy/README.md) for more info.

## Stream with virtue-security 

Make sure to replace **$UUID** with the proper identifier for the newly started sensor, which can
be found by [listing the active Kafka topics](control/logging/README.md#list-topics) or by checking
the log messages produced when starting the Dummy sensor.

```bash
cd control/virtue-security
. ./venv/bin/activate
pip install -r requirements.txt
python virtue-security stream --sensor-id $UUID --since "100 minutes ago" --follow --filter-log-level info
```

## Start the Topic Consumer

You can also subscribe directly to Kafka with a command line consumer.

Make sure to replace **$UUID** with the proper identifier for the newly started sensor, which can
be found by [listing the active Kafka topics](control/logging/README.md#list-topics) or by checking
the log messages produced when starting the Dummy sensor.

```bash
cd control/logging/kafka_2.12-1.0.0
./bin/kafka-console-consumer.sh --bootstrap-server localhost:9092 --topic $UUID --from-beginning
```

See [Start a consumer](contro/logging/README.md) for more info.