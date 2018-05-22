These instructions detail how to run the Sensing API, Dockerized target machines, and stand alone sensors locally in a development mode. These instructions closely follow the instructions (and architecture) for [running the API in production swarm mode](swarm.md). The primary deviations from the production mode:

 - Instead of Virtues running in VMs, they run as Docker containers, either attached to the APINET network of the Sensing API, or connecting to the Swarm externally
 - Route 53 is unavailable in local mode, so the DNS routes required by the API are emulated by container naming and modifications to the Docker host `/etc/hosts` file

Otherwise, the API itself is identical to that running in production mode. Our development mode Sensing API is defined in `docker-compose.yml`.


# Setup

These instructions assume that you already have Docker and `docker-compose` installed locally. Ideally, disable host firewalls to open all ports on the host machine. If this is undesirable (for instance, running on an untrusted network), you can get the list of required open ports from the [deployment guide](deployment.md).

## Environment

The [dockerized](bin/) tools are equiped to work in production and development mode. To trigger development mode, export an `API_MODE` environment variable:

```bash
> export API_MODE=dev
```

## Swarm Init

For local development, we'll be running on a single node Docker Swarm. Initialize the local Docker environment to be a Swarm manager:

```bash
> docker swarm init
```

# Running

From this point on, the network and deployed stack can be started, stopped, and removed as needed during development.


## The APINET network

We directly configure the network that will be used by the swarm using a `docker network` command. The subnet used doesn't matter, so long as it doesn't cause IP and subnet conflicts with the local LAN:

```bash
> docker network create --driver overlay --attachable --subnet 192.168.1.0/24 apinet
```

Our network is named `apinet`, which is important - this is the identifier we use to later attach new networks or containers to the same network running the API. Because we're using an `overlay` network, any ports exposed by containers within the `apinet` network (explicity declared in the `docker-compose.yml` file) can be accessed at the IP and hostname of the Docker host (your local machine).

## Setup a Docker Registry

Moving containers built with `docker-compose` into a docker swarm requires a registry. Rather than using the global Docker Hub registry, we spin up our own registry as part of our deploy step. Start the registry with:

```bash
> sudo docker service create --name registry --publish 5000:5000 registry:2
```

You can confirm that the registry is running with:

```bash
> curl http://localhost:5000/v2/
{}
```

Your `registry` can stay active throughout development, and only needs to be restarted if your Docker Swarm node has restarted, or Docker has otherwise restarted.

# Development Cycle

## Sensing API

Running containers and compose files on Docker Swarm is slightly different than the normal Docker cycle. During development that impacts any of the Sensing API services, you'll likely follow the cycle:

 - build
 - push
 - deploy
 - tear down


### Building the API

You need to build the API using `docker-compose`:

```bash
> docker-compose build
```

This will rebuild any containers explicity defined in the `docker-compose.yml` file. Notably, this will not rebuild target (sensor instrumented) containers.

### Pushing the API Containers

Before you can deploy the API, the built containers must be pushed to the `registry` we started on our swarm:

```bash
> docker-compose push
```

### Deploying the API

Deploy the API with the `docker stack` command:

```bash
> docker stack deploy --compose-file docker-compose.yml savior-api
```

If this is a first deployment, you may need to load configuration data into the API:

```bash
./bin/load_sensor_configurations.py
```

### Tearing down the API

You need to tear down the API before re-deploying after making changes and rebuilding:

```bash
> docker stack rm savior-api
```

This tear down may take a few seconds. **NOTE**: you _do not_ need to tear down the `apinet` network between deploys.

## Target Containers

The primary mode of testing new sensing capabilities with the Sensing API in development mode is to add the sensors to the `demo-target`. This target can be built with a `bin` command:

```bash
> ./bin/dockerized-build.sh
```

New targets can be attached to the network with the `add-target.sh` script:

```bash
> ./bin/add-target.sh target_2
```

Multiple targets can be added at once, so long as each target attached to the network has a unique name:

```bash
> ./bin/add-target.sh target_3 target_4 my_target
```

You can clear out the added targets with another `bin` command:

```bash
> ./bin/clear_network.py
```

## Other Containers

Containers other that the `demo-target` can be added to the network, just be sure to use the `---network=apinet` flag:

```bash
docker run -d -rm --name my-container-name --network=apinet ubuntu:trusty bash
```

## External Capabilities/Sensors

Developing components that need to talk to the API, but don't need to attach to the `apinet` network is also possible. The only additional change required to support this is adding the following entries to your *host machine* `/etc/hosts` file:

```
127.0.0.1			sensing-api.savior.internal
127.0.0.1			sensing-ca.savior.internal
127.0.0.1			sensing-kafka.savior.internal
```

Then your component should be able to use the normal DNS names of the services, with the Swarm hosting the Sensing API receiving the traffic on your local Docker Swarm.

# FAQ

## How can I do a COMPLETE rebuild

This is tricky - it requires basically removing **all** existing Docker images. This is not a careful pruning, this is removing **every** image on your system:

```bash
docker stack rm savior-api
docker service rm registry
docker network rm apinet
docker kill $(docker ps -q)
docker rmi $(docker images -a --filter=dangling=true -q) --force
docker rm $(docker ps --filter=status=exited --filter=status=created -q) --force
docker rmi $(docker images -a -q) --force
```

This **should** remove every image on your system. Rebuilding at this point is a simple build:

```bash
docker-compose build --no-cache
```