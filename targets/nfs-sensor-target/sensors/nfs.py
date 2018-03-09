#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Scapy-based parser for NFS v3
## https://tools.ietf.org/html/rfc1813
##

## Requirements: grab scapy v2.4.0 rc4 via
##   git submodule init
##   git submodule update
##
## The scapy acquired via 'pip3 install scapy-python3' doesn't support
## features we need to parse
##   READDIR's  response, or
##   READDIRPLUS's response
##

import os
import sys
import logging
import struct

import pdb

import nfs_const

from scapy.all import *


if sys.version_info.major < 3 or sys.version_info.minor < 5:
    print( "NOTICE: This module is intented for Python 3.5+" )


##
## RPC replies are not self-contained - they don't have all the info
## needed to figure out their type. Thus, we track transaction IDs,
## which are matched between a self-contained call message and its
## associated reply. We can then piece together the reply.
##
## Map: transaction ID --> NFS reply class
## Tracks outstanding XIDs so replies can be parsed.
##

xid_reply_map = dict()


##
## This module is responsible for mapping RPC replies to their
## respective calls. RPC replies then have an additional field,
## 'rpc_call' (e.g. reply.rpc_call), which can be used to access the
## call packet. This capability is particularly useful to a consumer
## that reads replies and has to determine what they're in response
## to. These actions are all implemented in
## RPC_Header.post_dissection.
##
## This is particularly important when handling PORTMAP replies, since
## we bind ports to the RPC header parser immediately upon discovering a
## new RPC protocol usage.
##

##
## Maps XIDs to their respective request for lookup in replies
##
xid_call_map = dict()



class nfs_config:
    breakpoints = False

def dobreak():
    if nfs_config.breakpoints:
        pdb.set_trace()

##
## Most of our Packet classes must specify no padding, thus they
## follow this template to minimze code duplication. N.B. this is
## incompatible with guess_next_payload(). If you want to define that
## method, then inherit directly from Packet.
##

class PacketNoPad( Packet ):
    def extract_padding(self, p):
        return "", p


######################################################################
## RFC 1813, Basic Data Types
##
## Our convention: l = length, v = value, p = pad
######################################################################


##
## RFC 1813 Basic Data Types (page 15-16)
##

def _bin2str( b ):
    return str( ''.join( [ chr(x) for x in b ] ) )

def _bin2hex( b ):
    return ''.join( [ '%02x' % x for x in b ] )

# filename3
#  typedef string filename3<>;
class NFS3_fname( PacketNoPad ):
    name = "fname"
    fields_desc = [ FieldLenField(     "l",  0, length_of="v", fmt='I'  ),
                    StrLenField(       "v", "", length_from=lambda pkt: pkt.l ),
                    XStrFixedLenField( "p", "", length_from=lambda pkt:(-pkt.l % 4) ), ]
    def __str__(self):
        return _bin2str( self.v )
    def __repr(self):
        return str( self )
    def __hash__(self):
        return hash( ( self.l, str(self) ) )

# nfspath3
#  typedef string nfspath3<>;
class NFS3_path( PacketNoPad ):
    name = "path"
    fields_desc = [ FieldLenField(     "l",  0, length_of="v", fmt='I' ),
                    StrLenField(       "v", "", length_from=lambda pkt: pkt.l),
                    XStrFixedLenField( "p", "", length_from=lambda pkt:(-pkt.l % 4) ), ]

    def __str__(self):
        return _bin2str( self.v )
    def __repr(self):
        return str( self )
    def __hash__(self):
        return  hash( ( self.l, str(self) ) )

# fileid3
#  typedef uint64 fileid3;
class NFS3_fileid( PacketNoPad ):
    name = "fileid"
    fields_desc = [ XLongField( "v", 0 ), ]

# cookie3
#  typedef uint64 cookie3;
# This might be labeled as "Verifier" in Wireshark
class NFS3_cookie( PacketNoPad ):
    name = "cookie"
    fields_desc = [ XLongField( "v", 0 ), ]

# cookieverf3
#       typedef opaque cookieverf3[NFS3_COOKIEVERFSIZE];
class NFS3_cookieverf( PacketNoPad ):
    name = "cookieverf"
    fields_desc = [ XLongField( "v", 0 ), ]

# createverf3
#       typedef opaque createverf3[NFS3_CREATEVERFSIZE];
class NFS3_createverf( PacketNoPad ):
    name = "createverf"
    fields_desc = [ XLongField( "v", 0 ), ]

# writeverf3
#       typedef opaque writeverf3[NFS3_WRITEVERFSIZE];
class NFS3_writeverf( PacketNoPad ):
    name = "writeverf"
    fields_desc = [ XLongField( "v", 0 ), ]

# uid3
#       typedef uint32 uid3;
class NFS3_uid( PacketNoPad ):
    name = "uid"
    fields_desc = [ XIntField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)

# gid3
#       typedef uint32 gid3;
class NFS3_gid( PacketNoPad ):
    name = "gid"
    fields_desc = [ XIntField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)


# size3
#       typedef uint64 size3;
class NFS3_size( PacketNoPad ):
    name = "size"
    fields_desc = [ XLongField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)

# offset3
#       typedef uint64 offset3;
class NFS3_offset( PacketNoPad ):
    name = "offset"
    fields_desc = [ XLongField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)


# mode3
#       typedef uint32 mode3;
class NFS3_mode( PacketNoPad ):
    name = "mode"
    fields_desc = [ XIntField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)

# count3
#       typedef uint32 count3;
class NFS3_count( PacketNoPad ):
    name = "count"
    fields_desc = [ XIntField( "v", 0 ), ]
    def __str__(self):
        return str(self.v)

class NFS3_ftype( PacketNoPad ):
    name = "ftype"
    fields_desc = [ IntEnumField( "v", 0, nfs_const.Nfs3_ValFtypeMap ), ]


class RPC_verifier( PacketNoPad ):
    name = "NFS Verifier"
    fields_desc = [ IntField( "flavor",    0 ),
                    FieldLenField( "l", None, length_of="d", fmt="I" ),
                    StrLenField(   "d",   "", length_from=lambda pkt: pkt.l ), ]

#specdata3
#      struct specdata3 {
#           uint32     specdata1;
#           uint32     specdata2;
#      };
class NFS3_specdata( PacketNoPad ):
    name = "specdata"
    fields_desc = [ XIntField( "specdata1", 0 ),
                    XIntField( "specdata2", 0 ), ]


#nfstime3
#      struct nfstime3 {
#         uint32   seconds;
#         uint32   nseconds;
#      };
class NFS3_time( PacketNoPad ):
    name = "time"
    fields_desc = [ XIntField( "sec", 0 ),
                    XIntField( "nsec", 0 ), ]

class NFS3_fhandle( PacketNoPad ):
    name = "fhandle"
    fields_desc = [ FieldLenField( "l",   0, length_of="v", fmt='I' ),
                    StrLenField(   "v", b"", length_from=lambda pkt: pkt.l ), ]
    def __str__(self):
        return _bin2hex( self.v )
    def __repr__(self):
        return _bin2hex( self.v )
    def __hash__(self):
        return hash( ( self.l, str(self) ) )

