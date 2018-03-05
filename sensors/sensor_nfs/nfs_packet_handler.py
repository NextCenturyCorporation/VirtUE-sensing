#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Handles incoming packet stream, without regard to how the stream is
## acquired. Selects metadata from stream for reporting to sensor
## system.
##

import os
import sys
import argparse
import logging
import pdb

from scapy.all import * #sniff, rdpcap
import nfs
import nfs_const

pkt_ct = 0
xid_auth_map = dict()

handle_name_map = dict()
name_handle_map = dict()

def _handle_nop( rpc_call, rpc_reply ):
    logging.debug( "#{} RPC program {} procedure {} not handled"
                   .format( pkt_ct, rpc_call.prog, rpc_call.proc ) )

def _handle_nfs3_null( rpc_call, rpc_reply ):
    logging.debug( "#{} NFSnull: " )

def _handle_nfs3_getattr( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS getattr: {}"
                   .format( pkt_ct, name ) )

def _handle_nfs3_setattr( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.args.handle ]
    logging.debug( "#{} NFS setattr: {}"
                   .format( pkt_ct, name ) )

def _handle_nfs3_lookup( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.args.handle ]
    logging.debug( "#{} NFS lookup: {}"
                   .format( pkt_ct, name ) )

def _handle_nfs3_access( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS access {}"
                   .format( pkt_ct, name )  )

def _handle_nfs3_readlink( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS readlink:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_read( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS read:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_write( rpc_call, rpc_reply ):
    pdb.set_trace()
    name = handle_name_map[ rpc_call.handle ]
    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        logging.debug( "#{} NFS write: {} bytes ofs {}"
                       .format( pkt_ct, name, rpc_call.count, rpc_call.offset ) )
    else:
        logging.debug( "#{} NFS write: failed {}"
                       .format( pkt_ct, rpc_reply.nfs_status ) )

def _handle_nfs3_create( rpc_call, rpc_reply ):
    handle = rpc_call.args.handle
    name = handle_name_map[ handle ]
    
    logging.debug( "#{} NFS create: {} {} "
                   .format( pkt_ct, name, handle )  )

def _handle_nfs3_mkdir( rpc_call, rpc_reply ):
    hparent = rpc_call.args.handle
    dparent = handle_name_map[ hparent ]
    pdb.set_trace()
    new = os.path.join( dparent, str(rpc_call.args.fname) )

    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        if rpc_reply.handle.handle_follows:
            h = rpc_reply.handle.handle
            name_handle_map[ new ] = h
            handle_name_map[ h ] = new
            logging.debug( "#{} NFS mkdir: {} --> {}"
                           .format( pkt_ct, new, h ) )
    else:
        logging.debug( "#{} NFS mkdir: {} failed {}"
                       .format( pkt_ct, new, rpc_reply.nfs_status ) )

def _handle_nfs3_symlink( rpc_call, rpc_reply ):
    hparent = rpc_call.args.handle
    dparent = handle_name_map[ hparent ]
    new = os.path.join( dparent, rpc_call.args.fname )

    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        logging.debug( "#{} NFS symlink: {} --> {}"
                       .format( pkt_ct, new, rpc_reply.handle ) )
        name_handle_map[ new ] = rpc_reply.handle
        handle_name_map[ rpc_reply.handle ] = new
    else:
        logging.debug( "#{} NFS symlink: {} failed {}"
                       .format( pkt_ct, new, rpc_reply.nfs_status ) )

        
def _handle_nfs3_mknod( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS mknod:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_remove( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS remove:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_rmdir( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS rmdir:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_rename( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS rename:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_link( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS link:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_readdir( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS readdir: {} status {}"
                   .format( pkt_ct, name, rpc_reply.nfs_status ) )

def _handle_nfs3_readdirplus( rpc_call, rpc_reply ):
    name = handle_name_map[ rpc_call.handle ]
    logging.debug( "#{} NFS readdirplus: {} status {}"
                   .format( pkt_ct, name, rpc_reply.nfs_status ) )

def _handle_nfs3_fsstat( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS fsstat:"
                   .format( pkt_ct, name ) )

def _handle_nfs3_fsinfo( rpc_call, rpc_reply ):
    hparent = rpc_call.root
    dparent = handle_name_map[ hparent ]

    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        logging.debug( "#{} NFS fsinfo: {}"
                       .format( pkt_ct, dparent ) )
    else:
        logging.debug( "#{} NFS fsinfo: {} failed {}"
                       .format( pkt_ct, new, rpc_reply.nfs_status ) )

def _handle_nfs3_pathconf( rpc_call, rpc_reply ):
    hparent = rpc_call.path
    dparent = handle_name_map[ hparent ]

    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        logging.debug( "#{} NFS pathinfo: {}"
                       .format( pkt_ct, dparent ) )
    else:
        logging.debug( "#{} NFS pathinfo: {} failed {}"
                       .format( pkt_ct, new, rpc_reply.nfs_status ) )

def _handle_nfs3_commit( rpc_call, rpc_reply ):
    hparent = rpc_call.args.handle
    dparent = handle_name_map[ hparent ]
    new = os.path.join( dparent, rpc_call.args.fname )
    pdb.set_trace()
    if rpc_reply.nfs_status == nfs_const.NFS3_OK:
        logging.debug( "#{} NFS mkdir: {} --> {}"
                       .format( pkt_ct, new, rpc_reply.handle ) )
        name_handle_map[ new ] = rpc_reply.handle
        handle_name_map[ rpc_reply.handle ] = new
    else:
        logging.debug( "#{} NFS mkdir: {} failed {}"
                       .format( pkt_ct, new, rpc_reply.nfs_status ) )
    logging.debug( "#{} NFS commit:"
                   .format( pkt_ct, name ) )



nfs3_handler_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL' ]        : _handle_nfs3_null,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR' ]     : _handle_nfs3_getattr,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR' ]     : _handle_nfs3_setattr,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP' ]      : _handle_nfs3_lookup,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS' ]      : _handle_nfs3_access,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK' ]    : _handle_nfs3_readlink,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ' ]        : _handle_nfs3_read,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE' ]       : _handle_nfs3_write,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE' ]      : _handle_nfs3_create,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR' ]       : _handle_nfs3_mkdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK' ]     : _handle_nfs3_symlink,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD' ]       : _handle_nfs3_mknod,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE' ]      : _handle_nfs3_remove,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR' ]       : _handle_nfs3_rmdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME' ]      : _handle_nfs3_rename,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK' ]        : _handle_nfs3_link,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR' ]     : _handle_nfs3_readdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : _handle_nfs3_readdirplus,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT' ]      : _handle_nfs3_fsstat,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO' ]      : _handle_nfs3_fsinfo,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF' ]    : _handle_nfs3_pathconf,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT' ]      : _handle_nfs3_commit,
}


