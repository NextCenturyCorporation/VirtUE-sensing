#!/usr/bin/python
__VERSION__ = "1.20171106"


import argparse
import asks
import base64
from Crypto.PublicKey import RSA
from curio import subprocess, Queue, TaskGroup, run, spawn, tcp_server, CancelledError, SignalEvent
import email
import hashlib
from io import StringIO
import json
from kafka import KafkaProducer
import os
import pwd
import requests
from routes import Mapper
import signal
import socket
import sys
from uuid import uuid4

# configure ASKS to use curio
asks.init('curio')

#
# A dummy sensor that streams log messages based off of following the
# `lsof` command.
#
# This sensor tracks the output of a subprocess running `lsof` as well
# as repsonds to HTTP requests for sensor configuration and actuation.
# Because of this, the sensor makes heavy use of Python asyncio to run
# tasks concurrently.
#
#

# log message queueing
log_messages = Queue()


async def register_sensor(opts):
    """
    Register this sensor with the Sensing API.

    :param opts: argparse parsed parameters
    :return:
    """
    uri = construct_api_uri(opts, "/sensor/%s/register" % (opts.sensor_id,))
    pubkey_b64 = str(base64.urlsafe_b64encode(load_public_key(opts.public_key_path)[1].encode()), encoding="iso-8859-1")

    payload = {
        "sensor": opts.sensor_id,
        "virtue": opts.virtue_id,
        "user": opts.username,
        "public_key": pubkey_b64,
        "hostname": opts.sensor_hostname,
        "port": opts.sensor_port
    }

    print("registering with [%s]" % (uri,))

    # we need to setup headers ourselves, because for some stupid, stupid
    # reason the `asks` library lower cases all standard headers, which
    # confuses any strict HTTP server looking for headers. Stupid, stupid
    # stupidness. Guh.
    headers = {
        "Host": "%s:%d" % (opts.sensor_hostname, opts.sensor_port),
        "Connection": "keep-alive",
        "Accept-Encoding": "gzip, deflate",
        "Accept": "*/*",
        # "Content-Length": 0,
        "User-Agent": "python-asks/1.3.6",
        "Content-Type": "application/x-www-form-urlencoded"
    }

    res = await asks.put(uri, data=payload, headers=headers)
    # res = requests.put(uri, data=payload)

    if res.status_code == 200:
        print("Registered sensor with Sensing API")
        print(res.json())
        return res.json()
    else:
        print("Couldn't register sensor with Sensing API")
        print("  status_code == %d" % (res.status_code,))
        print(res.text)
        sys.exit(1)


def deregister_sensor(opts):
    """
    Deregister this sensor from the sensing API.

    :param opts: argparse parsed parameters
    :return:
    """
    uri = construct_api_uri(opts, "/sensor/%s/deregister" % (opts.sensor_id,))
    pubkey_b64 = str(base64.urlsafe_b64encode(load_public_key(opts.public_key_path)[1].encode()), encoding="iso-8859-1")

    payload = {
        "sensor": opts.sensor_id,
        "public_key": pubkey_b64
    }
    res = requests.put(uri, data=payload)
    if res.status_code == 200:
        print("Deregistered sensor with Sensing API")
    else:
        print("Couldn't deregister sensor with Sensing API")
        print("  status_code == %d" % (res.status_code,))
        print(res.text)


async def lsof():
    """
    Continuously read lsof, at the default interval of 15 seconds between
    repeats.

    :return: -
    """
    print(" ::starting lsof")

    # just read from the subprocess and append to the log_message queue
    p = subprocess.Popen(["lsof", "-r"], stdout=subprocess.PIPE)
    async for line in p.stdout:
        await log_messages.put(line.decode("utf-8"))


async def log_drain(kafka_bootstrap_hosts=None, kafka_channel=None):
    """
    Pull messages off of the reporting queue, and splat them out on STDOUT
    for now.

    :param kafka_bootstrap_hosts: Bootstrap host list for kafka
    :param kafka_channel:
    :return:
    """
    print(" ::starting log_drain")
    print("  ? configuring KafkaProducer(bootstrap=%s, topic=%s)" % (",".join(kafka_bootstrap_hosts), kafka_channel))

    producer = KafkaProducer(
        bootstrap_servers=kafka_bootstrap_hosts,
        retries=5,
        max_block_ms=10000,
        value_serializer=str.encode
    )

    # basic channel tracking
    msg_count = 0
    msg_bytes = 0

    # just read messages as they become available in the message queue
    async for message in log_messages:

        msg_count += 1
        msg_bytes += len(message)

        producer.send(kafka_channel, message)

        # progress reporting
        if msg_count % 5000 == 0:
            print("  :: %d messages received, %d bytes total" % (msg_count, msg_bytes))


