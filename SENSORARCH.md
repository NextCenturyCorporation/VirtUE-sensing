
# Sensor Registration Flow

The current registration flow for a sensor includes contacting the Sensing API
to register and verify the sensor, as well as retrieve configuration information that is then used when connecting the log output of the sensor to a Kafka instance.

This registration flow will also include the issuance of TLS client certificates
as a precursor to sensor registration.

```
Sensing API                                                      Sensor
===========                                                   =============

                                                            Spawn HTTP Endpoint
															        |																	
Receive Registration     <------------------------------    regsiter with API
       |
Verify Sensor over HTTP   <----------------------------->    HTTP Endpoint Responder
       |
Record Sensor + Issue Config  -------------------------->   Connect to Kafka
                                                                     |
                                                            Instrument Sensor 
															          /---------------\
																	  |               |
																	  \/              |
															Data streaming            |
                                                                                      |
Sync + Re-record Sensor    <----------------------------    Send heartbeat to API     |
                                                                      |               |
																	  |               |
																	  \---------------/

Remove Sensor Record      <-----------------------------    Graceful shutdown

Remove Sensor Record       (sustained missing heartbeat)

```

# Implementing the Registration Cycle

This section will walk through the process of registering a sensor with the Sensing API, and retrieving configuration information. This includes the URIs, and example payloads and responses. Before going through the Registration Cycle, a sensor must have successfully gone through the [Certificate Cycle](https://github.com/twosixlabs/savior/blob/master/CERTIFICATES.md), as all registration API calls, and communication with the logging and API infrastructure post registration, is done via mutually authenticated TLS.

Registration follows these steps:

 1. Setup an HTTPS responder
 2. Register with the Sensing API
 3. Respond to the Sensor Registration Verification
 4. Stream data to Logging (_technically not part of the cycle_)
 5. Heartbeat Sync with Sensing API
 6. Gracefully de-register

## 1. Setup an HTTPS Responder

During the HTTPS call to the Sensing API in step 2, the API will asynchronously send an HTTPS GET request back to use to verify communications. Our HTTPS responder will need to recieve that request and respond appropriately. **Note**: the contents of this response will change in the future.

The Sensing API will do an HTTPS GET request:

```
GET https://<sensor hostname>:<sensor port>/sensor/<sensor uuid>/registered
```

and expects a JSON response that looks like:

```json
{
	"error": false,
	"msg": "ack"
}
```

The `sensor hostname`, `sensor port`, and `sensor uuid` are all values defined by our sensor, and are provided to the Sensing API as part of the registration request in Step 2. Considerations:

 - `sensor hostname` needs to be a routable hostname which our sensor controls and can open a port on
 - `sensor port` needs to be a port we can bind with an HTTPS server, and will remain active throughout the life time of our sensor
 - `sensor uuid` needs to be globally unique, and should be generated using **UUID Mode 4**.

With our HTTPS responder running, we can move on to Step 2 and send our registration request.

# 2. Register with the Sensing API

Registering with the Sensing API requires an HTTPS PUT request with a JSON payload in the request body. When making this request, make sure to specify the Savior CA Public Key as the trusted root for our certificate chain.

```
PUT https://api:17141/api/v1/sensor/<sensor uuid>/register
```

The JSON payload contains the identifying information for the sensor, including the sensor Public Key generated during the Certificate Cycle:

```json
{
	"sensor": <sensor uuid>,
	"virtue": <virtue uuid>,
	"user": <virtue user name>,
	"public_key": <sensor public key>,
	"hostname": <sensor host name>,
	"port": <sensor https responder port>,
	"name": <sensor name>
}
```

The `sensor` and `virtue` UUIDs should be globally unique. At a future point the Virute UUID will be auto-generated at the virtue or kernel level and inherited by all sensors within that Virtue or kernel.

The `user` is the system or LDAP or Active Directory username of the user interacting with the Virtue, or `root` for unikernels.

The `public_key` should be the PEM encoded public key from our public/private key pair retrieved during the Certificate Cycle.

The `hostname` and `port` are the values used in our HTTPS Responder that we setup in step 1.

The sensor `name` should be the canonical name of the sensor suffixed with a version number or date stamp, like `lsof-2018.01.12`. During registration the version suffix is removed, and the remaining value (in this case `lsof`) is used to find configuration data for the sensor. If no configuration data can be found, registration will fail.

The JSON payload returned from registration will look like:

```json
{
	"error": false,
	"timestamp": "2018-01-30 14:54:15.679406Z",
	"sensor": <sensor uuid>,
	"registered": true,
	"kafka_bootstrap_hosts": ["kafka:9455"],
	"sensor_topic": <kafka topic name for sensor logs>,
	"default_configuration": <string of config data>
}
```

The `sensor` UUID should be an exact match for our UUID. The list of `kafka_bootstrap_hosts` may change over time, but will be suitable to be directly passed to Kafka libraries as bootstrap servers to the message brokers.

The `sensor_topic` is the topic channel to which our sensor should publish log messages (in JSON format). These sensor topics are randomized per-sensor, to make brute-force enumeration or selection of topics more difficult.

The initial configuration for our sensor will be packed as a string in the `default_configuration` field. The format of the configuration is sensor dependent, but could be, for instance, JSON, XML, or other configuration formats. It is up to the sensor to decode the string into appropriate structures and/or locations.

# 3. Respond to the Sensor Registraion Verification

After our request to the Sensing API, but before we receive a response, the API will asynchronously issue a GET request to the host and port we specify in the registration information in step 2. The HTTPS responder we configured in step 1 should repsond appropriately, which will enable the rest of the registration process to complete.

# 4. Stream Data to Logging

While technically not part of the Registration Cycle, at this point our sensor should establish communication with the Kafka bootstrap servers and begin broadcasting to the topic channel.

Communication with the Kafka brokers is authenticated via mutual TLS, so the sensor public/private key pair will be required, as well as the CA public key. Exact settings and parameter names will be library specific, but in Python the following configuration options are required:

```python
producer = KafkaProducer(
    bootstrap_servers=kafka_bootstrap_hosts,
    retries=5,
    max_block_ms=10000,
    value_serializer=str.encode,
    ssl_cafile="filesystem path to CA public key",
    security_protocol="SSL",
    ssl_certfile="file system path to sensor public key",
    ssl_keyfile="file system path to sensor private key"
)
```

# 5. Heartbeat sync with Sensing API

Sensors are required to periodically issue a heartbeat HTTPS request to the Sensing API, otherwise sensors will be automatically de-registered. These heartbeats should be issued every 1-2 minutes, and involve a PUT request to the API:

```
PUT https://api:17141/api/v1/sensor/<sensor uuid>/sync
```

A JSON payload should be included in the request body:

```json
{
	"sensor": <sensor uuid>,
	"public_key": <sensor public key>
}
```

The Sensing API will respond to a sync message with the following JSON:

```json
{
	"error": false,
	"timestamp": "2018-01-30 14:54:15.679406Z",
	"sensor": <sensor uuid>,
	"synchronized": true
}
```

The sync heartbeat messages should continue for the life span of the sensor. 

# 6. Gracefully de-register

Sensors should make every attempt to gracefully de-register from the Sensing API when the Virtue, and sensors, are spinning down. Sensors will be automatically de-registered from the Sensing API after a number of missing synchronization heartbeats in the cases where de-registration is not possible or incomplete.

De-registration is an HTTPS PUT request with a JSON payload:

```
PUT https://api:17141/api/v1/sensor/<sensor uuid>/deregister
```

The JSON payload is the same as used for sensor synchronization:

```json
{
	"sensor": <sensor uuid>,
	"public_key": <sensor public key>
}
```

Sensors can check for an `HTTP 200` status code, or inspect the JSON response from the API:

```json
{
	"error": false,
	"timestamp": "2018-01-30 14:54:15.679406Z",
	"sensor": <sensor uuid>,
	"deregistered": true
}
```