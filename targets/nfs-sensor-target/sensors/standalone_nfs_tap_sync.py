#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018


import sys
import os

sys.path.append( os.path.join( os.path.dirname( os.path.realpath(__file__) ),
                               '..', 'sensor_libraries', 'scapy-repo' ) )

import argparse
import logging
import pdb
import datetime

from scapy.all import * # sniff, rdpcap
import nfs
import nfs_const

import nfs_packet_handler as handler
import nfs_state_monitor as state

# Keep sniffing?
sniff_sockets = True

# This breaks parts of TCP or UDP parsers

# conf.debug_dissector = 1


def recv_pkt( pkt ):
    r = handler.pkt_handler_standard( pkt )
    if not r:
        return
    (level, data) = r
    
    logmsg = { "timestamp" : datetime.now().isoformat(),
               "level"     : level,
               "action"    : data }

    print( logmsg )

    
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

    if args.pcap:
        sniff( offline=args.pcap, prn=recv_pkt )
    else:
        sniff( iface=args.iface, prn=recv_pkt )
    