def _handle_mount3_mnt( rpc_call, rpc_reply ):
    handle_name_map[ rpc_reply.handle ] = str(rpc_call.path)
    name_handle_map[ str(rpc_call.path) ] = rpc_reply.handle
    logging.debug( "#{} MOUNT mnt: {} --> {:}"
                   .format( pkt_ct, rpc_reply.handle, str(rpc_call.path) ) )
    
def _handle_mount3_umnt( rpc_call, rpc_reply ):
    handle = name_handle_map[ rpc_call.path ]
    logging.debug( "#{} MOUNT umnt: {} --> {}"
                   .format( pkt_ct, handle, rpc_call.path ) )
    del handle_name_map[ handle ]
    del name_handle_map[ str(rpc_call.path) ]

def _handle_mount3_umnt_all( rpc_call, rpc_reply ):
    logging.debug( "#{} MOUNT umnt_all:".format( pkt_ct ) )
    pass
    
mount3_handler_lookup = {
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_NULL'    ] : _handle_nop,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_MNT'     ] : _handle_mount3_mnt,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_DUMP'    ] : _handle_nop,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNT'    ] : _handle_mount3_umnt,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNTALL' ] : _handle_mount3_umnt_all,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_EXPORT'  ] : _handle_nop,
}


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
        pass #rpccall = xid_call_map[ rpc.xid ]
        
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

    #logging.debug( "#{} - RPC: XID {:x} dir {} packet {}"
    #               .format( pkt_ct, rpc.xid, rpc.direction, repr(rpc) ) )
    
    #rpc.show()

    # Ignore calls. The NFS parser binds replies to their calls. We
    # examine only replies so we have all the info at hand.

    #if nfs_const.MsgTypeValMap['CALL'] == rpc.direction:
    #    logging.debug( "Ingoring call side of XID {:x}".format( rpc.xid ) )
    #    return

    #import pdb;pdb.set_trace()
    
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
    #_state_tracker_rpc( rpc )

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

def _handler_core( pkt ):
    _state_tracker( pkt )

    #if pkt_ct in (67,68,):
    #    import pdb;pdb.set_trace()

    rpc = pkt.getlayer( nfs.RPC_Header )
    if not rpc:
        logging.debug( "#{} not RPC".format( pkt_ct ) )
        return

    #logging.debug( "#{} - RPC: XID {:x} dir {} packet {}"
    #               .format( pkt_ct, rpc.xid, rpc.direction, repr(rpc) ) )
    #rpc.show()

    # Ignore calls. The NFS parser binds replies to their calls. We
    # examine only replies so we have all the info at hand.

    if not rpc.rpc_call: # call packets have None for call field
        logging.debug( "#{} Ignoring call side of XID {:x}"
                       .format( pkt_ct, rpc.xid ) )
        return

    rpc_call = rpc.rpc_call[0]

    if rpc_call.prog not in ( nfs_const.ProgramValMap['mountd'],
                              nfs_const.ProgramValMap['nfs']  ):
        logging.debug( "Ignoring program {} call/reply with XID {:x}".
                       format( rpc_call.prog, rpc.xid ) )
        return
    #import pdb;pdb.set_trace()

    
    if rpc_call.prog == nfs_const.ProgramValMap['mountd']:
        # This is a mountd reply. Call its handler with the (call, reply) pair
        mount3_handler_lookup[ rpc_call.proc ] ( rpc_call, rpc )

    elif rpc_call.prog == nfs_const.ProgramValMap['nfs']:
        # This is an NFS reply. Call its handler with the (call, reply) pair
        nfs3_handler_lookup[ rpc_call.proc ] ( rpc_call, rpc )

def null_handler( pkt ):
    pass


def raw_print_handler( pkt ):
    _state_tracker( pkt )
    logging.debug( "#{} - packet {}"
                   .format( pkt_ct, repr(pkt) ) )

    
def standalone_handler( pkt ):
    _handler_core( pkt )
