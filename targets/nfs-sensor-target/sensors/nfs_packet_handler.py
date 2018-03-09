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

import nfs_state_monitor as state

def _handle_nop( rpc_call, rpc_reply ):
    logging.debug( "#{} RPC program {} procedure {} not handled"
                   .format( state.pkt_ct, rpc_call.prog, rpc_call.proc ) )

def _handle_nfs3_null( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS null: ".format( state.pkt_ct ) )

def _handle_nfs3_getattr( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )
    logging.debug( "#{} NFS getattr: {} by {} status {}"
                   .format( state.pkt_ct, name, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_setattr( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )
    
    logging.debug( "#{} NFS setattr: {} to {} by {} status {}"
                   .format( state.pkt_ct, name, str(rpc_call.new_attr),
                            state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_lookup( rpc_call, rpc_reply ):
    parent = state.get_path( rpc_call.args.handle )
    name = str(rpc_call.args.fname)

    newpath = os.path.join( parent, name )

    #if rpc_reply.nfs_status.success():
    #    _create_handle_mapping( newpath, rpc_reply.handle, rpc_call )

    logging.debug( "#{} NFS lookup: {} by {} status {}"
                   .format( state.pkt_ct, newpath, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_access( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )
    check = rpc_call.check_access

    access = None
    if rpc_reply.nfs_status.success():
        access = rpc_reply.access
    
    logging.debug( "#{} NFS access {}: {} / {} by {}"
                   .format( state.pkt_ct, name, check, access, state.get_auth_str(rpc_call) )  )

def _handle_nfs3_readlink( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )

    target = None
    if rpc_reply.nfs_status.success():
        target = rpc_reply.path
    
    logging.debug( "#{} NFS readlink: {} --> {} by {} status {}"
                   .format( state.pkt_ct, name, target,
                            state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_read( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )
    logging.debug( "#{} NFS read: {} bytes {} ofs {} by {} status {}"
                   .format( state.pkt_ct, name, rpc_call.count, rpc_call.offset,
                            state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )


def _handle_nfs3_write( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )
    logging.debug( "#{} NFS write: {} bytes {} ofs {} by {} status {}"
                   .format( state.pkt_ct, name, rpc_call.count, rpc_call.offset,
                            state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_create( rpc_call, rpc_reply ):
    parent_name = state.get_path( rpc_call.args.handle )
    new_path = os.path.join( parent_name, str( rpc_call.args.fname ) )

    logging.debug( "#{} NFS create: {} by {} status {} "
                   .format( state.pkt_ct, new_path, state.get_auth_str(rpc_call), rpc_reply.nfs_status )  )

    #if rpc_reply.nfs_status.success():
    #    hnew = None
    #    if rpc_reply.handle.handle_follows:
    #        hnew = rpc_reply.handle.handle
    #        _create_handle_mapping( new_path, hnew, rpc_call )

def _handle_nfs3_mkdir( rpc_call, rpc_reply ):
    parent_name = state.get_path( rpc_call.args.handle )
    new_path = os.path.join( parent_name, str( rpc_call.args.fname ) )

    #if rpc_reply.nfs_status.success():
    #    if rpc_reply.handle.handle_follows:
    #        h = rpc_reply.handle.handle
    #        _create_handle_mapping( new_path, h, rpc_call )
    #        logging.debug( "#{} NFS mkdir: {} --> {}"
    #                       .format( state.pkt_ct, new_path, h ) )
    #else:
    logging.debug( "#{} NFS mkdir: {} status {}"
                   .format( state.pkt_ct, new_path, rpc_reply.nfs_status ) )

def _handle_nfs3_symlink( rpc_call, rpc_reply ):
    parent = state.state.get_path( rpc_call.args.handle )
    new = os.path.join( parent, str(rpc_call.args.fname) )

    #handle = None
    #if rpc_reply.nfs_status.success():
    #    if rpc_reply.handle.handle_follows:
    #        handle = rpc_reply.handle.handle
    #        _create_handle_mapping( new, handle, rpc_call )

    logging.debug( "#{} NFS symlink: {} --> {} by {} status {}"
                   .format( state.pkt_ct, new, handle,
                            state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )
        
def _handle_nfs3_mknod( rpc_call, rpc_reply ):
    logging.debug( "#{} NFS mknod by {} status {}"
                   .format( state.pkt_ct, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_remove( rpc_call, rpc_reply ):
    name = os.path.join( state.get_path( rpc_call.args.handle ), rpc_call.args.name )
    logging.debug( "#{} NFS remove: {} by {} status {}"
                   .format( state.pkt_ct, name, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_rmdir( rpc_call, rpc_reply ):
    name = os.path.join( state.get_path( rpc_call.args.handle ), rpc_call.args.name )
    logging.debug( "#{} NFS rmdir: {} by {} status {}"
                   .format( state.pkt_ct, name, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_rename( rpc_call, rpc_reply ):
    from_name = os.path.join( state.get_path( rpc_call.ffrom.handle ),
                              str(rpc_call.ffrom.fname) )
    to_name   = os.path.join( state.get_path( rpc_call.fto.handle ),
                              str(rpc_call.fto.fname) )

    logging.debug( "#{} NFS rename: {} -> {} by {}"
                   .format( state.pkt_ct, from_name, to_name, state.get_auth_str(rpc_call) ) )

def _handle_nfs3_link( rpc_call, rpc_reply ):
    source = state.get_path( rpc_call.handle )
    dest   = os.path.join( state.get_path( rpc_call.link.handle ), rpc_call.link.fname )

    logging.debug( "#{} NFS link: {} -> {} by {} status {}" 
                   .format( state.pkt_ct, source, dest, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )
    #if rpc_reply.nfs_status.status():
    #    if rpc_reply.file.handle_follows:
    #        _create_handle_mapping( dest, rpc_reply.file.handle, rpc_call )


def _handle_nfs3_readdir( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )

    logging.debug( "#{} NFS readdir: {} by {} status {}"
                       .format( state.pkt_ct, name, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_readdirplus( rpc_call, rpc_reply ):
    name = state.get_path( rpc_call.handle )

    logging.debug( "#{} NFS readdirplus: {} by {} status {}"
                   .format( state.pkt_ct, name, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_fsstat( rpc_call, rpc_reply ):
    root = state.get_path( rpc_call.root )
    logging.debug( "#{} NFS fsstat: {} by {} status {}"
                   .format( state.pkt_ct, root, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_fsinfo( rpc_call, rpc_reply ):
    root = state.get_path( rpc_call.root )

    if rpc_reply.nfs_status.success():
        logging.debug( "#{} NFS fsinfo: {} by {} status {}"
                       .format( state.pkt_ct, root, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )
    else:
        logging.debug( "#{} NFS fsinfo: {} by {} failed {}"
                       .format( state.pkt_ct, root, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_pathconf( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.path )

    if rpc_reply.nfs_status.success():
        logging.debug( "#{} NFS pathconf: {} by {}"
                       .format( state.pkt_ct, path, state.get_auth_str(rpc_call) ) )
    else:
        logging.debug( "#{} NFS pathconf: {} by {} failed {}"
                       .format( state.pkt_ct, path, state.get_auth_str(rpc_call), rpc_reply.nfs_status ) )

def _handle_nfs3_commit( rpc_call, rpc_reply ):
    path = state.get_path( rpc_call.root )

    logging.debug( "#{} NFS commit: {} by {} status {}"
                       .format( state.pkt_ct, path, state.get_auth_str(rpc_call),
                                rpc_reply.nfs_status ) )

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
    logging.debug( "#{} MOUNT mnt: {} --> {:}"
                   .format( state.pkt_ct, rpc_reply.handle, str(rpc_call.path) ) )

def _handle_mount3_umnt( rpc_call, rpc_reply ):
    logging.debug( "#{} MOUNT umnt: {}"
                   .format( state.pkt_ct, rpc_call.path ) )

def _handle_mount3_umnt_all( rpc_call, rpc_reply ):
    logging.debug( "#{} MOUNT umnt_all:".format( state.pkt_ct ) )
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
        

_curr_mount3_handlers = mount3_handler_lookup
_curr_nfs3_handlers   = nfs3_handler_lookup
    
def _handler_core( pkt ):
    parse = True

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
            _curr_mount3_handlers[ rpc_call.proc ] ( rpc_call, rpc )
            
        elif rpc_call.prog == nfs_const.ProgramValMap['nfs']:
            # This is an NFS reply. Call its handler with the (call, reply) pair
            _curr_nfs3_handlers[ rpc_call.proc ] ( rpc_call, rpc )
            
        else:
            logging.debug( "Ignoring program {} call/reply with XID {:x}".
                           format( rpc_call.prog, rpc.xid ) )

    state.update_state( pkt )


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


def pkt_handler_basic( pkt ):
    """
    Most useful handler for parsing. If you want it to handle
    standalone (outside of sensor framework) parsing.
    """
    _handler_core( pkt )


