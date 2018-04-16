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
import json

import datetime

from curio import sleep
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


# Global timeouts (in seconds)
select_timeout = 0.10
sleep_timeout  = 0.01

# This breaks parts of TCP or UDP parsers
# conf.debug_dissector = 1


async def recv_pkt( pkt, message_stub, message_queue ):
    r = handler.pkt_handler_standard( pkt )
    if not r:
        return
    (level, data) = r

    logmsg = { "timestamp" : datetime.now().isoformat(),
               "level"     : level,
               "action"    : data }

    # interpolate everything from our message stub
    logmsg.update( message_stub )

    await message_queue.put( json.dumps( logmsg ) )


async def nfs3_sniff_iface(message_stub, config, message_queue):
    """
    Kick off the NFS sniffer (a long-running activity).
    """

    global sniff_sockets

    iface = find_nfs_iface()
    print(" ::starting NFS sniffer")
    print("     using network interface {}".format( iface ) )

    sock = conf.L2listen(type=ETH_P_ALL, iface=iface)

    # Register the socket with an async-friendly select(). Once
    # there's a packet to read, let the parser deal with it. Our
    # packet handler, recv_pkt, has to be a coroutine, so we can't use
    # the cleaner loop.add_reader(). Think of this block as a poor
    # man's sniff() function.

    # @TODO: cleanup this loop, make better use of coroutines
    with selectors.DefaultSelector() as sel:
        sel.register( sock, selectors.EVENT_READ )

        while sniff_sockets:
            for sk, events in sel.select():
                p = sock.recv()
                if not p:
                    break

                # There's a packet to process: receive it and yield
                # for message_queue.put()
                p.sniffed_on = sock
                await recv_pkt( p, message_stub, message_queue )
                await sleep( sleep_timeout )

async def nfs3_process_pkts( message_stub, config, message_queue ):
    pkts = rdpcap( wrapper.opts.pcap )

    for p in pkts:
        await recv_pkt( p, message_stub, message_queue )


async def nfs3_sense( message_stub, config, message_queue ):

    if wrapper.opts.iface and wrapper.opts.pcap:
        raise RuntimeError( "Cannot specify both --iface and --pcap" )
    if not wrapper.opts.iface and not wrapper.opts.pcap:
        raise RuntimeError( "Must specify one of --iface or --pcap" )

    nfs.init()

    if wrapper.opts.iface:
        await nfs3_sniff_iface( message_stub, config, message_queue )
    else:
        await nfs3_process_pkts( message_stub, config, message_queue )


def find_nfs_iface():
    print( "Returning network interface" )
    return wrapper.opts.iface


if __name__ == "__main__":
    logging.basicConfig( level=logging.DEBUG )

    wrapper = SensorWrapper( "NFS", [nfs3_sense,] )

    wrapper.argparser.add_argument( "--iface", dest="iface", default=None,
                                    help="Network interface to sniff" )
    wrapper.argparser.add_argument( "--pcap", dest="pcap", default=None,
                                    help="Pcap file to parse" )

    wrapper.start()
