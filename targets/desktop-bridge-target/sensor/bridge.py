#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

"""
Async Desktop Bridge sensor - forwards JSON messages to the API
sensing infrastructure.

Creates two async tasks: (1) Watch inbound JSON message queue and
forward such messages to the outbound (API) queue, and (2) spawn a
server that handles inbound TCP/IP connections and reads inbound JSON
messages from them.

The behavior of this script is affected by the following environment
variables:
* LISTEN_PORT=a.b.c.d:xxxx the interface and port the sensor should
  listen on for sensing data (REQUIRED).
* VERBOSE if evaluates as true, enables verbose logging (OPTIONAL).
* STANDALONE evaluates as true, runs in standalone mode; otherwise runs in
  production mode (OPTIONAL).
"""

import os
import sys
import argparse
import logging
import pdb
import json
import socket
import platform
import datetime

import asyncio
import selectors
import curio

inqueue = curio.Queue()
opts = None
server_iface = None
server_port = None


msg_ct = 0
async def handle_messages(message_stub=None, config=None, outqueue=None):
    """
    Read JSON messages from the input queue and forward to the output
    queue. Signature is compatible with SensorWrapper.

    Runs forever, even across multiple inbound connections.
    """

    global msg_ct

    logging.info("Starting message handler()")
    async for injson in inqueue:
        # read message from queue, put in outqueue

        logging.debug("Read message %s from inqueue", injson)
        if not injson:
            # That was the end of this stream, but we can accept more
            # alternate design: start handle_messages() once per connection
            injson = { "status" : "no more messages" }
            # TODO: flush outqueue

        # Entire inbound JSON message is stuffed into "message" value of outbound message
        outdict = { "timestamp" : datetime.datetime.now().isoformat(),
                    "level"     : "info",
                    "message"   : injson }
        if message_stub:
            outdict.update(message_stub)

        msg_ct += 1
        if os.getenv('STANDALONE'): #opts.standalone:
            logging.debug("JSON message # %d", msg_ct)
        else:
            out_str = json.dumps(outdict)
            logging.debug("Enqueuing JSON message %s", out_str)
            await outqueue.put( out_str + "\n" )
            logging.debug("JSON message put in queue")


async def read_json_doc(stream):
    """
    Read from the stream and return a decoded JSON document as soon as
    we can detect it. If we read too much non-JSON data we'll throw it
    out. Note that each inbound JSON message must end with a '\n'
    character.

    :param stream: network socket
    :return: decoded JSON data
    """

    body = ""
    decoded = None
    while True:
        line = ""
        #line = await stream.read(1) # slow alternate
        line = await stream.readline()
        if not line:
            decoded = None # give up
            break
        body += line.decode("utf-8").strip()
        try:
            decoded = json.loads(body)
            break # success
        except json.decoder.JSONDecodeError as _jde:
            # Failure: incomplete message so far
            logging.debug("Partial/invalid: %s", body)
            if len(body) > 8192:
                logging.warning("Dropping long, invalid message: %s", _jde)
                body = ""
    return decoded


async def handle_connection(reader, writer):
    """
    Read JSON messages from the reader and place them in the internal queue.
    """

    logging.info("Handling inbound connection from %r",
                 writer.get_extra_info('peername'))

    while True:
        msg = await read_json_doc(reader)
        if msg:
            logging.debug("Enqueuing message: %s", json.dumps(msg))
            await inqueue.put( msg )
        else:
            logging.debug("End of stream detected, from %r",
                          writer.get_extra_info('peername'))
            await writer.close() # close the socket to the producer
            await inqueue.put(None) # let consume know this stream is complete
            break


def start_server():
    """ Starts the server as a coroutine. """
    global server_iface, server_port

    loop = asyncio.get_event_loop()

    factory = asyncio.start_server(handle_connection, server_iface, server_port)
    server = loop.run_until_complete(factory)


async def handle_connection_curio(client, addr):
    logging.info("Handling inbound connection from %r", addr)
    s = client.as_stream()

    while True:
        msg = await read_json_doc(s)
        if not msg:
            break

        logging.debug("Enqueuing message in inqueue")
        await inqueue.put( msg )

    logging.debug("End of stream detected, from %r", addr)
    await client.close()
    await inqueue.put(None)


async def start_server_curio(*args):
    logging.info("Starting TCP server")
    await curio.spawn( curio.tcp_server, server_iface, server_port, handle_connection_curio)


async def server_curio(message_stub=None, config=None, outqueue=None):
    s = curio.socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind( (server_iface, server_port) )
    s.listen(5)
    logging.info("Server listening at %s:%d", server_iface, server_port)
    while True:
        async with s:
            client, addr = await s.accept()
            await curio.spawn(handle_connection_curio, client, addr)


async def dummy_wait():
    while True:
        await curio.sleep(10)


def production():
    """
    Production config: kick off the server and let the sensor wrapper
    spawn the message handling coroutine.
    """
    from sensor_wrapper import SensorWrapper

    plat = platform.system().lower()
    if plat not in ("linux", "windows"):
        raise RuntimeError("Unknown platform: {}".format(plat))

    stop_cb = { "linux" : None,
                "windows" : dummy_wait}[plat]

    wrapper = SensorWrapper( "desktop{}".format(plat),
                             [handle_messages, start_server_curio],
                             stop_notification=stop_cb )

    # Don't add any args here; they're handled by global arg parser in main block
    wrapper.start()


async def run_standalone():
    async with curio.TaskGroup(wait=all) as g:
        await g.spawn( start_server_curio )
        await g.spawn( handle_messages )


def standalone():
    """
    Standalone config: kick off server and message handling
    coroutine.
    """
    curio.run( run_standalone )


if __name__ == "__main__":
    if os.getenv('VERBOSE'):
        logging.basicConfig( level=logging.DEBUG )
    else:
        logging.basicConfig( level=logging.INFO )

    # Should be called via run-venv.sh or run-venv.bat, so don't check
    # args that carefully.
    (server_iface, server_port) = os.getenv('LISTEN_PORT').split(':')
    server_port = int(server_port)

    if os.getenv('STANDALONE'):
        standalone()
    else:
        production()
