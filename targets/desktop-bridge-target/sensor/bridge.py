#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Async Desktop Bridge sensor - forwards JSON messages to the API
## sensing infrastructure.
##

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

# Keep sniffing?
sniff_sockets = True

# Global timeouts (in seconds)
select_timeout = 0.10
sleep_timeout  = 0.01

opts = None

async def wait_for_json(stream):
    """
    Because everything is broken, nothing works, and the internet only stays up
    because everything is coded to assume everyone else is a brain-dead mole rat
    with ADHD.

    Fuh.

    If we try and do a normal read of the request body from a stream, curio will
    let us happily block until the connection is timed out by the client. So, we
    need to detect when we've reached the end of the request body. In this case,
    that means repeatedly checking to see if our results are JSON parse-able. If
    we can parse it, return the decoded JSON data.
    
    :param stream: network socket
    :return: decoded JSON data
    """

    body = ""
    decoded = None
    while True:
        line = await stream.recv(1024)
        body += line.decode("utf-8")
        logging.info(body)
        try:
            decoded = json.loads(body)
            break
        except json.decoder.JSONDecodeError as _jde:
            # ignore this error - carry on!
            pass
    return decoded

async def handle_client(conn, message_stub, config, message_queue):
    global opts

    while True:
        json = await wait_for_json(conn)

        meta = { "timestamp" : datetime.datetime.now().isoformat() }
        json.update(meta)

        if message_stub:
            json.update(message_stub)

        if opts.standalone:
            logging.debug("Message: %s", json)
        else:
            await message_queue.put( json.dumps(json) )

    
async def start_server(message_stub, config, message_queue):
    global opts
    (host,port) = opts.listen.split(':')

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind( (host,int(port)) )
    #server.setblocking(False)
    server.listen(3)
    logging.info("Awaiting connection")

    while True:
        try:
            conn, client = server.accept()
            logging.info("Handling client %r", client)
        except BlockingIOError:
            continue
        await handle_client(conn, message_stub, config, message_queue)

def production():
    global opts
    
    from sensor_wrapper import SensorWrapper

    wrapper = SensorWrapper( "desktop", [start_server] )

    # Don't add any args here; they're handled by global arg parser in main block
    wrapper.start()


def standalone():
    global opts

    loop = asyncio.get_event_loop()
    loop.run_until_complete( start_server(None, None, None) )

    
if __name__ == "__main__":
    logging.basicConfig( level=logging.DEBUG )

    usage_string = """usage: %s [...]""" % sys.argv[0]
    parser = argparse.ArgumentParser(description=usage_string)

    # Global arguments: apply to both standalone and production mode
    parser.add_argument( "--standalone", dest="standalone", action="store_true",default=False,
                         help="Run in standalone mode? Default: false (production)")
    parser.add_argument("--listen", 
                        help="TCP/IP address to listen on, e.g. 0.0.0.0:3333" )
    #global opts
    opts = parser.parse_args()

    if not opts.listen:
        parser.print_help()
        sys.exit(1)

    #if os.getenv('STANDALONE'):
    if opts.standalone:
        standalone()
    else:
        production()
        
    
