#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

import os
import sys
import argparse
import logging

sys.path.append( os.path.join( os.path.dirname( os.path.realpath(__file__) ),
                               'scapy-repo' ) )

from scapy.all import * #sniff, rdpcap
import nfs
import nfs_const


# conf.debug_dissector = 1
# packages: python-libpcap, scapy-python3(pip)

xid_call_map = dict()
xid_auth_map = dict()

ct = 0
def recv_pkt( pkt ):
    global ct
    ct += 1

    print( "#{}".format( ct ) )
    rpc = pkt.getlayer( nfs.RPC_Header )
    if not rpc:
        return

    authsys = None
    print( "#{} - RPC: XID {:x} dir {} all {}".format( ct, rpc.xid, rpc.direction, rpc.show() ) )
    if True:
        return

    # Map XID --> call pkt for response lookup
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        xid_call_map[ rpc.xid ] = pkt

        rpccall = pkt.getlayer( nfs.RPC_Call )
        if not rpccall:
            import pdb;pdb.set_trace()
        assert rpccall

        auth = rpccall.auth
        if auth and auth.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ] :
            xid_auth_map[ rpc.xid ] = auth
            authsys = auth.sys

    else:
        callpkt = xid_call_map[ rpc.xid ]
        del xid_call_map[ rpc.xid ]

    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        if authsys:
            import pdb;pdb.set_trace()
            print( "XID {} Machine '{}' UID {} GID {}".
                   format( rpc.xid, authsys.machine_name, authsys.uid, authsys.gid ) )

        
        
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
    
    #print( "break  PacketField.m2i" )
    #import pdb;pdb.set_trace()

    if args.iface and args.pcap:
        parser.error( "Cannot specify both --iface and --pcap" )
        sys.exit( 1 )

    if args.pcap:
        pkts = rdpcap( args.pcap )

        for p in pkts:
            recv_pkt( p )
        #for i,pkt in enumerate( pkts ):
        #    if i in (75,76):
        #        recv_pkt( pkt )
    else:
        sniff( iface=args.iface,
               filter="tcp or upd",
               store=False,
               prn=recv_pkt )
    #prn=lambda x: x.summary())

    '''
    max_bytes = 1024
    promiscuous = True
    read_timeout = 100 # in milliseconds
    pc = pcapy.open_live( given_args.iface,
                          max_bytes, 
                          promiscuous,
                          read_timeout)

    pc.setfilter('tcp')
    
    packet_limit = 10 # -1
    pc.loop(packet_limit, recv_pkts) # capture packets
    '''