async def send_json(stream, json_data, status_code=200):
    """
    Serialize a data structure to JSON and send it as a response
    to an HTTP request. This includes sending HTTP headers and
    closing the connection (via `Connection: close` header).

    :param stream: AsyncIO writable stream
    :param json_data: Serializable data structure
    :return:
    """

    # serialize
    raw = json.dumps(json_data).encode()

    # get the raw length
    content_size = len(raw)

    # figure out the status code string given our numeric code
    #
    #       https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
    #
    # This is an incomplete list, because we don't use all of the codes.
    # If we need additional codes, we can add them later.
    sc_reason = {
        200: "ok",
        500: "internal server error",
        400: "bad request",
        401: "unauthorized",
        404: "not found"
    }

    # write the status code
    res_header = "HTTP/1.1 %d %s\r\n" % (status_code, sc_reason[status_code])

    await stream.write(res_header.encode())

    # write the headers
    headers = {
        "Content-Type": "application/json",
        "Content-Length": str(content_size),
        "Connection": "close"
    }
    for h_k, h_v in headers.items():
        await stream.write( ("%s: %s\r\n" % (h_k, h_v)).encode())

    await stream.write("\r\n".encode())

    # write the json
    await stream.write(raw)


def configure_http_handler(opts):
    """
    Expose the run time options to the http handler which
    will be spawned by the tcp server.

    :param opts: argparse options
    :return: async http_handler function
    """

    # Configure our HTTP routes
    mapper = Mapper()
    mapper.connect(None, "/sensor/{uuid}/registered", controller="route_registration_ping")
    mapper.connect(None, "/sensor/status", controller="route_sensor_status")

    # and which methods handle what?
    mapper_funcs = {
        "route_registration_ping": route_registration_ping,
        "route_sensor_status": route_sensor_status
    }

    async def http_handler(client, addr):
        """
        Handle an HTTP request from a remote client.

        :param client: tcp_server client object
        :param addr: Client address
        :return: -
        """
        print(" -> connection from ", addr)
        s = client.as_stream()
        try:

            # read everything in. we're parsing HTTP, so
            # we'll look for the trailing \r\n\r\n after
            # which we'll jump into header parsing and then
            # response
            buff = b''

            async for line in s:
                buff += line
                if buff[-4:] == b'\r\n\r\n':
                    break

            # now we need to parse the request
            path, headers = buff.decode('iso-8859-1').split('\r\n', 1)
            message = email.message_from_file(StringIO(headers))
            headers = dict(message.items())

            # break out the request string into it's pieces, as it
            # is currently something like:
            #
            #       GET /sensor/status HTTP/1.1
            #
            (r_method, r_path, r_version) = path.strip().split(" ")

            # handle the request, figuring out where it goes with the mapper
            print("[info] got [%s] request path [%s]" % (r_method, r_path,))
            handler = mapper.match(r_path)

            if handler is not None:
                print("[%(controller)s] > handling request" % handler)
                await mapper_funcs[handler["controller"]](opts, s, headers, handler)
            else:
                # whoops
                print("   :: No route available for request [%s]" % (path,))

                # send an error
                await send_json(s, {"error": True, "msg": "no such route"}, status_code=404)

        except CancelledError:

            # ruh-roh, connection broken
            print("connection goes boom")

        # request cycle done
        print(" <- connection closed")

    return http_handler


async def route_registration_ping(opts, stream, headers, params):
    """
    Handle a request for:

        /sensor/{uuid}/registered

    :param opts: argparse parsed options
    :param stream: Async client stream
    :param headers: HTTP headers
    :param params: URI path parameters and controller id
    :return: None
    """

    # do our UUIDs match?
    if opts.sensor_id == params["uuid"]:
        print("  | sensor ID is a match")
        await send_json(stream, {"error": False, "msg": "ack"})
    else:
        print("  | sensor ID is NOT A MATCH")
        await send_json(stream, {"error": True, "msg": "Sensor ID does not match registration ID"}, status_code=401)