#fattr3, page 22
#      struct fattr3 {
#         ftype3     type;
#         mode3      mode;
#         uint32     nlink;
#         uid3       uid;
#         gid3       gid;
#         size3      size;
#         size3      used;
#         specdata3  rdev;
#         uint64     fsid;
#         fileid3    fileid;
#         nfstime3   atime;
#         nfstime3   mtime;
#         nfstime3   ctime;
#      };
class NFS3_fattr( PacketNoPad ):
    name = "NFS fattr"
    fields_desc = [ PacketField(    "ftype", None, NFS3_ftype ),
                    PacketField(     "mode", None, NFS3_mode ),
                    PacketField(    "nlink", None, NFS3_count ),
                    PacketField(      "uid", None, NFS3_uid ),
                    PacketField(      "gid", None, NFS3_gid ),
                    PacketField(     "size", None, NFS3_size ),
                    PacketField(     "used", None, NFS3_size ),
                    PacketField( "specdata", None, NFS3_specdata ),
                    XLongField(      "fsid",    0 ),
                    PacketField(   "fileid", None, NFS3_fileid ),
                    PacketField(    "atime", None, NFS3_time ),
                    PacketField(    "mtime", None, NFS3_time ),
                    PacketField(    "ctime", None, NFS3_time ), ]

# Page 26
#      struct sattr3 {
#         set_mode3   mode;
#         set_uid3    uid;
#         set_gid3    gid;
#         set_size3   size;
#         set_atime   atime;
#         set_mtime   mtime;
#      };
class NFS3_sattr( PacketNoPad ):
    name = "NFS sattr"
    fields_desc = [ IntField( "set_mode", 0 ),
                    ConditionalField( PacketField( "mode",  None, NFS3_mode ),
                                      lambda pkt: pkt.set_mode != 0 ),
                    IntField( "set_uid", 0 ),
                    ConditionalField( PacketField( "uid",   None, NFS3_uid ),
                                      lambda pkt: pkt.set_uid != 0 ),
                    IntField( "set_gid", 0 ),
                    ConditionalField( PacketField( "gid",   None, NFS3_gid ),
                                      lambda pkt: pkt.set_gid != 0 ),
                    IntField( "set_size", 0 ),
                    ConditionalField( PacketField( "size",  None, NFS3_size ),
                                      lambda pkt: pkt.set_size != 0 ),
                    IntField( "set_atime", 0 ),
                    ConditionalField( PacketField( "atime", None, NFS3_time ),
                                      lambda pkt: pkt.set_atime != 0 ),
                    IntField( "set_mtime", 0 ),
                    ConditionalField( PacketField( "mtime", None, NFS3_time ),
                                      lambda pkt: pkt.set_mtime != 0 ), ]
    def __str__(self):
        s = ""

        if self.set_mode:
            s += "mode: {:x} ".format(self.mode.v)
        if self.set_uid:
            s += "uid: {} ".format(self.uid)
        if self.set_gid:
            s += "gid: {} ".format(self.gid)
        if self.set_size:
            s += "size: {} ".format(self.size)
        if self.set_atime:
            s += "atime: {} ".format(self.atime)
        if self.set_mtime:
            s += "mtime: {} ".format(self.mtime)
        return s
            
class NFS3_post_op_attr( PacketNoPad ):
    name = "NFS post_op_attr"
    fields_desc = [ IntField( "attribs_follow", 0 ),
                    ConditionalField( PacketField( "fattr", None, NFS3_fattr ),
                                      lambda pkt: pkt.attribs_follow != 0 ) ]

class NFS3_post_op_fh( PacketNoPad ):
    name = "post_op_fh"
    fields_desc = [ IntField( "handle_follows", 0 ),
                    ConditionalField( PacketField( "handle", None, NFS3_fhandle ),
                                      lambda pkt: pkt.handle_follows != 0 ) ]

class NFS3_wcc_attr( PacketNoPad ):
    name = "wcc_attr"
    fields_desc = [ PacketField( "size",     0, NFS3_size ),
                    PacketField( "mtime", None, NFS3_time ),
                    PacketField( "ctime", None, NFS3_time ), ]

class NFS3_pre_op_attr( PacketNoPad ):
    name = "pre_op_attr"
    fields_desc = [ IntField(    "attribs_follow",    0 ),
                    PacketField( "attr",           None, NFS3_wcc_attr ) ]

# Page 24

class NFS3_wcc_data( PacketNoPad ):
    name = "wcc_data"
    fields_desc = [ PacketField( "before", None, NFS3_pre_op_attr ),
                    PacketField(  "after", None, NFS3_post_op_attr ) ]


# Page 27
# struct diropargs3 {
#     nfs_fh3 dir;
#     filename3 name;
# };
class NFS3_dir_op_args( PacketNoPad ):
    name = "dir_op_args"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField(  "fname", None, NFS3_fname ), ]

# Page 31
class NFS3_sattrguard( PacketNoPad ):
    fields_desc = [ IntField( "check", 0 ),
                    ConditionalField( PacketField( "obj_ctime", None, NFS3_time ),
                                      lambda pkt:pkt.check != 0 ) ]

# Page 61
class NFS3_symlink_data( PacketNoPad ):
    name = "symlink_data"
    fields_desc = [ PacketField( "attr", None, NFS3_sattr ),
                    PacketField( "path", None, NFS3_path ), ]

class NFS3_status( PacketNoPad ):
    name = "nfs_status"
    fields_desc = [ IntEnumField( "v", None, nfs_const.Nfs3_ValStatMap ), ]

    def success( self ):
        return 0 == self.v

    def __str__( self ):
        return nfs_const.Nfs3_ValStatMap[ self.v ]

######################################################################
## NFS Procedures: These are the NFS functions built on top of
## RPC. They're enumerated in the standard starting on page 28. The
## RPC layer (RPC_Call below) tells the scapy parser which Call-side
## class to use based on the procedure number. That same class also
## maps the transaction ID to the type of response required, thus
## enabling correct parsing of responses.
######################################################################


######################################################################
## NFS Procedure 0: NULL
######################################################################

class NFS3_NULL_Call( Packet ):
    name = "NFS NULL call"
    fields_desc = []


class NFS3_NULL_Reply( Packet ):
    name = "NFS NULL response"
    fields_desc = []

    
######################################################################
## NFS Procedure 1: GETTATTR
######################################################################
class NFS3_GETATTR_Call( Packet ):
    name = "NFS GETATTR call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ), ]

class NFS3_GETATTR_ReplyOk( Packet ):
    name = "NFS GETATTR Reply OK"
    fields_desc = [ PacketField( "fattr", None, NFS3_fattr ), ]

class NFS3_GETATTR_Reply( Packet ):
    name = "NFS GETATTR reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]
    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_GETATTR_ReplyOk
        return Packet.guess_payload_class(self, payload)

######################################################################
## NFS Procedure 2: SETATTR
######################################################################
class NFS3_SETATTR_Call( Packet ):
    name = "NFS SETATTR call"
    fields_desc = [ PacketField(   "handle", None, NFS3_fhandle ),
                    PacketField( "new_attr", None, NFS3_sattr ),
                    PacketField(    "guard", None, NFS3_sattrguard ), ]

    
