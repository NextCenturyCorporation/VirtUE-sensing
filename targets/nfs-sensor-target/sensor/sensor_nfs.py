#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Async NFS packet processor, similar to sensor implementation
##

import os
import sys
import argparse
import logging
import pdb

import datetime

import asyncio
import selectors
import subprocess

from scapy.all import * # sniff, rdpcap

import nfs
import nfs_const

import nfs_packet_handler as handler
import nfs_state_monitor as state


from sensor_wrapper import SensorWrapper


# Keep sniffing?
sniff_sockets = True

# This breaks parts of TCP or UDP parsers
# conf.debug_dissector = 1

# Some globals are needed due to constraints on recv_pkt() callback signature

#message_queue = None
#message_stub  = None
#network_iface = None


async def recv_pkt( pkt, message_queue ):
    r = handler.pkt_handler_standard( pkt )
    if not r:
        return
    (level, data) = r

    logmsg = { "timestamp" : datetime.now().isoformat(),
               "level"     : level,
               "action"    : data }

    # interpolate everything from our message stub
    logmsg.update( message_stub )

    await message_queue.put( logmsg )


async def nfs3_sniff(message_stub, config, message_queue):
    """
    Kick off the NFS sniffer (a long-running activity).
    """

    global sniff_sockets

    iface = find_nfs_iface()
    print(" ::starting NFS sniffer")
    print("     using network interface {}".format( iface ) )

    p = subprocess.Popen( "ip link show", stdout=subprocess.PIPE)
    print("     Interfaces:" )
    async for line in p.stdout:
        print("    " + line )
    p.close()

    nfs.init()

    sock = conf.L2listen(type=ETH_P_ALL, iface=iface)

    # Register the socket with an async-friendly select(). Once
    # there's a packet to read, let the parser deal with it. Our
    # packet handler, recv_pkt, has to be a coroutine, so we can't use
    # the cleaner loop.add_reader().

    with selectors.DefaultSelector() as sel:
        sel.register( sock, selectors.EVENT_READ )

        while sniff_sockets:
            for sk, events in sel.select():
                p = sock.recv()
                if not p:
                    break
                p.sniffed_on = sock
                await recv_pkt( p, message_queue )

def find_nfs_iface():
    print( "Returning network interface" )
    return wrapper.opts.iface

if __name__ == "__main__":
    logging.basicConfig( level=logging.DEBUG )

    wrapper = SensorWrapper( "NFS", [nfs3_sniff,] )

    wrapper.argparser.add_argument( "--iface", dest="iface", default=None,
                                    help="Network interface to sniff" )
    wrapper.start()
