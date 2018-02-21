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