class NFS3_SETATTR_Reply( Packet ):
    name = "NFS SETATTR reply"
    fields_desc = [ IntEnumField( "nfs_status",    0, nfs_const.Nfs3_ValStatMap ),
                    PacketField(     "obj_wcc", None, NFS3_wcc_data ), ]


######################################################################
## NFS Procedure 3: LOOKUP
######################################################################
class NFS3_LOOKUP_Call( Packet ):
    name = "NFS LOOKUP call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ) ]

class NFS3_LOOKUP_ReplyOk( PacketNoPad ):
    name = "NFS LOOKUP reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField(    "obj", None, NFS3_post_op_attr ),
                    PacketField(    "dir", None, NFS3_post_op_attr ), ]

class NFS3_LOOKUP_ReplyFail( Packet ):
    name = "NFS LOOKUP reply Fail"
    fields_desc = [ PacketField( "dir", None, NFS3_post_op_attr ), ]

class NFS3_LOOKUP_Reply( Packet ):
    name = "NFS LOOKUP reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_LOOKUP_ReplyOk
        else:
            return NFS3_LOOKUP_ReplyFail


######################################################################
## NFS Procedure 4: ACCESS
######################################################################
class NFS3_ACCESS_Call( PacketNoPad ):
    name = "NFS ACCESS call"
    fields_desc = [ PacketField(     "handle", None, NFS3_fhandle ),
                    XIntField( "check_access",    0 ), ]

class NFS3_ACCESS_ReplyOk( Packet ):
    name = "NFS ACCESS reply OK"
    fields_desc = [ PacketField(  "obj", None,    NFS3_post_op_attr ),
                    XIntField( "access",    0 ),                     ]
    
class NFS3_ACCESS_ReplyFail( Packet ):
    name = "NFS ACCESS reply fail"
    fields_desc = [ PacketField( "obj", None, NFS3_post_op_attr ) ]
    
class NFS3_ACCESS_Reply( Packet ):
    name = "NFS ACCESS reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]
    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_ACCESS_ReplyOk
        else:
            return NFS3_ACCESS_ReplyFail

######################################################################
## NFS Procedure 5: READLINK
######################################################################
class NFS3_READLINK_Call( Packet ):
    name = "NFS READLINK call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ), ]
    
class NFS3_READLINK_ReplyOk( Packet ):
    name = "NFS READLINK reply OK"
    fields_desc = [ PacketField( "link", None, NFS3_post_op_attr ),
                    PacketField( "path", None, NFS3_path ), ]

class NFS3_READLINK_ReplyFail( Packet ):
    name = "NFS READLINK reply Fail"
    fields_desc = [ PacketField( "link", None, NFS3_post_op_attr ), ]

class NFS3_READLINK_Reply( Packet ):
    name = "NFS READLINK reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_READLINK_ReplyOk
        else:
            return NFS3_READLINK_ReplyFail

######################################################################
## NFS Procedure 6 (section 3.3.6): READ
######################################################################

# N.B. see note on WRITE

class NFS3_READ_Call( Packet ):
    name = "NFS READ call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "offset", None, NFS3_offset ),
                    PacketField( "count",  None, NFS3_count ), ]

    
class NFS3_READ_ReplyOk( PacketNoPad ):
    name = "NFS READ reply OK"
    fields_desc = [ PacketField(    "file", None, NFS3_post_op_attr ),
                    FieldLenField( "count", 0, fmt="I", length_of="data" ),
                    IntField(        "eof", 0 ),
                    IntField(   "data_len", 0 ),
                    StrLenField(    "data", 0, length_from=lambda pkt: pkt.count ),
                    PadField( StrLenField( "p", "", length_from=lambda pkt: pkt.count),
                              4, padwith=b'\x00' ), ]

class NFS3_READ_ReplyFail( Packet ):
    name = "NFS READ reply Fail"
    fields_desc = [ PacketField( "file", None, NFS3_post_op_attr ), ]
    
class NFS3_READ_Reply( Packet ):
    name = "NFS READ reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_READ_ReplyOk
        else:
            return NFS3_READ_ReplyFail


######################################################################
## NFS Procedure 7 (section 3.3.7): WRITE
######################################################################

# N.B. In case of a large write that can't fit in one packet, the
# operation will be split across multiple TCP segments and the
# cumulative size reported in the WRITE call. Since we're not
# performing reassembly, we'll be given fewer bytes than reported by
# the WRITE call header.
#
# Moreover, a large write can be split into multiple WRITEs which are
# then split across several TCP packets. WRITEs after the first one
# might contain, in this order:
# 1. TCP header
# 2. The final bytes from the previous WRITE
# 3. The RPC and NFS headers for the new WRITE
#
# This means that we need to track data offsets in large writes to
# find subsequent WRITEs. As currently implemented, we look for an RPC
# header immediately after the TCP header and will miss the RPC
# header. We attempt to parse the data bytes as an RPC header but
# *should* fail out.
#
# The bottom line is that we will catch the first WRITE, so we know
# who attempted to write to which file. However, we miss the full
# contents.
#
# The same likely applies to READs.

class NFS3_WRITE_Call( Packet ):
    name = "NFS WRITE call"

    # .data consumes remainder of packet
    fields_desc = [ PacketField(  "handle", None, NFS3_fhandle ),
                    PacketField(  "offset", None, NFS3_offset ),
                    PacketField(   "count", None, NFS3_count ),
                    IntEnumField( "stable",    0, nfs_const.Nfs3_ValStableMap ),
                    IntField(   "data_len",    0 ),
                    StrField(       "data",    ""), ]
    
class NFS3_WRITE_ReplyOk( Packet ):
    name = "NFS WRITE reply OK"
    fields_desc = [ PacketField(        "wcc", None, NFS3_wcc_data ),
                    XIntField(        "count",    0 ),
                    IntEnumField( "committed",    0, nfs_const.Nfs3_ValStableMap ),
                    PacketField(  "writeverf", None, NFS3_writeverf ) ]
    
class NFS3_WRITE_ReplyFail( Packet ):
    name = "NFS WRITE reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_WRITE_Reply( Packet ):
    name = "NFS WRITE reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_WRITE_ReplyOk
        else:
            return NFS3_WRITE_ReplyFail

######################################################################
## NFS Procedure 8 (section 3.3.8): CREATE
######################################################################
class NFS3_CREATE_Call( Packet ):
    name = "NFS CREATE call"
    fields_desc = [ PacketField(  "args",  None, NFS3_dir_op_args ),
                    IntEnumField( "mode", 0, nfs_const.Nfs3_ValCreateModeMap ),
                    ConditionalField( PacketField( "attr", None, NFS3_sattr ),
                                      lambda pkt: pkt.mode in (
                                          nfs_const.Nfs3_CreateModeValMap['UNCHECKED'],
                                          nfs_const.Nfs3_CreateModeValMap['GUARDED'] ) ),
                    ConditionalField( PacketField( "how", None, NFS3_createverf ),
                                      lambda pkt: pkt.mode in (
                                          nfs_const.Nfs3_CreateModeValMap['EXCLUSIVE'], ) ), ]
    
