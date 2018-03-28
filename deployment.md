A guide for deployment requirements for the Sensing API and sensors in the SAVIOR environment.

# Open Ports

Various ports need to be open and accessible between different hosts within the infrastructure for everything to work. This is an ongoing enumeration of those ports.

## Between API and VMs

The following ports need to be open between the Sensing API hosts and any Virtue VMs or AWS instances:

 - **9555/tcp**: Kafka TLS
 - **17141/tcp**: Sensing API HTTP
 - **17504/tcp**: Sensing API HTTPS/TLS
 - **11000 - 11100/tcp** (Sensor HTTPS actuation)

## Between API Swarm Hosts

The following ports need to be open and accessible between hosts in the Sensing API Swarm network:

 - **2181/tcp, 2181/udp**: Used by Zookeeper and Kafka for service management
 - **2377/tcp, 2377/udp**: Used by Docker Swarm for cluster management
 - **4789/tcp, 4789/udp**: Used by Docker Swarm overlay network
 - **5000/tcp**: Temporary Docker Registry port for container distribution to Docker Swarm nodes
 - **7946/tcp, 7946/udp**: Docker Swarm control traffic
 - **9455/tcp**: Swarm internal port for Kafka communication
 - **9555/tcp**: Swarm external port for Kafka communication
 - **17141/tcp**: Sensing API Insecure port
 - **17504/tcp**: Sensing API Secure port
