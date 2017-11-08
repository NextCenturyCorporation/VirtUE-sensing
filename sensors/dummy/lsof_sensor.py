#!/usr/bin/python
__VERSION__ = "1.20171106"


import argparse
from Crypto.PublicKey import RSA
from curio import subprocess, Queue, TaskGroup, run, spawn, tcp_server, CancelledError
import email
import hashlib
from io import StringIO
import json
import os
import pwd
import requests
import socket
import sys
from uuid import uuid4


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

    :return:
    """
    uri = construct_api_uri(opts, "/sensor/%s/register" % (opts.sensor_id,))
    payload = {
        "sensor": opts.sensor_id,
        "virtue": opts.virtue_id,
        "user": opts.username,
        "public_key": load_public_key(opts.public_key_path)[1],
        "hostname": opts.sensor_hostname,
        "port": opts.sensor_port
    }
    res = requests.put(uri, data=payload)
    if res.status_code == 200:
        print("Registered sensor with Sensing API")
    else:
        print("Couldn't register sensor with Sensing API")
        print("  status_code == %d" % (res.status_code,))
        print(res.text)
        sys.exit(1)


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


async def log_drain():
    """
    Pull messages off of the reporting queue, and splat them out on STDOUT
    for now.

    :return:
    """
    print(" ::starting log_drain")

    # basic channel tracking
    msg_count = 0
    msg_bytes = 0

    # just read messages as they become available in the message queue
    async for message in log_messages:

        msg_count += 1
        msg_bytes += len(message)

        # progress reporting
        if msg_count % 5000 == 0:
            print("  :: %d messages received, %d bytes total" % (msg_count, msg_bytes))


async def send_json(stream, json_data):
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

    # write the status code
    await stream.write("HTTP/1.1 200 ok\n".encode())

    # write the headers
    headers = {
        "Content-Type": "application/json",
        "Content-Length": str(content_size),
        "Connection": "close"
    }
    for h_k, h_v in headers.items():
        await stream.write( ("%s: %s\n" % (h_k, h_v)).encode())

    await stream.write("\n".encode())

    # write the json
    await stream.write(raw)


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

        # report on the request
        print("    :: HTTP request for [%s]" % (path,))
        for k,v in headers.items():
            print("  \t%s => %s" % (k, v))

        # send some dummy JSON as a response for now
        await send_json(s, {"error": False, "msg": "hola"})

    except CancelledError:

        # ruh-roh, connection broken
        print("connection goes boom")

    # request cycle done
    print(" <- connection closed")


async def main(opts):
    """
    Primary task spin-up. we're going to coordinate and launch all of
    our asyncio co-routines from here.

    :return: -
    """
    async with TaskGroup() as g:
        await g.spawn(log_drain)
        await g.spawn(lsof)
        await g.spawn(tcp_server, '', 11000, http_handler)
        await g.spawn(register_sensor, opts)


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
    parser.add_argument("--sensor-port", dest="sensor_port", default=4000, help="Port on sensor host where sensor is listening for API actuations")

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
        opts.sensor_id = uuid4()

    if opts.virtue_id is None:
        if "VIRTUE_ID" is os.environ:
            opts.virtue_id = os.environ["VIRTUE_ID"]
        else:
            opts.virtue_id = uuid4()

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
    print("\thostname == %s" % (opts.api_host,))
    print("\tport     == %d" % (opts.api_port,))
    print("\tversion  == %s" % (opts.api_version,))


if __name__ == "__main__":

    # good morning.
    print("Starting lsof-sensor(version=%s)" % (__VERSION__,))

    opts = options()

    check_identification(opts)

    # let's jump right into async land
    run(main, opts)
