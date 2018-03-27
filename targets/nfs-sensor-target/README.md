# NFS Sensor - Overview

The NFS sensor watches a network interface for NFS version 3 traffic
and reports certain metadata. While the component actually functions
as a sensor -- that is, it observes system state and reports on it to
the Sensor API -- its code lives in the target directory because it
more closely matches a target.  It runs in its own environment (not in
a target VM/container) and does not host other sensors.

The sensor is intended to run alongside the
[NFS Server Unikernel](https://github.com/NextCenturyCorporation/VirtUE-VMs).
It requires
access to the NIC that the NFS traffic traverses and access to the
Sensor API server. To simplify dependencies for the user, it runs
within a Docker container. This can cause challenges when attempting
to test the code in production mode, since the container needs access
to the host networking and also the Docker swarm's `savior_default`
network.

The sensor uses [Scapy](https://github.com/secdev/scapy) and pulls it
in as a git submodule.

# Build

To build the sensor's container, navigate to the `nfs-sensor-target` directory in a shell and run:

```bash
> sudo docker build -t virtue-savior/nfs-sensor-target:latest .
```

# Run

## Production

There are a couple requirements to run the sensor in production mode:

* There must be a DNS entry for the sensor API server, which must have been
  referenced in `run.sh` when the container was built.

* The DOMID or the VIF of the NFS Server Unikernel must be known prior
  to starting the sensor. This is passed to the container through the
  environment.

If these requirements are fulfilled, the sensor can be run by:

```bash
> sudo docker run -e NFS_SENSOR_SNIFF_INTERFACE=vif3.0 --network=host virtue-savior/nfs-sensor-target:latest
```

Where `NFS_SENSOR_SNIFF_INTERFACE` should indicate the actual NIC used
by the NFS Server Unikernel.


## Development

Look at `run.sh`, taking note of the variable
`NFS_SENSOR_STANDALONE`. If the variable is set (non-empty) then the
standalone sensor will be launched. It does not interact with the
Sensor API; it only parses NFS messages and prints them out in
summary, JSON form. It uses the same NFS parser and message generator
as the production code.


# Design details

Whether using the production or standalone script, the sensor's core
functions the same way. It relies on
[Scapy](https://github.com/secdev/scapy) to sniff packets off the wire
(or grab from a file) and to direct parsing.

1. Read in a packet, either from a `.pcap` file or over a network
interface.

2. Parse the packet. Scapy initiates this process, but we direct Scapy
in certain ways within `sensor/nfs.py`:

   * Bind TCP and UPD ports 111, 955, and 2049 to Scapy-derived RPC
     Header handlers. RPC is described in 
     [RFC 1831](https://tools.ietf.org/html/rfc1831).

   * Bind other TCP and UDP ports to the RPC handlers upon completion
     of a GETPORT exchange.

3. The RPC header directs Scapy to parse the next protocol (PORTMAP,
MOUNTD, or NFSv3 - RFC 1813) based on data in the RPC header. It's
important to note here that the RPC Call header contains a transaction
ID, the 'program' number, and the 'procedure' number (in RPC
nomenclature). However, the RPC Reply only has the transation
ID. Therefore our parser maps transaction IDs to their RPC Call
objects so it knows how to parse RPC Replies.

4. Once the RPC packet has been parsed, it is "handled", meaning it is
summarized into a Python dictionary. This is done is
`sensor/nfs_packet_handler.py`. If the standalone sensor is running,
it will just print out that dictionary. If the production sensor is
running, it will put the dictionary into a JSON message and forward it
to the API server.