async def route_sensor_status(opts, stream, headers, params):
    """
    Handle a request for:

        /sensor/status

    :param opts:
    :param stream:
    :param headers:
    :param params:
    :return:
    """
    print("  | reporting status ")
    await send_json(stream,
                    {
                        "error": False,
                        "sensor": opts.sensor_id,
                        "virtue": opts.virtue_id,
                        "user": opts.username
                    }
                    )


async def main(opts):
    """
    Primary task spin-up. we're going to coordinate and launch all of
    our asyncio co-routines from here.

    :return: -
    """
    Goodbye = SignalEvent(signal.SIGINT, signal.SIGTERM)

    async with TaskGroup() as g:

        # we need to spin up our actuation listener first
        await g.spawn(tcp_server, opts.sensor_hostname, opts.sensor_port, configure_http_handler(opts))

        # now we register and wait for the results
        reg = await g.spawn(register_sensor, opts)

        print("  @ waiting for registration cycle")
        reg_data = await reg.join()
        print(reg_data)
        print("  = got registration")

        await g.spawn(log_drain, reg_data["kafka_bootstrap_hosts"], reg_data["sensor_topic"])
        await g.spawn(lsof)

        await Goodbye.wait()
        print("Got SIG: deregistering sensor and shutting down")
        await g.cancel_remaining()

        # don't run this async - we're happy to block on deregistration
        deregister_sensor(opts)

        print("Stopping.")


def load_public_key(key_path):
    """
    Load and validate a public key for use with the API.

    An exception is raised if the public key is invalid

    :param key_path: Path to the public key.
    :return: RSAKEY object, PEM String
    """

    # validate the key
    try:

        if key_path is None:
            raise ValueError("Public key path is not specified.")

        key_string = open(key_path, 'r').read()

        rsakey = RSA.importKey(key_string)

        # make sure it's a pubkey
        if rsakey.has_private():
            raise ValueError("specified key is an RSA private key")

    except ValueError as ve:

        print("Couldn't import RSA public key at %s - %s" % (key_path, str(ve)))
        print("  If this is encountered during testing, run the gen_cert.sh script to create a key pair, and try testing again")
        sys.exit(1)

    return rsakey, key_string


def load_private_key(key_path):
    """
    Load and validate a private key for use with the API and
    stream encryption.

    An exception is raised if the key is not a private key.

    :param key_path: Path to the public key
    :return: RSAKEY object, PEM STring
    """

    # validate the key
    try:

        if key_path is None:
            raise ValueError("Private key path is not specified")

        key_string = open(key_path, 'r').read()

        rsakey = RSA.importKey(key_string)

        # make sure it's a pubkey
        if not rsakey.has_private():
            raise ValueError("specified key is not an RSA private key")

    except ValueError as ve:

        print("Couldn't import RSA private key at %s - %s" % (key_path, str(ve)))
        print("  If this is encountered during testing, run the gen_cert.sh script to create a key pair, and try testing again")
        sys.exit(1)

    return rsakey, key_string


def insert_char_every_n_chars(string, char='\n', every=64):
    return char.join(
        string[i:i + every] for i in range(0, len(string), every))


def rsa_public_fingerprint(key):
    """
    Generate a hex digest in the form ##:##:##...## of the given
    public key. The fingerprint will be a 47 character string, with
    32 characters of data.

    The public key fingerprint is an md5 digest.

    :param key: RSA Public Key object
    :return:
    """
    md5digest = hashlib.md5(key.exportKey("DER")).hexdigest()
    fingerprint = insert_char_every_n_chars(md5digest, ':', 2)
    return fingerprint


def rsa_private_fingerprint(key):
    """
    Generate a hex digest of the form ##:##:##..## of the given
    private key. The finger print will be a 59 character string,
    with 40 characters of data.

    The private key fingerprint is a sha1 digest.

    :param key:
    :return:
    """
    sha1digest = hashlib.sha1(key.exportKey("DER", pkcs=8)).hexdigest()
    fingerprint = insert_char_every_n_chars(sha1digest, ':', 2)
    return fingerprint


def construct_api_uri(opts, uri_path):
    """
    Build a full URI for a request to the API.

    :param opts: Command configuration options
    :param uri_path: Full path for the API
    :return: String URI
    """

    # setup the host
    host = opts.api_host

    if not host.startswith("http"):
        host = "http://%s" % (host,)

    # setup the full uri
    return "%s:%d/api/%s%s" % (host, opts.api_port, opts.api_version, uri_path)