class NFS3_CREATE_ReplyOk( Packet ):
    name = "NFS CREATE reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_post_op_fh ),
                    PacketField(   "attr", None, NFS3_post_op_attr ),
                    PacketField(    "wcc", None, NFS3_wcc_data ),    ]
    
class NFS3_CREATE_ReplyFail( Packet ):
    name = "NFS CREATE reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]

class NFS3_CREATE_Reply( Packet ):
    name = "NFS CREATE reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_CREATE_ReplyOk
        else:
            return NFS3_CREATE_ReplyFail

######################################################################
## NFS Procedure 9 (section 3.3.9): MKDIR
######################################################################
class NFS3_MKDIR_Call( Packet ):
    name = "NFS MKDIR call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ),
                    PacketField( "attr", None, NFS3_sattr ), ]

class NFS3_MKDIR_ReplyOk( Packet ):
    name = "NFS MKDIR reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_post_op_fh ),
                    PacketField(   "attr", None, NFS3_post_op_attr ),
                    PacketField(    "wcc", None, NFS3_wcc_data ),    ]

class NFS3_MKDIR_ReplyFail( Packet ):
    name = "NFS MKDIR reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_MKDIR_Reply( Packet ):
    name = "NFS MKDIR reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_MKDIR_ReplyOk
        else:
            return NFS3_MKDIR_ReplyFail

######################################################################
## NFS Procedure 10 (section 3.3.10): SYMLINK
######################################################################
class NFS3_SYMLINK_Call( Packet ):
    name = "NFS SYMLINK call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ),
                    PacketField( "link", None, NFS3_symlink_data ), ]

class NFS3_SYMLINK_ReplyOk( Packet ):
    name = "NFS SYMLINK reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_post_op_fh ),
                    PacketField(   "attr", None, NFS3_post_op_attr ),
                    PacketField(    "wcc", None, NFS3_wcc_data ),    ]

class NFS3_SYMLINK_ReplyFail( Packet ):
    name = "NFS SYMLINK reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_SYMLINK_Reply( Packet ):
    name = "NFS SYMLINK reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_SYMLINK_ReplyOk
        else:
            return NFS3_SYMLINK_ReplyFail

######################################################################
## NFS Procedure 11 (section 3.3.11): MKNOD *** NOT IMPLEMENTED FOR NOW ***
######################################################################
class NFS3_MKNOD_Call( Packet ):
    name = "NFS MKNOD call"
    fields_desc = []

class NFS3_MKNOD_Reply( Packet ):
    name = "NFS MKNOD reply"
    fields_desc = [ ]


######################################################################
## NFS Procedure 12 (section 3.3.12): REMOVE
######################################################################
class NFS3_REMOVE_Call( Packet ):
    name = "NFS REMOVE call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ), ]

class NFS3_REMOVE_Reply( Packet ):
    name = "NFS REMOVE reply"
    fields_desc = [ IntEnumField( "nfs_status",    0, nfs_const.Nfs3_ValStatMap ),
                    PacketField(         "wcc", None, NFS3_wcc_data ),    ]

######################################################################
## NFS Procedure 13 (section 3.3.13): RMDIR
######################################################################
class NFS3_RMDIR_Call( Packet ):
    name = "NFS RMDIR call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ), ]

class NFS3_RMDIR_Reply( Packet ):
    name = "NFS RMDIR reply"
    fields_desc = [ IntEnumField( "nfs_status",    0, nfs_const.Nfs3_ValStatMap ),
                    PacketField(         "wcc", None, NFS3_wcc_data ),    ]

######################################################################
## NFS Procedure 14 (section 3.3.14): RENAME
######################################################################
class NFS3_RENAME_Call( Packet ):
    name = "NFS RENAME call"
    fields_desc = [ PacketField( "ffrom", None, NFS3_dir_op_args ),
                    PacketField(   "fto", None, NFS3_dir_op_args ), ]

class NFS3_RENAME_Reply( Packet ):
    name = "NFS RENAME reply"
    fields_desc = [ IntEnumField( "nfs_status",    0, nfs_const.Nfs3_ValStatMap ),
                    PacketField(       "ffrom", None, NFS3_wcc_data ),
                    PacketField(         "fto", None, NFS3_wcc_data ), ]

    
######################################################################
## NFS Procedure 15 (section 3.3.15): LINK
######################################################################
class NFS3_LINK_Call( Packet ):
    name = "NFS LINK call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField(  "link" , None, NFS3_dir_op_args ), ]

class NFS3_LINK_Reply( Packet ):
    name = "NFS LINK reply"
    fields_desc = [ PacketField(    "file", None, NFS3_post_op_fh ),
                    PacketField( "linkdir", None, NFS3_wcc_data ), ]

######################################################################
## NFS Procedure 16 (section 3.3.16) READDIR
######################################################################
class NFS3_READDIR_Call( Packet ):
    name = "NFS READDIR call"
    fields_desc = [ PacketField(     "handle", None, NFS3_fhandle ),
                    PacketField(     "cookie", None, NFS3_cookie ),
                    PacketField( "cookieverf", None, NFS3_cookieverf ),
                    PacketField(     "count" , None, NFS3_count ), ]

class NFS3_READDIR_entry( Packet ):
    name = "NFS READDIR entry"
    fields_desc = [ PacketField(        "fileid", None, NFS3_fileid ),
                    PacketField(         "fname", None, NFS3_fname ),
                    PacketField(        "cookie", None, NFS3_cookie ),
                    IntField(    "entry_follows", False ) ]
    
    @staticmethod
    def next_entry( pkt, lst, curr, remain ):
        if (not curr) or curr.entry_follows:
            # We got here, so NFS3_READDIR_ReplyOk.entry_follows == True
            return NFS3_READDIR_entry
        else:
            return None

class NFS3_READDIR_ReplyOk( Packet ):
    name = "NFS READDIR reply ok"
    fields_desc = [ PacketField(        "dir", None, NFS3_post_op_attr ),
                    PacketField( "cookieverf", None, NFS3_cookieverf ),
                    # READDIR3resok.reply.entries (next 2 fields)
                    IntField( "entry_follows", False ),
                    ConditionalField( PacketListField( "entries", None, NFS3_READDIR_entry,
                                                       next_cls_cb=NFS3_READDIR_entry.next_entry ),
                                      lambda pkt: pkt.entry_follows ),
                    IntField(           "eof",  0 ) ]

class NFS3_READDIR_ReplyFail( Packet ):
    name = "NFS READDIR reply fail"
    fields_desc = [ PacketField( "dir", None, NFS3_post_op_attr ), ]

class NFS3_READDIR_Reply( Packet ):
    name = "NFS READDIR reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_READDIR_ReplyOk
        else:
            return NFS3_READDIR_ReplyFail


######################################################################
## NFS Procedure 17 (section 3.3.17): READDIRPLUS
######################################################################
        
