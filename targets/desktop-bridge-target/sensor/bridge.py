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
"""

import os
import sys
import argparse
import logging
import pdb
import json
import socket

import datetime

from curio import sleep
import asyncio
import selectors

inqueue = asyncio.Queue()
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
    
    while True:
        logging.debug("Awaiting data from inbound queue")
        injson = await inqueue.get()
        logging.debug("Read msg from inqueue")
        if not injson:
            # Sentinal value found; consume it and move on
            inqueue.task_done()
            continue

        # read message from queue, put in outqueue
        meta = { "timestamp" : datetime.datetime.now().isoformat() }
        injson.update(meta)

        msg_ct += 1
        if message_stub:
            injson.update(message_stub)

        if os.getenv('STANDALONE'): #opts.standalone:
            logging.debug("JSON message # %d", msg_ct)
        else:
            await message_queue.put( json.dumps(injson) )
        inqueue.task_done()


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
        line = await stream.readline()
        # line = await stream.read(1) # slow alternate
        if not line:
            decoded = None
            break

        body += line.decode("utf-8")
        logging.info(body)
        try:
            decoded = json.loads(body)
            break
        except json.decoder.JSONDecodeError as _jde:
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
            meta = { "timestamp" : datetime.datetime.now().isoformat() }
            msg.update(meta)

            logging.debug("Enqueuing message")
            await inqueue.put( msg )
        else:
            logging.debug("End of stream detected, from %r",
                          writer.get_extra_info('peername'))
            writer.close()
            await inqueue.put(None)
            break


def start_server():
    """ Starts the server as a coroutine. """
    global server_iface, server_port

    loop = asyncio.get_event_loop()

    factory = asyncio.start_server(handle_connection, server_iface, server_port)
    server = loop.run_until_complete(factory)
        
    logging.info("Awaiting connection")


def production():
    """
    Production config: kick off the server and let the sensor wrapper
    spawn the message handling coroutine.
    """
    global server_iface, server_port
    
    from sensor_wrapper import SensorWrapper

    start_server()

    wrapper = SensorWrapper( "desktop", [handle_messages] )

    # Don't add any args here; they're handled by global arg parser in main block
    wrapper.start()


def standalone():
    """
    Standalone config: kick off server and message handling
    coroutine.
    """

    loop = asyncio.get_event_loop()
    start_server()
    loop.create_task( handle_messages(None,None,None) )
    loop.run_forever()

    
if __name__ == "__main__":
    if os.getenv('VERBOSE'):
        logging.basicConfig( level=logging.DEBUG )
    else:
        logging.basicConfig( level=logging.INFO )

    # Should be called via run.sh, so don't check args that carefully
    (server_iface, server_port) = os.getenv('LISTEN_PORT').split(':')
    server_port = int(server_port)

    if os.getenv('STANDALONE'):
        standalone()
    else:
        production()