def options():
    """
    Parse out the command line options.

    :return:
    """
    parser = argparse.ArgumentParser(description="LSOF sensor")

    # top level control

    # key management
    parser.add_argument("--public-key-path", dest="public_key_path", default=None, help="Path to the public key to use")
    parser.add_argument("--private-key-path", dest="private_key_path", default=None, help="Path to the private key to use")
    # communications
    parser.add_argument("-a", "--api-host", dest="api_host", default="localhost", help="API host URI")
    parser.add_argument("-p", "--api-port", dest="api_port", default=4000, type=int, help="API host port")
    parser.add_argument("--api-version", dest="api_version", default="v1", help="API version being called")

    # identification
    parser.add_argument("--sensor-id", dest="sensor_id", default=None, help="ID of the sensor, auto-generated if absent")
    parser.add_argument("--virtue-id", dest="virtue_id", default=None, help="ID of the current Virtue, auto-generated if absent")
    parser.add_argument("--username", dest="username", default=None, help="Name of the observed user, inferred if absent")
    parser.add_argument("--sensor-hostname", dest="sensor_hostname", default=None, help="Addressable name of the sensor host")
    parser.add_argument("--sensor-port", dest="sensor_port", default=11000, help="Port on sensor host where sensor is listening for API actuations")

    return parser.parse_args()


def check_identification(opts):
    """
    Check the identification fields in the options and build values if none
    exist. The fields checked are:

        sensor_id - auto-generated as UUID if missing
        virtue_id - pulled from ENV if available, auto-generated as UUID if missing
        username - pulled from OS if available, auto-generated as user@hostname if missing
        public-key-path - check for valid RSA pubkey
        private-key-path - check for valid RSA private key
        sensor-hostname - fill in a valid hostname

    :param opts: run time options (argparse parsed options)
    :return: None
    """

    if opts.sensor_id is None:
        opts.sensor_id = str(uuid4())

    if opts.virtue_id is None:
        if "VIRTUE_ID" is os.environ:
            opts.virtue_id = os.environ["VIRTUE_ID"]
        else:
            opts.virtue_id = str(uuid4())

    if opts.username is None:

        # try and resolve the user from the PWD
        pstruct = pwd.getpwuid(os.getuid())
        if pstruct is not None:
            opts.username = pstruct[0]

        # no? maybe it's in the environment
        if opts.username is None:
            if "USERNAME" is os.environ:
                opts.username = os.environ["USERNAME"]

        # really? Maybe the process controller?
        if opts.username is None:
            opts.username = os.getlogin()

        # ok. now we're getting desperate. Let's make something up
        if opts.username is None:
            opts.username = "user@%s" % (socket.getfqdn(),)

        # we're out of options at this point
        if opts.username is None:
            raise ValueError("Can't identify the current username, bailing out")

    if opts.sensor_hostname is None:

        opts.sensor_hostname = socket.getfqdn()

        # bork bork bork
        if opts.sensor_hostname is None:
            raise ValueError("Can't identify the current hostname, bailing out")

    pub_key, pub_key_string = load_public_key(opts.public_key_path)
    pri_key, pri_key_string = load_private_key(opts.private_key_path)

    # report
    print("\tsensor_id == %s" % (opts.sensor_id,))
    print("\tvirtue_id == %s" % (opts.virtue_id,))
    print("\tusername  == %s" % (opts.username,))
    print("\thostname  == %s" % (opts.sensor_hostname,))
    print("\tpub key   == %s" % (rsa_public_fingerprint(pub_key),))
    print("\tpriv key  == %s" % (rsa_private_fingerprint(pri_key),))

    print("Sensing API")
    print("\thostname  == %s" % (opts.api_host,))
    print("\tport      == %d" % (opts.api_port,))
    print("\tversion   == %s" % (opts.api_version,))

    print("Sensor Interface")
    print("\thostname  == %s" % (opts.sensor_hostname,))
    print("\tport      == %d" % (opts.sensor_port,))


if __name__ == "__main__":

    # good morning.
    print("Starting lsof-sensor(version=%s)" % (__VERSION__,))

    opts = options()

    check_identification(opts)

    # let's jump right into async land
    run(main, opts)
