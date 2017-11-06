#!/usr/bin/python
__VERSION__ = "1.20171106"

import curio
from curio import subprocess, Queue, TaskGroup, run, spawn, tcp_server, CancelledError
import email
from io import StringIO
import json

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


async def main():
    """
    Primary task spin-up. we're going to coordinate and launch all of
    our asyncio co-routines from here.

    :return: -
    """
    async with TaskGroup() as g:
        await g.spawn(log_drain)
        await g.spawn(lsof)
        await g.spawn(tcp_server, '', 11000, http_handler)


if __name__ == "__main__":

    # good morning.
    print("Starting lsof-sensor(version=%s)" % (__VERSION__,))

    # let's jump right into async land
    run(main)
