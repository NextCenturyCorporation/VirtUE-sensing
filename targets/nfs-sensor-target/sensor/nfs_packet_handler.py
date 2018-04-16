#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Handles incoming packet stream, without regard to how the stream is
## acquired. Selects metadata from stream for reporting to sensor
## system. Must be used via the sensor system; does not work in
## standalone mode. Each handler returns a dictionary for use by the
## main sensor code, or None.
##

import os
import sys
import argparse
import logging
import pdb

from scapy.all import * #sniff, rdpcap
import nfs
import nfs_const

import nfs_state_monitor as state


debug_level = "debug"
info_level  = "info"
warn_level  = "warn"
error_level  = "error"
event_level  = "event"


def _handle_nop( rpc_call, rpc_reply ):
    logging.debug( "#{} RPC program {} procedure {} not handled"
                   .format( state.pkt_ct, rpc_call.prog, rpc_call.proc ) )
    return None

def _handle_nfs3_null( rpc_call, rpc_reply ):
    return ( debug_level,
             { "proto"   : "nfs",
               "command" : "NULL",
               "path"    : None,
               "who"     : None,
               "status"  : None } )

def _handle_nfs3_getattr( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "getattr",
               "who"     : state.get_auth_str(rpc_call),
               "path"    : str( path ),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_setattr( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    logging.debug( "#{} NFS setattr: {} to {} by {} status {}"
                   .format( state.pkt_ct, path, str(rpc_call.new_attr),
                            state.get_auth_str(rpc_call), str( rpc_reply.nfs_status ) ) )
    return ( info_level,
             { "proto"   : "nfs",
               "command" : "setattr",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "new_attr": str(rpc_call.new_attr), } )

def _handle_nfs3_lookup( rpc_call, rpc_reply ):
    newpath = os.path.join( state.get_path( rpc_call.args.handle ),
                            str(rpc_call.args.fname) )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "lookup",
               "path"    : str( newpath ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_access( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )
    check = rpc_call.check_access

    access = None
    if rpc_reply.nfs_status.success():
        access = rpc_reply.access

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "access",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "access"  : access } )

def _handle_nfs3_readlink( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    target = None
    if rpc_reply.nfs_status.success():
        target = rpc_reply.path

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "readlink",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "target"  : target } )


def _handle_nfs3_read( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "read",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_write( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "write",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_create( rpc_call, rpc_reply ):
    path = os.path.join( state.get_path( rpc_call.args.handle ),
                         str( rpc_call.args.fname ) )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "create",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_mkdir( rpc_call, rpc_reply ):
    path = os.path.join( state.get_path( rpc_call.args.handle ),
                         str( rpc_call.args.fname ) )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "mkdir",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_symlink( rpc_call, rpc_reply ):
    # Where symlink is created
    target = os.path.join( state.get_path( rpc_call.where.handle ),
                           str( rpc_call.where.fname ) )
    # Source info
    source = str( rpc_call.symlink.path )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "symlink",
               "path"    : source,
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "target"  : target } )

def _handle_nfs3_mknod( rpc_call, rpc_reply ):

    # TODO: implement call/reply in nfs.py
    return ( info_level,
             { "proto"   : "nfs",
               "command" : "mknod",
               "path"    : None,
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_remove( rpc_call, rpc_reply ):
    path = os.path.join( state.get_path( rpc_call.args.handle ), rpc_call.args.name )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "remove",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_rmdir( rpc_call, rpc_reply ):
    path = os.path.join( state.get_path( rpc_call.args.handle ), rpc_call.args.name )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "rmdir",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_rename( rpc_call, rpc_reply ):
    from_name = os.path.join( state.get_path( rpc_call.ffrom.handle ),
                              str(rpc_call.ffrom.fname) )
    to_name   = os.path.join( state.get_path( rpc_call.fto.handle ),
                              str(rpc_call.fto.fname) )
    return ( info_level,
             { "proto"   : "nfs",
               "command" : "rename",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "target"  : to_name } )

def _handle_nfs3_link( rpc_call, rpc_reply ):
    source = state.get_path( rpc_call.handle )
    dest   = os.path.join( state.get_path( rpc_call.link.handle ), rpc_call.link.fname )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "link",
               "path"    : source,
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ),
               "target"  : dest } )

