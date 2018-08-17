# Sensing Message Protocol

The initial implementation of the Sensing Message Protocol runs over an AF_UNIX socket. This implies that the two parties are connected and in agreement over how to send and receive messages.

The Protocol itself does not imply any particular medium, but it does require a connection agreement over reading and writing messages.

The protocol requires Client and Server roles. The Server "listens" for a connection request from the Client, and responds with a connection medium of some kind.

Our initial implementation uses AF_UNIX and SOCK_STREAM for this connection agreement. The kernel sensor acts as the server, listens on the AF_UNIX socket for connection requests from clients. There may be multiple clients concurrently engaging with the server.


## Request/Response

Messages are related to each other. Each message is either a request message or a response message. Each request is expected to be followed by at least one response. Some requests may cause multiple responses, such as a request to dump log records.

## JSONL
Messages are in the JSONLines format: http://jsonlines.org, JSONL is a variant of JSON http://json.org, that uses new line characters (0x0a and 0x0d) to deliminate records.

### Parsing of JSONL in the Sensing Protocol

While it is possible with JSON to embed multiple messages in a single string buffer, we do not support this message format. Each message is terminated by the server with a newline or carriage return character. _(Hence the JSONL designation)_.

The server will seek the newline character and use it to terminate the message _before_ it submits the message to the JSON parsing library.

#### Escaping Newline Characters
See _Escaping Newlines in Response Record Data_ below.

## Message Header
Each message must begin with a protocol header that includes a version value, followed by an array of zero or more records:
"{Virtue-protocol-version: 0.1, [...]}\n"

## Protocol

### Session Request and Protocol version
After establishing a connection medium (see above) the client makes a request containing only the protocol header and version (represented by 0.1) by writing to the connected socket:

"{Virtue-protocol-version: 0.1}\n"

The server reads this and, if it agrees to serve the client it responds with:

"{Virtue-protocol-version: 0.1}\n"

Now the Client and Server are in agreement over the protocol version, and ready to engage in the protocol.

If the server refuses the session request, it responds with:
"{}\n" and the client closes the connection.

### Discovery Request and Resply
At any time the client may make a discovery request:

"{Virtue-protocol-verion: 0.1, request: [nonce, discovery, sensor] }\n"

Where _nonce_ is a unique value that identifies a specific request and response, and _sensor_ is assumed to be `*` and ignored (but must be present in version 0.1 of the protocol.)

The server is expected to respond with:

'{Virtue-protocol-verion: 0.1, reply: [nonce, discovery,
   [ ["sensor1 name", "sensor1 uuid"],
     ["sensor2 name", "sensor2 uuid"], ...
     ["sensorN name", "sensorN uuid"]
   ] ]}\n'

Where _nonce_ is idendical to the value from the request. The _sensor ids_ array contains an array with the Name and UUID string of each sensor that is registered with the server. For example, ["kernel-ps", "uuid"], ["kernel-lsof", "uuid"], and so on.

The client then knows the name and UUID string of each sensor that is registered with the server. The client can address a message to a specific sensor using either its name or its uuid. (However, only the uuid is guaranteed to be unique to a specific running instance of a sensor.)

#### Nonce Size
The `nonce` size is upon the discretion of the client, provided it is between 16-40 bytes (`MIN_NONCE_SIZE` -  `MAX_NONCE_SIZE`). MAX_NONCE_SIZE is 40 bytes, which is large enough to contain a double-quoted quoted RFC 4421 UUID string.


### Sensor Commands
The client may issue commands to the sensor:

"{Virtue-protocol-verion: 0.1, request: [nonce, command, sensor] }\n"

_sensor_ may be either the sensor's `uuid` or the sensor's `name`. Either usage will work, but only the `uuid` is guaranteed to uniquely target a sensor instance when there are multiple copies of a sensor running at once. 

_command_ is one of:

* off: turn sensor sensing off
* on: turn sensor sensing on
* increase: increase sensor sensing level
* decrease: decrease sensor sensing level
* low: set sensing level to _low_
* default: set sensing level to _default_
* high: set sensing level to _high_
* adversarial: set sensing level to _adversarial_
* reset: reset sensor to _default_ state
* records: retrieve sensor records

The server will respond with:

"{Virtue-protocol-verion: 0.1, reply: [nonce, result] }\n"

where _result_ is one of:

* off: sensor is off
* on: sensor is on
* level: sensor is sensing at this level
* reset: sensor is reset, meaning its state is erased
* records: one or more messages with result records. Multiple records are correlated using the _sensor id_ and _nonce_ from the original request.

Each sensor must define the meaning of the specific states for itself. For example, _adversarial_ entails different sets of behavior for different types of sensors.

Each sensor will reply with its records differently. The most common format is for each record to be one JSONL message:

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, record 1] }\n"

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, record 2] }\n"

...

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, record n] }\n"

and finally:

"{Virtue-protocol-verion: 0.1, reply: nonce, id] }\n"

A reply with only the nonce, and no record, will terminate the original records request.


### Sensor Record Retention

Each sensor may have a different policy for retaining records. The default policy is for a sensor to retain a record until that record has been read by a client or until the sensor runs out of memory for record storage.

This means, for example, that serial _records_ requests will not result in duplicate records responses. For example, the first _records_ request will result in records responses containing records 1...n. The next _records_ request will result in records n+1...n+n.

A sensor may have a different policy for record retention and reporting. For example, a sensor may retain records indefinitely and may respond to a request for specific records.

"{Virtue-protocol-verion: 0.1, request: [nonce, id, record 2048] }\n"

Is a request for _record number 2048_ from probe _id_. The response would be:

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, record 2048] }\n"

if that record existed and was retained by the probe, or:

"{Virtue-protocol-verion: 0.1, reply: [nonce, id] }\n"

if that record did not exist or was not retained by the sensor.

The client must have prior knowledge of a particular sensor's policy for record retention. If a sensor has a particular way of indexing records, the client must have prior knowledge also of that fact.

### Sensor Record Filters

A client may include a _filter_ in a records request:

"{Virtue-protocol-verion: 0.1, request: [nonce, id, filter 'filter'] }\n"

The server would respond with any records that passed the filter:

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, record ] }\n"

the protocol makes no assumption regarding the contents or interpretation of a _filter_. The client and sensor must have a prior knowledge of the filter and its interpretaion.

### Escaping Newlines in Response Record Data

If a sensor creates a sensing record that has newline characters as data, it should escape the newline characters with a '\' character when it forms a reply message containing the record.

For example, a reply record that containes an embedded new line could be similar to:

"{Virtue-protocol-verion: 0.1, reply: [nonce, [id, record 1 \\\n data]] }\n"

The server will escape the newline characters when forming the reply, and the client is responsible for un-escaping the newline characters when it parses the reply message.

### Un-escaped data in Sensor Record Responses

If a sensor includes record data that is problematic (or not useful) to encode as a JSONL string, the record will include a nested object of the form:

{type: raw, length: _length_}

indicating that non-JSONL data follows the record reply object, as follows:

"{Virtue-protocol-verion: 0.1, reply: [nonce, id, {type: raw, length: _length_} ] }\n" _length_ bytes of data.