class NFS3_READDIRPLUS_Call( Packet ):
    name = "NFS READDIRPLUS call"
    fields_desc = [ PacketField(     "handle", None, NFS3_fhandle ),
                    PacketField(     "cookie", None, NFS3_cookie ),
                    PacketField( "cookieverf", None, NFS3_cookieverf ),
                    PacketField(   "dircount", None, NFS3_count ),
                    PacketField(   "maxcount", None, NFS3_count ), ]

# This is how the packet looks in Wireshark; the RFC isn't clear on
# this. Note how entry_follows is both in this Packet and in
# NFS3_READDIRPLUS_ReplyOk
class NFS3_READDIRPLUS_entry( PacketNoPad ):
    name = "NFS READDIRPLUS entry"
    fields_desc = [ PacketField(     "fileid", None, NFS3_fileid ),
                    PacketField(      "fname", None, NFS3_fname ),
                    PacketField(     "cookie", None, NFS3_cookie ),
                    PacketField(  "name_attr", None, NFS3_post_op_attr ),
                    PacketField(       "file", None, NFS3_post_op_fh ),
                    # struct entryplus3 { ... entryplus3 * nextentry }
                    IntField( "entry_follows", False ), ]
        
    @staticmethod
    def next_entry( pkt, lst, curr, remain ):
        if (not curr) or curr.entry_follows:
            # We got here, so NFS3_READDIRPLUS_ReplyOk.entry_follows == True
            return NFS3_READDIRPLUS_entry
        else:
            return None

class NFS3_READDIRPLUS_ReplyOk( PacketNoPad ):
    name = "NFS READDIRPLUS reply ok"
    fields_desc = [ PacketField(        "dir", None, NFS3_post_op_attr ),
                    PacketField( "cookieverf", None, NFS3_cookieverf ),

                    # READDIRPLUS3resok.reply.entries (next 2 fields)
                    IntField( "entry_follows", False ),
                    ConditionalField( PacketListField( "entries", None, NFS3_READDIRPLUS_entry,
                                                       next_cls_cb=NFS3_READDIRPLUS_entry.next_entry ),
                                      lambda pkt: pkt.entry_follows ),
                    # READDIRPLUS3resok.reply.eof
                    IntField(           "eof",  0 ) ]

class NFS3_READDIRPLUS_ReplyFail( Packet ):
    name = "NFS READDIRPLUS reply fail"
    fields_desc = [ PacketField( "dir", None, NFS3_post_op_attr ), ]

class NFS3_READDIRPLUS_Reply( Packet ):
    name = "NFS READDIRPLUS reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_READDIRPLUS_ReplyOk
        else:
            return NFS3_READDIRPLUS_ReplyFail

######################################################################
## NFS Procedure 18 (section 3.3.18): FSSTAT
######################################################################

class NFS3_FSSTAT_Call( Packet ):
    name = "NFS FSSTAT call"
    fields_desc = [ PacketField( "root",   None, NFS3_fhandle ), ]

class NFS3_FSSTAT_ReplyOk( Packet ):
    name = "NFS FSSTAT reply OK"
    fields_desc = [ PacketField(    "dir", None, NFS3_post_op_attr ),

                    PacketField( "tbytes", None, NFS3_size ),
                    PacketField( "fbytes", None, NFS3_size ),
                    PacketField( "abytes", None, NFS3_size ),
                    
                    PacketField( "tfiles", None, NFS3_size ),
                    PacketField( "ffiles", None, NFS3_size ),
                    PacketField( "afiles", None, NFS3_size ),

                    IntField(  "invarsec",    0 ), ]
    

class NFS3_FSSTAT_ReplyFail( Packet ):
    name = "NFS FSSTAT reply fail"
    fields_desc = [ PacketField( "fs"  , None, NFS3_post_op_attr ), ]
    
class NFS3_FSSTAT_Reply( Packet ):
    name = "NFS FSSTAT reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_FSSTAT_ReplyOk
        else:
            return NFS3_FSSTAT_ReplyFail


######################################################################
## NFS Procedure 19 (section 3.3.19): FSINFO
######################################################################

class NFS3_FSINFO_Call( Packet ):
    name = "NFS FSINFO call"
    fields_desc = [ PacketField( "root", None, NFS3_fhandle ), ]
    
class NFS3_FSINFO_ReplyFail( Packet ):
    name = "NFS FSINFO reply fail"
    fields_desc = [ PacketField( "fs"  , None, NFS3_post_op_attr ), ]
        
class NFS3_FSINFO_ReplyOk( PacketNoPad ):
    name = "NFS FSINFO reply OK"
    fields_desc = [ PacketField(          "fs", None, NFS3_post_op_attr ),
                    XIntField(         "rtmax",    0 ),
                    XIntField(        "rtpref",    0 ),
                    XIntField(        "rtmult",    0 ),

                    XIntField(         "wtmax",    0 ),
                    XIntField(        "wtpref",    0 ),
                    XIntField(        "wtmult",    0 ),

                    XIntField(        "dtmult",    0 ),
                    
                    PacketField( "maxfilesize", None, NFS3_size ),
                    PacketField(  "time_delta", None, NFS3_time ),
                    XIntField(    "properties",    0 ), ] # nfs_const.Nfs3_ValFsinfoMap

class NFS3_FSINFO_Reply( Packet ):
    name = "NFS FSINFO reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_FSINFO_ReplyOk
        else:
            return NFS3_FSINFO_ReplyFail

######################################################################
## NFS Procedure 20 (section 3.3.20): PATHCONF
######################################################################

class NFS3_PATHCONF_Call( Packet ):
    name = "NFS PATHCONF call"
    fields_desc = [ PacketField( "path", None, NFS3_fhandle ), ]
    
class NFS3_PATHCONF_ReplyOk( Packet ):
    name = "NFS PATHCONF reply OK"
    fields_desc = [ PacketField(            "dir", None, NFS3_post_op_attr ),
                    XIntField(          "linkmax",    0 ),
                    XIntField(         "name_max",    0 ),
                    XIntField(          "notrunc",    0 ),
                    XIntField( "chown_restricted",    0 ),
                    XIntField( "case_insensitive",    0 ),
                    XIntField(  "case_preserving",    0 ), ]
    
class NFS3_PATHCONF_ReplyFail( Packet ):
    name = "NFS PATHCONF reply fail"
    fields_desc = [ PacketField( "pathconf"  , None, NFS3_post_op_attr ), ]
    
class NFS3_PATHCONF_Reply( Packet ):
    name = "NFS PATHCONF reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_PATHCONF_ReplyOk
        else:
            return NFS3_PATHCONF_ReplyFail


######################################################################
## NFS Procedure 21 (section 3.3.21): COMMIT
######################################################################
class NFS3_COMMIT_Call( Packet ):
    name = "NFS COMMIT call"
    fields_desc = [ PacketField(   "root", None, NFS3_fhandle ),
                    PacketField( "offset", None, NFS3_offset ),
                    PacketField(  "count", None, NFS3_count ), ]

class NFS3_COMMIT_ReplyOk( Packet ):
    name = "NFS COMMIT reply OK"
    fields_desc = [ PacketField(       "wcc", None, NFS3_wcc_data ),
                    PacketField( "writeverf", None, NFS3_writeverf ) ]

