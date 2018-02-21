# Sensing Message Protocol

The initial implementation of the Sensing Message Protocol runs over an AF_UNIX socket. This implies that the two parties are connected and in agreement over how to send and receive messages.

The Protocol itself does not imply any particular medium, but it does require a connection agreement over reading and writing messages.

The protocol requires Client and Server roles. The Server "listens" for a connection request from the Client, and responds with a connection medium of some kind.

Our initial implementation uses AF_UNIX and SOCK_STREAM for this connection agreement. The kernel sensor acts as the server, listens on the AF_UNIX socket for connection requests from clients. There may be multiple clients concurrently engaging with the server.


## Request/Response

Messages are related to each other. Each message is either a request message or a response message. Each request is expected to be followed by at least one response. Some requests may cause multiple responses, such as a request to dump log records.

## JSONL
Messages are in the JSONLines format: http://jsonlines.org, JSONL is a variant of JSON http://json.org, that uses line characters to deliminate records.

## Message Header
Each message must begin with a protocol header that includes a version value, followed by an array of zero or more records:
"{Virtue protocol version: 0.1, [...]}\n"

## Protocol

### Session Request and Protocol version
After establishing a connection medium (see above) the client makes a request containing only the protocol header and version (represented by 0.1) by writing to the connected socket:

"{Virtue protocol version: 0.1}\n"

The server reads this and, if it agrees to serve the client it responds with:

"{Virtue protocol version: 0.1}\n"

Now the Client and Server are in agreement over the protocol version, and ready to engage in the protocol.

If the server refuses the session request, it responds with:
"{}\n" and the client closes the connection.

### Discovery Request and Response
At any time the client may make a discovery request:

"{Virtue protocol version: 0.1, request: [nonce, discovery] }\n"

Where _nonce_ is a unique value that identifies a specific request and response.

The server is expected to respond with:

"{Virtue protocol version: 0.1, reply: [nonce, [probe ids]] }\n"

Where _nonce_ is idendical to the value from the request. The _probe ids_ array contains the Id String of each probe that is registered with the server. For example, "kernel-ps", "kernel-lsof", and so on.

The client then knows the identifying string of each probe that is registered with the server.

### Probe Commands
The client may issue commands to the probe:

"{Virtue protocol version: 0.1, request: [nonce, command [probe ids]] }\n"
where _command_ is one of:

* off: turn probe sensing off
* on: turn probe sensing on
* increase: increase probe sensing level
* decrease: decrease probe sensing level
* reset: reset probe state
* records: retrieve probe records


The server will respond with:

"{Virtue protocol version: 0.1, reply: [nonce, command [probe ids], [id, result]] }\n"

where _result_ is one of:

* off: probe is off
* on: probe is on
* level: probe is sensing at this level
* reset: probe is reset, meaning its state is erased
* records: one or more messages with result records. Multiple records are correlated using the _probe id_ and _nonce_ from the original request.

Each probe will reply with its records differently. The most common format is for each record to be one JSONL message:

"{Virtue protocol version: 0.1, reply: [nonce, [id, record 1]] }\n"

"{Virtue protocol version: 0.1, reply: [nonce, [id, record 2]] }\n"

...

"{Virtue protocol version: 0.1, reply: [nonce, [id, record n]] }\n"

and finally:

"{Virtue protocol version: 0.1, reply: [nonce, [id]] }\n"

A reply with only the nonce, and no record, will terminate the original records request.


### Probe Record Retention

Each probe may have a different policy for retaining records. The default policy is for a probe to retain a record until that record has been read by a client or until the probe runs out of memory for record storage.

This means, for example, that serial _records_ requests will not result in duplicate records responses. For example, the first _records_ request will result in records responses containing records 1...n. The next _records_ request will result in records n+1...n+n.

A probe may have a different policy for record retention and reporting. For example, a probe may retain records indefinitely and may respond to a request for specific records.

"{Virtue protocol version: 0.1, request: [nonce, [id, record 2048]] }\n"

Is a request for _record number 2048_ from probe _id_. The response would be:

"{Virtue protocol version: 0.1, reply: [nonce, [id, record 2048]] }\n"

if that record existed and was retained by the probe, or:

"{Virtue protocol version: 0.1, reply: [nonce, [id]] }\n"

if that record did not exist or was not retained by the probe.

The client must have prior knowledge of a particular probe's policy for record retention. If a probe has a particular way of indexing records, the client must have prior knowledge also of that fact.

### Probe Record Filters

A client may include a _filter_ in a records request:

"{Virtue protocol version: 0.1, request: [nonce, [id, filter 'filter']] }\n"

The server would respond with any records that passed the filter:

"{Virtue protocol version: 0.1, reply: [nonce, [id, record ]] }\n"

the protocol makes no assumption regarding the contents or interpretation of a _filter_. The client and probe must have a prior knowledge of the filter and its interpretaion.
