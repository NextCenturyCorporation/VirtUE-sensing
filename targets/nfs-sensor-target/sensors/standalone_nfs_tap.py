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

sys.path.append( os.path.join( os.path.dirname( os.path.realpath(__file__) ),
                               '..', 'sensor_libraries', 'scapy-repo' ) )

from scapy.all import * # sniff, rdpcap
import nfs
import nfs_const

import nfs_packet_handler as handler
import nfs_state_monitor as state

# Keep sniffing?
sniff_sockets = True

# This breaks parts of TCP or UDP parsers

# conf.debug_dissector = 1


async def recv_pkt( pkt ):
    r = handler.pkt_handler_standard( pkt )
    if not r:
        return
    (level, data) = r

    logmsg = { "timestamp" : datetime.now().isoformat(),
               "level"     : level,
               "action"    : data }

    print( logmsg )


async def async_sniff( loop,iface ):
    """
    Kick off the NFS sniffer (a long-running activity).
    """

    global sniff_sockets

    print(" ::starting NFS sniffer")
    print("     using network interface {}".format( iface ) )

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
                await recv_pkt( p )


async def proc_all_pkts( pkts ):
    for p in pkts:
        await recv_pkt( p )


if __name__ == "__main__":
    # setup commandline arguments
    parser = argparse.ArgumentParser( description='NFS Protocol Sniffer' )
    parser.add_argument( '--iface', action="store",
                         dest="iface", default=None )

    parser.add_argument( "--pcap", action="store",
                         dest="pcap", default=None )

    # parse arguments
    args = parser.parse_args()
    logging.basicConfig( level=logging.DEBUG )

    if args.iface and args.pcap:
        parser.error( "Cannot specify both --iface and --pcap" )
        sys.exit( 1 )
    if not args.iface and not args.pcap:
        parser.error( "Must specify one of --iface or --pcap" )
        sys.exit( 1 )

    nfs.init()

    loop = asyncio.get_event_loop()

    if args.pcap:
        loop.run_until_complete( proc_all_pkts( pkts ) )
    else:
        loop.run_until_complete( async_sniff( loop, args.iface ) )