class NFS3_COMMIT_ReplyFail( Packet ):
    name = "NFS COMMIT reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]

class NFS3_COMMIT_Reply( Packet ):
    name = "NFS COMMIT reply"
    fields_desc = [ PacketField( "nfs_status", 0, NFS3_status ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status.success():
            return NFS3_COMMIT_ReplyOk
        else:
            return NFS3_COMMIT_ReplyFail


## Lookup table: Procedure value --> Call handler
nfs3_call_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL'        ] : NFS3_NULL_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR'     ] : NFS3_GETATTR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR'     ] : NFS3_SETATTR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP'      ] : NFS3_LOOKUP_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS'      ] : NFS3_ACCESS_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK'    ] : NFS3_READLINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ'        ] : NFS3_READ_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE'       ] : NFS3_WRITE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE'      ] : NFS3_CREATE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR'       ] : NFS3_MKDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK'     ] : NFS3_SYMLINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD'       ] : NFS3_MKNOD_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE'      ] : NFS3_REMOVE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR'       ] : NFS3_RMDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME'      ] : NFS3_RENAME_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK'        ] : NFS3_LINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR'     ] : NFS3_READDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : NFS3_READDIRPLUS_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT'      ] : NFS3_FSSTAT_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO'      ] : NFS3_FSINFO_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF'    ] : NFS3_PATHCONF_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT'      ] : NFS3_COMMIT_Call,
}
 
## Lookup table: Procedure value --> Response handler
nfs3_reply_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL'        ] : NFS3_NULL_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR'     ] : NFS3_GETATTR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR'     ] : NFS3_SETATTR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP'      ] : NFS3_LOOKUP_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS'      ] : NFS3_ACCESS_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK'    ] : NFS3_READLINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ'        ] : NFS3_READ_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE'       ] : NFS3_WRITE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE'      ] : NFS3_CREATE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR'       ] : NFS3_MKDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK'     ] : NFS3_SYMLINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD'       ] : NFS3_MKNOD_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE'      ] : NFS3_REMOVE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR'       ] : NFS3_RMDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME'      ] : NFS3_RENAME_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK'        ] : NFS3_LINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR'     ] : NFS3_READDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : NFS3_READDIRPLUS_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT'      ] : NFS3_FSSTAT_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO'      ] : NFS3_FSINFO_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF'    ] : NFS3_PATHCONF_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT'      ] : NFS3_COMMIT_Reply,
} 


######################################################################
## MOUNT3 procedures, also defined in RFC 1813.
######################################################################

######################################################################
## MOUNT3 Procedure 0 (section 5.2.0) NULL
######################################################################
class MOUNT3_NULL_Call( Packet ):
    name = "MOUNT3 NULL call"
    fields_desc = []

class MOUNT3_NULL_Reply( Packet ):
    name = "MOUNT3 NULL reply"
    fields_desc = []

######################################################################
## MOUNT3 Procedure 1 (section 5.2.1) MNT
######################################################################
class MOUNT3_MNT_Call( PacketNoPad ):
    name = "MOUNT3 MNT call"
    fields_desc = [ PacketField( "path", None, NFS3_path ), ]

class MOUNT3_MNT_ReplyOk( Packet ):
    name = "NFS MNT reply OK"
    fields_desc = [ PacketField(           "handle", None, NFS3_fhandle ),
                    FieldLenField( "auth_flavor_ct",    0,    count_of="auth_flavors", fmt='I' ),
                    FieldListField(  "auth_flavors",    0,
                                    IntEnumField( "flavor", 0, nfs_const.ValAuthMap ),
                                    count_from=lambda pkt: pkt.auth_flavor_ct ) ]
class MOUNT3_MNT_Reply( Packet ):
    name = "NFS MNT reply"
    fields_desc = [ IntField( "status",    0 ), ]

    def guess_payload_class( self, payload ):
        if self.status == nfs_const.MOUNT3_OK:
            return MOUNT3_MNT_ReplyOk

######################################################################
## MOUNT3 Procedure 3 (section 5.2.1) UMNT
######################################################################
class MOUNT3_UMNT_Call( PacketNoPad ):
    name = "MOUNT3 UMNT call"
    fields_desc = [ PacketField( "path", None, NFS3_path ), ]

class MOUNT3_UMNT_Reply( Packet ):
    name = "NFS UMNT reply"
    fields_desc = []

######################################################################
## MOUNT3 Procedure 4 (section 5.2.1) UMNTALL
######################################################################
class MOUNT3_UMNTALL_Call( PacketNoPad ):
    name = "MOUNT3 UMNTALL call"
    fields_desc = []

class MOUNT3_UMNTALL_Reply( Packet ):
    name = "NFS UMNTALL reply"
    fields_desc = []

######################################################################
## MOUNT3 Procedure 5 (section 5.2.5) EXPORT
######################################################################
class MOUNT3_EXPORT_Call( PacketNoPad ):
    name = "MOUNT3 EXPORT call"
    fields_desc = []

class MOUNT3_EXPORT_Reply( Packet ):
    name = "NFS EXPORT reply"
    fields_desc = []

    
# Not all procedures are implemented by us
mount3_call_lookup = {
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_NULL'    ] : MOUNT3_NULL_Call,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_MNT'     ] : MOUNT3_MNT_Call,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_DUMP'    ] : None,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNT'    ] : MOUNT3_UMNT_Call,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNTALL' ] : MOUNT3_UMNTALL_Call,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_EXPORT'  ] : MOUNT3_EXPORT_Call,
}

mount3_reply_lookup = {
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_NULL'    ] : MOUNT3_NULL_Reply,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_MNT'     ] : MOUNT3_MNT_Reply,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_DUMP'    ] : None,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNT'    ] : MOUNT3_UMNT_Reply,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_UMNTALL' ] : MOUNT3_UMNTALL_Reply,
    nfs_const.Mount3_ProcedureValMap[ 'MOUNTPROC3_EXPORT'  ] : MOUNT3_EXPORT_Reply,
}



######################################################################
## PORTMAP procedures, defined in RFC 1833
######################################################################

######################################################################
## PORTMAP Procedure 0 (section 3.1, 3.2) NULL
######################################################################
class PORTMAP_NULL_Call( Packet ):
    name = "PORTMAP NULL call"
    fields_desc = []

class PORTMAP_NULL_Reply( Packet ):
    name = "PORTMAP NULL reply"
    fields_desc = []
    
######################################################################
## PORTMAP Procedure 3 (section 3.1, 3.2) GETPORT
######################################################################


class PORTMAP_GETPORT_Call( Packet ):
    name = "PORTMAP GETPORT call"
    fields_desc = [ IntEnumField( "program", 0, nfs_const.ValProgramMap ),
                    IntField(     "version", 0 ),
                    IntField(       "proto", 0 ),
                    IntField(        "port", 0 ) ]


