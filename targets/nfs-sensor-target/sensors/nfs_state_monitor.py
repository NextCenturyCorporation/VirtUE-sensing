#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Tracks NFS state such as:
##  * Mount points
##  * Handle --> path mappings
##
## The easiest way to use this is to invoke update_state() on every
## packet, and let it take care of the rest. Since state might be
## destroyed, the function should be called only after the caller is
## done processing the packet.
##

import nfs_const
import nfs
import os
import logging

import pdb


# stats should be updated _after_ packet has been parsed, so start at 1
pkt_ct = 1

xid_auth_map = dict()

handle_name_map = dict()
name_handle_map = dict()
active_mount_map = dict()


def _create_handle_mapping( path, handle, rpc_call ):
    handle_name_map[ handle ] = path
    name_handle_map[ path ]   = handle
    logging.debug( "Handle {} <--> Path '{}', user {}"
                   .format( handle, path, get_auth_str(rpc_call) ) )

def _destroy_handle_mapping( path=None, handle=None ):
    if (path and handle) or (not path and not handle):
        raise RuntimeError( "Pass in exactly one of [handle, path]" )

    if not path:
        path = handle_name_map[ handle ]
    if not handle:
        handle = name_handle_map[ path ]

    logging.debug( "Invalidating handle {}".format( handle ) )
    logging.debug( "Invalidating path {}".format( path ) )

    del handle_name_map[ handle ]
    del name_handle_map[ path ]


def _create_mount_mapping( path, handle, rpc_call ):
    active_mount_map[ path ] = handle
    _create_handle_mapping( path, handle, rpc_call )


def _destroy_mount_mapping( path ):
    for k in [ p for p in name_handle_map.keys()
               if str(p).startswith( str(path) ) ]:
        _destroy_handle_mapping( path=k )
    del active_mount_map[ path ]


def get_path( handle ):
    return str( handle_name_map.get( handle, '{unknown}' ))


def get_handle( name ):
    return str( name_handle_map.get( handle, '{unknown}' ))


def get_auth_str( rpc_call ):
    if rpc_call.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ]:
        return str( rpc_call.auth_sys )
    if rpc_call.cred_flav == nfs_const.AuthValMap[ 'AUTH_NULL' ]:
        return "AUTH_NULL"
    else:
        return "AUTH_UNKNOWN"


def is_reply_parsable( rpc_reply, rpc_call ):
    if rpc_call.prog == nfs_const.ProgramValMap['mountd']:

        if rpc_reply.accept_state.v != nfs_const.RPC_ACCEPT_SUCCESS:
            return False
        # Not all MOUNT3 headers are parsed
        try:
            status = rpc_reply.getfieldval( "status" )
        except AttributeError:
            return False

        if status != nfs_const.MOUNT3_OK:
            # Operation failed, nothing more to do for state update
            return False
        return True

    elif rpc_call.prog == nfs_const.ProgramValMap['nfs']:
        # If the operation failed or the reply has no NFS status, then skip it
        try:
            if not rpc_reply.nfs_status.success():
                return False
        except AttributeError:
            return False

        return True

    # Unsupported program
    return False


def update_state( pkt ):
    global pkt_ct
    pkt_ct += 1

    rpc_reply = pkt.getlayer( nfs.RPC_Header )
    if (not rpc_reply) or (not rpc_reply.rpc_call):
        # Calls have None as their rpc_call fields
        return

    # pkt is an RPC reply
    rpc_call = rpc_reply.rpc_call

    if not is_reply_parsable( rpc_reply, rpc_call ):
        return

    # Update state for certain program / procedure combinations
    if rpc_call.prog == nfs_const.ProgramValMap['mountd']:

        if rpc_call.proc == nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_MNT' ]:
            _create_mount_mapping( rpc_call.path, rpc_reply.handle, rpc_call )

        elif rpc_call.proc == nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNT' ]:
            _destroy_mount_mapping( rpc_call.path )

        elif rpc_call.proc == nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNTALL' ]:
            for k in [ p for p in active_mount_map ]:
                _destroy_mount_mapping( k )

    elif rpc_call.prog == nfs_const.ProgramValMap['nfs']:

        if rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP' ]:
            new = os.path.join( get_path( rpc_call.args.handle ),
                                str(rpc_call.args.fname) )

            _create_handle_mapping( new, rpc_reply.handle, rpc_call )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE' ]:
            if rpc_reply.handle.handle_follows:
                new = os.path.join( get_path( rpc_call.args.handle ),
                                    str( rpc_call.args.fname ) )

                _create_handle_mapping( new, rpc_reply.handle.handle, rpc_call )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR' ]:
            if rpc_reply.handle.handle_follows:
                new = os.path.join( get_path( rpc_call.args.handle ),
                                    str( rpc_call.args.fname ) )
                _create_handle_mapping( new, rpc_reply.handle.handle, rpc_call )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK' ]:
            if rpc_reply.handle.handle_follows:
                new = os.path.join( state.get_path( rpc_call.args.handle ),
                                    str(rpc_call.args.fname) )
                _create_handle_mapping( new, rpc_reply.handle.handle, rpc_call )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR' ]:
            _destroy_handle_mapping( name=os.path.join( get_path( rpc_call.args.handle ),
                                                        rpc_call.args.name ) )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME' ]:
            from_name = os.path.join( get_path( rpc_call.ffrom.handle ),
                                      str(rpc_call.ffrom.fname) )
            to_name   = os.path.join( get_path( rpc_call.fto.handle ),
                                      str(rpc_call.fto.fname) )

            _destroy_handle_mapping( name=from_name )
            _create_handle_mapping( to_name, get_handle( from_name ), rpc_call )


        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK' ]:
            if rpc_reply.file.handle_follows:
                _create_handle_mapping( os.path.join( get_path( rpc_call.link.handle ),
                                                      rpc_call.link.fname ),
                                        rpc_reply.file.handle,
                                        rpc_call )

        elif rpc_call.proc == nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE' ]:
            _destroy_handle_mapping(
                path=os.path.join( get_path( rpc_call.args.handle ),
                                   rpc_call.args.name ) )

    else:
        return
