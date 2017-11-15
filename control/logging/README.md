

# Testing

## Install Kafka binary

This is really so we have command line tools while testing:

```bash
wget http://apache.claz.org/kafka/1.0.0/kafka_2.12-1.0.0.tgz
tar -xvzf kafka_2.12-1.0.0.tgz
cd kafka_2.12-1.0.0
```

## Start Kafka - docker

```bash
./start.sh
```

## Create a topic

```bash
./bin/kafka-topics.sh --create --zookeeper localhost:2181 --replication-factor 1 --partitions 1 --topic mytopic
```

## List topics

```bash
./bin/kafka-topics.sh --list --zookeeper localhost:2181
```

## Start a producer

```bash
./bin/kafka-console-producer.sh --broker-list localhost:9092 --topic test2
```

## Start a consumer

```bash
./bin/kafka-console-consumer.sh --bootstrap-server localhost:9092 --topic test2 --from-beginning
```