class PORTMAP_GETPORT_Reply( Packet ):
    name = "PORTMAP GETPORT reply"
    fields_desc = [ IntField( "port", 0 ) ]
    
    def post_dissection( self, pkt ):
        """ Bind newly-discovered port to RPC """
        request = self.underlayer.underlayer.rpc_call

        logging.info( "RPC program '{}' expected on port {}"
                      .format( nfs_const.ValProgramMap[ request.program ],
                               self.port ) )
        bind_port_to_rpc( self.port,
                          bind_udp=(request.proto == nfs_const.ProtoValMap['UDP']),
                          bind_tcp=(request.proto == nfs_const.ProtoValMap['TCP']) )

# Not all procedures are implemented by us
portmap_call_lookup = {
    nfs_const.PortMapperValMap[ 'PMAPPROC_NULL'    ] : PORTMAP_NULL_Call,
    nfs_const.PortMapperValMap[ 'PMAPPROC_SET'     ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_UNSET'   ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_GETPORT' ] : PORTMAP_GETPORT_Call,
    nfs_const.PortMapperValMap[ 'PMAPPROC_DUMP'    ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_CALLIT'  ] : None,
}

portmap_reply_lookup = {
    nfs_const.PortMapperValMap[ 'PMAPPROC_NULL'    ] : PORTMAP_NULL_Reply,
    nfs_const.PortMapperValMap[ 'PMAPPROC_SET'     ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_UNSET'   ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_GETPORT' ] : PORTMAP_GETPORT_Reply,
    nfs_const.PortMapperValMap[ 'PMAPPROC_DUMP'    ] : None,
    nfs_const.PortMapperValMap[ 'PMAPPROC_CALLIT'  ] : None,
}

######################################################################
## UDP packets use the RPC header first; TCP use the RPC RecordMarker,
## which is always followed by the RPC header.
######################################################################

class RPC_status( PacketNoPad ):
    name = "rpc_status"
    fields_desc = [ IntEnumField( "v", 0, nfs_const.ValAcceptStatMap ), ]

    def success( self ):
        return 0 == self.v

    def __str__( self ):
        return nfs_const.ValAcceptStatMap[ self.v ]

# N.B. see note in WRITE. In certain cases we're looking for the RPC
# header at the wrong offset. This can be fixed by tracking TCP state
# (i.e. file offsets) and using them to adjust the payload contents
# via {RPC_Header,RPC_RecordMarker}.pre_dissect()
class RPC_Header( Packet ):
    """ RPC Header. Replies save a reference to their caller packets. """
    __slots__ = Packet.__slots__ + [ 'rpc_call' ]
    
    name = "RPC Header (NF)"
    fields_desc = [ XIntField(          "xid", None ),
                    IntEnumField( "direction", None, nfs_const.ValMsgTypeMap ) ]

    pkt_ct  = 0
    def pre_dissect( self, pay ):
        RPC_Header.pkt_ct += 1

        # Basic validation
        (xid, direction) = struct.unpack( "!II", pay[:8] )

        if xid == 0 or direction not in nfs_const.ValMsgTypeMap:
            logging.debug( "Rejecting packet with checksum {:x} as non-RPC (READ/WRITE continuation?)"
                           .format( self.underlayer.underlayer.chksum ) )
            dobreak()
            raise RuntimeError( "Bad RPC header" )
        dobreak()
        return pay
            
    def _validate( self ):
        if self.direction not in nfs_const.ValMsgTypeMap:
            logging.warning( "Rejecting XID {:x} as RPC packet (READ/WRITE continuation?)"
                             .format( self.xid ) )
            dobreak()
            self.remove_payload()
            raise RuntimeError( "Illegal RPC packet direction" )
            
    seen = False
    def guess_payload_class( self, payload ):
        #if self.xid is None or self.xid in ( #0x7cfc2417,
        #        0x7dfc2417, ) or RPC_Header.seen:
        #    pdb.set_trace()
        #    RPC_Header.seen = True
            
        if self.direction is nfs_const.MsgTypeValMap['CALL']:
            logging.debug( "CALL XID: {:x}".format( self.xid ) )
            return RPC_Call
        elif self.direction is nfs_const.MsgTypeValMap['REPLY']:
            logging.debug( "REPLY XID: {:x}".format( self.xid ) )
            return RPC_Reply
        else:
            return None

    def post_dissection( self, pkt ):
        """
        When a call is seen, save it globally. Once its reply is seen,
        associate it with the reply only.
        """

        xid = self.xid

        if self.direction is nfs_const.MsgTypeValMap['CALL']:
            xid_call_map[ xid ] = self
            self.rpc_call = None
        elif self.direction is nfs_const.MsgTypeValMap['REPLY']:
            assert not hasattr( self, 'rpc_call' )
            self.rpc_call = xid_call_map.get( xid )
            if not self.rpc_call:
                logging.warning( "RPC REPLY for unknown CALL {:x}".format( xid ) )
                return

            # Remove the call packet from the global dictionary,
            # provided no more replies are expected.
            if not ( isinstance( self.underlayer, RPC_RecordMarker ) and
                     not self.underlayer.last ):
                del xid_call_map[ xid ]
                
class RPC_RecordMarker( Packet ):
    # Called "Fragment Header" in Wireshark
    name = "RPC Header (F)"
    # Incorrect: 'last' is indicated only by the most significant bit;
    # fixed up in post_dissection()
    fields_desc = [ BitField( "last",  0, 8  ),
                    BitField( "len",   0, 24 ) ]

    def pre_dissect( self, pay ):

        # Approximate validation: max size we'll accept is 64k
        # (0x10000 = 2^16). NetBSD's NFS breaks large WRITEs into 32k
        # (0x8000 = 2^15) chunks.

        maxsize = 2**16
        
        (field, ) = struct.unpack( "!I", pay[:4] )

        last = (field >> 20) # should be either 0 or 0x800
        size = (field & (2**20-1))

        if (last & 0x7ff) != 0 or (size > maxsize):
            logging.debug( "Rejecting TCP packet with checksum {:x} as non-RPC (READ/WRITE continuation?)"
                           .format( self.underlayer.chksum ) )
            dobreak()
            raise RuntimeError( "Bad RPC Record marker header" )
        dobreak()
        return pay

    '''
    def _validate( self ):
        # Reject last field other than 0x80 or 0x00 (only MSB can be asserted)
        if self.last not in (0, 1, 0x80):
            logging.warning( "Rejecting packet as RPC Record Marker. May be TCP continuation." )
            dobreak()
            self.remove_payload()
            raise RuntimeError( "Rejecting as RPC Record Marker / Fragment Header" )
    '''
    
    # Always followed by RPC Header
    def guess_payload_class( self, payload ):
        return RPC_Header

    def post_dissection( self, pkt ):
        """ Adjust fields due to imprecision above """

        self.len = ( (self.last & 0x7f) << 24) | self.len        
        if self.last:
            self.last = 1
        else:
            self.last = 0
            
class RPC_AuthNull( PacketNoPad ):
    name = "RPC Authorization NULL"
    fields_desc = []

class RPC_AuthSys( PacketNoPad ):
    name = "RPC Authorization Sys"
    # RFC-1831 (appendix A, AUTH_SYS)
    #  struct authsys_parms {
    #   unsigned int stamp;
    #   string machinename<255>;
    #   unsigned int uid;
    #   unsigned int gid;
    #   unsigned int gids<16>;
    # };

    fields_desc = [ XIntField(          "stamp",  0 ), 
                    FieldLenField(   "name_len",  0, fmt='I', length_of="machine_name" ),
                    StrLenField( "machine_name",  0, length_from=lambda pkt: pkt.name_len),
                    StrFixedLenField(     "pad", "", length_from=lambda pkt:(-pkt.name_len % 4) ),
                    PacketField(          "uid",  0, NFS3_uid ),
                    PacketField(          "gid",  0, NFS3_gid ),
                    FieldLenField( "aux_gid_ct",  0, count_of="aux_gids", fmt='I' ),
                    FieldListField(  "aux_gids",  0, IntField( "GID", 0 ),
                                    count_from = lambda pkt: pkt.aux_gid_ct), ]

    def __str__(self):
        return ("AUTH_UNIX: host {} / uid {} / gid {} / aux_gids {}"
                .format( _bin2str(self.machine_name),
                         str(self.uid),
                         str(self.gid),
                         self.aux_gids ) )
    
# Broken
'''
class RPC_Auth( Packet ):
    name = "RPC Auth"
    fields_desc = [ IntEnumField( "flavor", 0, nfs_const.ValAuthMap ),
                    XIntField(         "l", 0 ), ]
                        
    #def guess_payload_class( self, payload ):
    #    if self.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ]:
    #        return RPC_AuthSys
    #    elif self.cred_flav == nfs_const.AuthValMap[ 'AUTH_NULL' ]:
    #        return RPC_AuthNull
    #    else:
    #        return Packet.guess_payload_class( self, payload )
'''

class RPC_Call( Packet ):
    """ RPC request: can hold NFS, PORTMAP, or MOUNT """ # ?????
    name = "RPC call"
    fields_desc = [
        IntField(   "rpc_version", 0 ),
        IntEnumField(      "prog", 0,    nfs_const.ValProgramMap ),
        IntField(  "prog_version", 0 ),

        # proc should be an enum, but that's too much work given
        # scapy's constraints (the lookup depends on prog, not only
        # proc).
        IntField(          "proc", 0 ),

        # Officially, the first field in opaque_auth. Cleaner to have
        # it in an RPC_Auth class, but that doesn't seem to work.
        IntEnumField( "cred_flav", 0, nfs_const.ValAuthMap ),

        # It'd be cleaner to include RCP_Auth and direct the parser to
        # Auth Packets via guess_paylaod_class(). But, that doesn't work.
        #PacketField( "auth", None, RPC_Auth ),

        # cred_len is a part of the auth field(s) but it's easier to
        # have it here, and compatible with AUTH_UNIX and AUTH_NULL.
        IntField(      "cred_len", 0 ),
        ConditionalField( PacketLenField( "auth_sys", None, RPC_AuthSys,
                                          length_from=lambda pkt: pkt.cred_len ),
                          lambda pkt: (pkt.cred_len > 0 and
                                       pkt.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ]) ),
        ConditionalField( PacketLenField( "auth_null", None, RPC_AuthNull,
                                          length_from=lambda pkt: pkt.cred_len ),
                          lambda pkt: pkt.cred_flav == nfs_const.AuthValMap[ 'AUTH_NULL' ] ),
        PacketField(  "verifier", None, RPC_verifier ) ]

    def guess_payload_class( self, payload ):
        cls = None
        if self.prog == nfs_const.ProgramValMap['portmapper']:
            assert self.proc in portmap_call_lookup
            cls = portmap_call_lookup[ self.proc ]
        elif self.prog == nfs_const.ProgramValMap['mountd']:
            assert self.proc in mount3_call_lookup
            cls = mount3_call_lookup[ self.proc ]
        elif self.prog == nfs_const.ProgramValMap['nfs']:
            assert self.proc in nfs3_call_lookup
            cls = nfs3_call_lookup[ self.proc ]

        if not cls:
            cls = Packet.guess_payload_class( self, payload )

        logging.debug( "XID {:x} --> {}".format( self.underlayer.xid, cls.__name__ ) )
        return cls

    def post_dissection( self, pkt ):
        """ Figures out the response class, maps the XID to it in global dict """
        cls = None
        xid = self.underlayer.xid

        if self.prog == nfs_const.ProgramValMap['portmapper']:
            cls = portmap_reply_lookup[ self.proc ]
        elif self.prog == nfs_const.ProgramValMap['mountd']:
            cls = mount3_reply_lookup[ self.proc ]
        elif self.prog == nfs_const.ProgramValMap['nfs']:
            cls = nfs3_reply_lookup[ self.proc ]
        if cls:
            logging.debug( "XID {:x} --> {}".format( xid, cls.__name__ ) )
            xid_reply_map[ xid ] = cls

