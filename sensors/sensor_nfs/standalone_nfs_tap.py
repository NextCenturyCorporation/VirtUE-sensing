#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

import os
import sys
import argparse
import logging

sys.path.append( os.path.join( os.path.dirname( os.path.realpath(__file__) ),
                               'scapy-repo' ) )

from scapy.all import * # sniff, rdpcap
import nfs
import nfs_const

import nfs_packet_handler as handler

# conf.debug_dissector = 1
# packages: python-libpcap, scapy-python3(pip)


def recv_pkt( pkt ):
    handler.standalone_handler( pkt )
        
if __name__ == '__main__':
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

    if args.pcap:
        pkts = rdpcap( args.pcap )

        for p in pkts:
            recv_pkt( p )
    else:
        sniff( iface=args.iface,
               filter="tcp or upd",
               store=False,
               prn=recv_pkt )
