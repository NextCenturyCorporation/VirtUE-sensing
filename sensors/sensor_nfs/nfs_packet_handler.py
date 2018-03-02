#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Handles incoming packets, either in standalone mode (meaning packet
## information is printed to the logging system) or in sensor mode,
## meaning the packets are reported to the sensor system.
##

import os
import sys
import argparse
import logging

from scapy.all import * #sniff, rdpcap
import nfs
import nfs_const

xid_call_map = dict()
xid_auth_map = dict()
handle_name_map = dict()

ct = 0
global ct

def _state_tracker_rpc( rpc ):
    assert isinstance( rpc, nfs.RPC_Header )
    # Map XID --> call pkt for response lookup
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        xid_call_map[ rpc.xid ] = pkt
    else:
        del xid_call_map[ rpc.xid ]        


def _state_tracker( pkt ):
    global ct
    ct += 1
    rpc = pkt.getlayer( nfs.RPC_Header )
    if not rpc:
        print( "#{} not RPC".format( ct ) )
        return

    authsys = None
    print( "#{} - RPC: XID {:x} dir {} packet {}".format( ct, rpc.xid, rpc.direction, repr(rpc) ) )
    rpc.show()


    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        # handle a call
        rpccall = pkt.getlayer( nfs.RPC_Call )
        import pdb;pdb.set_trace()
        auth = rpccall.auth
        import pdb;pdb.set_trace()
        if auth and auth.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ] :
            xid_auth_map[ rpc.xid ] = auth
            authsys = auth.sys
        import pdb;pdb.set_trace()
        
    else:
        # handle response
        pass

    # We're done with whatever RPC state we needed for this
    # packet. Conversely, we can wait until now to record RPC state.
    _state_tracker_rpc( rpc )

    '''
    else:
        return
        callpkt = xid_call_map[ rpc.xid ]
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        if authsys:
            import pdb;pdb.set_trace()
            print( "XID {} Machine '{}' UID {} GID {}".
                   format( rpc.xid, authsys.machine_name, authsys.uid, authsys.gid ) )
    '''
    
    
def null_handler( pkt ):
    pass


def standalone_handler( pkt ):
    _state_tracker( pkt )