class RPC_ReplySuccess( PacketNoPad ):
    name = "RPC success"
    fields_desc = [ FieldLenField( "data_len",  0, length_of="data" ),
                    StrLenField(       "data", "", length_from=lambda pkt: pkt.data_len), ]

class RPC_ReplyMismatch( Packet ):
    name = "RPC mismatch"
    fields_desc = [ IntField( "low",  0 ),
                    IntField( "high", 0 ), ]

class RPC_Reply( Packet ):
    name = "RPC reply"
    fields_desc = [ IntField(          "state",    0),
                    PacketField(      "verify", None, RPC_verifier ),
                    PacketField( "accept_state", None, RPC_status ), ]

    def guess_payload_class( self, payload ):
        try:
            xid = self.underlayer.xid
        except:
            assert False # problem!
        cls = xid_reply_map.get( xid, None )
        if cls:
            del xid_reply_map[ xid ]

            if self.accept_state.success():
                logging.debug( "XID {:x} --> {}".format( self.underlayer.xid, cls.__name__ ) )
                return cls
            elif self.accept_state.v == nfs_const.AcceptStatValMap[ 'PROG_MISMATCH' ]:
                return RPC_ReplyMismatch

        return Packet.guess_payload_class(self, payload)

def RPC_Validator( Packet ):
    name = "RPC validator"
    fields_desc = []
    # ??????????????????????????

    
def bind_port_to_rpc( port, bind_udp=False, bind_tcp=False ):
    if bind_udp:
        bind_layers( UDP, RPC_Header, sport=port )
        bind_layers( UDP, RPC_Header, dport=port )

    if bind_tcp:
        bind_layers( TCP, RPC_RecordMarker,  sport=port )
        bind_layers( TCP, RPC_RecordMarker,  dport=port )

    logging.info( "RPC is bound on port {} for {} {}".
                  format( port,
                          { False : "", True : "TCP" } [bind_udp],
                          { False : "", True : "UDP" } [bind_tcp] ) )
        

######################################################################
## Global settings
######################################################################

def init():
    # Universal/common bindings. Although we might (re)discover these
    # via PORTMAP GETPORT calls, we need to assume them from the
    # beginning or else we could miss some traffic.
    #
    # 111 - portmapper
    # 955
    # 2049 - nfs
    for p in (111, 955, 2049):
        bind_port_to_rpc( p, bind_udp=True, bind_tcp=True )


