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

pkt_ct = 0
#xid_call_map = dict()
xid_auth_map = dict()
handle_name_map = dict()

#prog_call_lookup = { nfs_const.ProgramValMap['portmapper'] : PORTMAP_Call,
#                     nfs_const.ProgramValMap['mountd']     : MOUNT_Call,
#                     nfs_const.ProgramValMap['mountd']     : 'NFS' }

def _get_program_pkt( rpc ):

    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        rpccall = pkt.getlayer( nfs.RPC_Call )
        return nfs_const.ValProgramMap.get( rpccall.prog, None )
    else:
        pass
    
def _get_procedure_pkt( rpc ):
    prog = None
    if prog is 'NFS':
        prog = nfs.nfs3_call_lookup[ prog.proc ]
    else:
        pass #xid_call_map[ rpc.xid ]

def _get_rpc_call( rpc ):
    """
    Gets the RPC call for the given packet. In case this packet is a
    reply, returns the associated call.
    """
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        rpccall = rpc.getlayer( nfs.RPC_Call )
    else:
        
        rpccall = None
    return rpccall
        
def _state_tracker_rpc( rpc ):
    assert isinstance( rpc, nfs.RPC_Header )
    # Map XID --> call pkt for response lookup
    rpccall = _get_rpc_call( rpc )
    if rpccall:
        #xid_call_map[ rpc.xid ] = rpccall
        xid_auth_map[ rpc.xid ] = rpc.auth_sys
    else:
        #xid_call_map.pop( rpc.xid, None )
        xid_auth_map.pop( rpc.xid, None )

def _state_port_bindings( rpc ):
    reply = rpc.getlayer( nfs.PORTMAP_GETPORT_Reply )
    if not reply:
        return False

    #import pdb;pdb.set_trace()
    if 0 == rpc.state and 0 == rpc.accept_state:
        # The port binding was successfully found
        return
        request = xid_call_map[ rpc.xid ]

        nfs.bind_port_to_rpc( reply.port,
                              bind_udp=(request.proto == nfs_const.ProtoValMap['UDP']),
                              bind_tcp=(request.proto == nfs_const.ProtoValMap['TCP']) )
    return True


def _get_nfs_layer( rpc ):
    rpccall = _get_rpc_call( rpc )
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        pass
    if rpccall and rpccall.prog == nfs_const.ProgramValMap['nfs']:
        return nfs.nfs3_call_lookup[ rpccall.proc ]
    else:
        pass#rpccall = xid_call_map[ rpc.xid ]
        
        # if rpc.xid in xid_call_map

def _state_tracker( pkt ):
    global pkt_ct
    pkt_ct += 1
    rpc = pkt.getlayer( nfs.RPC_Header )

    if pkt_ct in (28,29):
        #import pdb;pdb.set_trace()
        pass
    if not rpc:
        logging.debug( "#{} not RPC".format( pkt_ct ) )
        return

    logging.debug( "#{} - RPC: XID {:x} dir {} packet {}"
                   .format( pkt_ct, rpc.xid, rpc.direction, repr(rpc) ) )
    rpc.show()

    # Dynamically discover and bind new RPC protocols. True means this was done!
    #if _state_port_bindings( rpc ):
    #    _state_tracker_rpc( rpc )
    #    return
    '''
    if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
        # handle a call
        rpccall = pkt.getlayer( nfs.RPC_Call )
        who = rpccall.auth_sys # None if AUTH_NULL used

        import pdb;pdb.set_trace()
        
    else:
        # handle response
        pass
    '''
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