def _handle_nfs3_readdir( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "readdir",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_readdirplus( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.handle )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "readdirplus",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_fsstat( rpc_call, rpc_reply ):
    root = state.get_path( rpc_call.root )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "fsstat",
               "path"    : str( root ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_fsinfo( rpc_call, rpc_reply ):
    root = state.get_path( rpc_call.root )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "fsinfo",
               "path"    : root,
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_pathconf( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.path )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "pathconf",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

def _handle_nfs3_commit( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.root )

    return ( info_level,
             { "proto"   : "nfs",
               "command" : "commit",
               "path"    : str( path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : str( rpc_reply.nfs_status ) } )

nfs3_handler_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL'        ] : _handle_nfs3_null,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR'     ] : _handle_nfs3_getattr,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR'     ] : _handle_nfs3_setattr,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP'      ] : _handle_nfs3_lookup,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS'      ] : _handle_nfs3_access,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK'    ] : _handle_nfs3_readlink,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ'        ] : _handle_nfs3_read,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE'       ] : _handle_nfs3_write,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE'      ] : _handle_nfs3_create,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR'       ] : _handle_nfs3_mkdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK'     ] : _handle_nfs3_symlink,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD'       ] : _handle_nfs3_mknod,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE'      ] : _handle_nfs3_remove,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR'       ] : _handle_nfs3_rmdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME'      ] : _handle_nfs3_rename,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK'        ] : _handle_nfs3_link,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR'     ] : _handle_nfs3_readdir,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : _handle_nfs3_readdirplus,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT'      ] : _handle_nfs3_fsstat,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO'      ] : _handle_nfs3_fsinfo,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF'    ] : _handle_nfs3_pathconf,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT'      ] : _handle_nfs3_commit,
}


def _get_rpc_status( rpc_call, rpc_reply ):

    # Not all RPC headers are parsed
    status = None
    if rpc_call.prog == nfs_const.ProgramValMap['mountd']:
        try:
            v = rpc_reply.getfieldval( "status" )
            status = nfs_const.Mount3_ValStatMap[ v ]
        except AttributeError:
            pass

    if not status:
        status = rpc_reply.accept_state.v

    return status

def _handle_mount3_mnt( rpc_call, rpc_reply ):
    return ( info_level,
             { "proto"   : "mount",
               "command" : "mnt",
               "path"    : str( rpc_call.path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : _get_rpc_status( rpc_call, rpc_reply ) } )

def _handle_mount3_umnt( rpc_call, rpc_reply ):
    return ( info_level,
             { "proto"   : "mount",
               "command" : "umnt",
               "path"    : str( rpc_call.path ),
               "who"     : state.get_auth_str(rpc_call),
               "status"  : _get_rpc_status( rpc_call, rpc_reply ) } )

def _handle_mount3_umnt_all( rpc_call, rpc_reply ):
    return ( info_level,
             { "proto"   : "nfs",
               "command" : "umnt_all",
               "path"    : None,
               "who"     : state.get_auth_str(rpc_call),
               "status"  : _get_rpc_status( rpc_call, rpc_reply ) } )

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


_curr_mount3_handlers = mount3_handler_lookup
_curr_nfs3_handlers   = nfs3_handler_lookup

def _handler_core( pkt ):
    parse = True
    res = None

    rpc = pkt.getlayer( nfs.RPC_Header )
    if (not rpc) or (not rpc.rpc_call):
        parse = False

    #if not rpc.rpc_call: # CALL packets have None for call field
    #    logging.debug( "#{} [call XID {:x}]"
    #                   .format( state.pkt_ct, rpc.xid ) )
    #    parse = False

    if parse:
        rpc_call = rpc.rpc_call

        if rpc_call.prog == nfs_const.ProgramValMap['mountd']:
            # This is a mountd reply. Call its handler with the (call, reply) pair
            res = _curr_mount3_handlers[ rpc_call.proc ] ( rpc_call, rpc )
        elif rpc_call.prog == nfs_const.ProgramValMap['nfs']:
            # This is an NFS reply. Call its handler with the (call, reply) pair
            res = _curr_nfs3_handlers[ rpc_call.proc ] ( rpc_call, rpc )

        else:
            logging.debug( "Ignoring program {} call/reply with XID {:x}".
                           format( rpc_call.prog, rpc.xid ) )

    state.update_state( pkt )
    return res


def set_rpc_handlers( nfs=None, mount3=None ):
    """ Updates RPC message handler(s) used by _handler_core() """
    if nfs:
        _curr_nfs3_handlers = nfs
    if mount3:
        _curr_mount3_handlers = mount3


# Use only one of these, or write your own
def pkt_handler_null( pkt ):
    pass


def pkt_handler_raw_print( pkt ):
    state.update_state( pkt )
    logging.debug( "#{} - packet {}"
                   .format( state.pkt_ct, repr(pkt) ) )


def pkt_handler_standard( pkt ):
    """
    Most useful handler for parsing. If you want it to handle
    standalone (outside of sensor framework) parsing.
    """
    return _handler_core( pkt )
