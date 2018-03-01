#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## Scapy-based parser for NFS v3
## https://tools.ietf.org/html/rfc1813
##

## Requirements:
##  pip3 install scapy-python3

##
## These features require a newer version of Scapy than currently
## supplied via requirements installation.
##   READDIR's dynamic response
##   READDIRPLUS's dynamic response

import os
import sys
import logging

import nfs_const

from scapy.all import *


##
## RPC replies are not self-contained - they don't have all the info
## needed to figure out their type. Thus, we track transaction IDs,
## which are matched between a self-contained call message and its
## associated reply. Then, we can piece together the reply.
##
## Map: transaction ID --> NFS reply class
## Tracks outstanding XIDs so replies can be parsed.
##

xid_reply_map = dict()

##
## Most of our Packet classes need to follow this template: they have
## no padding except as explicitely stated. N.B. this is incompatible
## with guess_next_payload(). If you want to define that method, then
## inherit directly from Packet.
##
class PacketNoPad( Packet ):
    def extract_padding(self, p):
        return "", p


######################################################################
## RFC 1813, Basic Data Types
######################################################################

def generate_fields( fields, cond=None ):
    if cond:
        return [ ConditionalField( f, cond ) for f in fields ]
    else:
        return fields

##
## RFC 1813 Basic Data Types (page 15-16)
##

# filename3
#  typedef string filename3<>;

class NFS3_fname( PacketNoPad ):
    name = "fname"
    fields_desc = [ FieldLenField( "l", 0, fmt='I', length_of="v" ),
                    StrLenField( "v", "", length_from=lambda pkt: pkt.l ),
                    XStrFixedLenField( "pad", "", length_from=lambda pkt:(-pkt.l % 4) ),]

# nfspath3
#  typedef string nfspath3<>;
class NFS3_path( PacketNoPad ):
    name = "path"
    fields_desc = [ FieldLenField( "l", 0, fmt='I', length_of="v" ),
                    StrLenField( "v", "", length_from=lambda pkt: pkt.l), ]
    
# fileid3
#  typedef uint64 fileid3;
class NFS3_fileid( PacketNoPad ):
    name = "fileid"
    fields_desc = [ XLongField( "v", 0 ), ]


# cookie3
#  typedef uint64 cookie3;
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
    fields_desc = [ XLongField( "createverf", 0 ), ]

# writeverf3
#       typedef opaque writeverf3[NFS3_WRITEVERFSIZE];
class NFS3_writeverf( PacketNoPad ):
    name = "writeverf"
    fields_desc = [ XLongField( "writeverf", 0 ), ]

# uid3
#       typedef uint32 uid3;
class NFS3_uid( PacketNoPad ):
    name = "uid"
    fields_desc = [ XIntField( "uid", 0 ), ]

# gid3
#       typedef uint32 gid3;
class NFS3_gid( PacketNoPad ):
    name = "gid"
    fields_desc = [ XIntField( "gid", 0 ), ]


# size3
#       typedef uint64 size3;
class NFS3_size( PacketNoPad ):
    name = "size"
    fields_desc = [ XLongField( "size", 0 ), ]

# offset3
#       typedef uint64 offset3;
class NFS3_offset( PacketNoPad ):
    name = "offset"
    fields_desc = [ XLongField( "offset", 0 ), ]


# mode3
#       typedef uint32 mode3;
class NFS3_mode( PacketNoPad ):
    name = "mode"
    fields_desc = [ XIntField( "mode", 0 ), ]

# count3
#       typedef uint32 count3;
class NFS3_count( PacketNoPad ):
    name = "count"
    fields_desc = [ XIntField( "count", 0 ), ]

class NFS3_ftype( PacketNoPad ):
    name = "ftype"
    fields_desc = [ IntEnumField( "ftype", 0, nfs_const.Nfs3_ValFtypeMap ), ]


class RPC_verifier( PacketNoPad ):
    name = "NFS Verifier"
    fields_desc = [ IntField( "verifier_flav", 0 ),
                    FieldLenField( "verifier_len",  None, fmt="I", length_of="verifier_data" ),
                    StrLenField( "verifier_data", "",
                                 length_from=lambda pkt: pkt.verifier_len), ]

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
    fields_desc = [ FieldLenField( "l", 0, fmt='I', length_of="v" ),
                    StrLenField( "v", b"", length_from=lambda pkt: pkt.l ), ]


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
    fields_desc = [ PacketField( "ftype", None, NFS3_ftype ),
                    PacketField( "mode",  None, NFS3_mode ),
                    PacketField( "nlink", None, NFS3_count ),
                    PacketField( "uid",   None, NFS3_uid ),
                    PacketField( "gid",   None, NFS3_gid ),
                    PacketField( "size",  None, NFS3_size ),
                    PacketField( "used",  None, NFS3_size ),
                    PacketField( "specdata", None, NFS3_specdata ),
                    XLongField(  "fsid", 0 ),
                    PacketField( "fileid", None, NFS3_fileid ),
                    PacketField( "atime",  None, NFS3_time ),
                    PacketField( "mtime",  None, NFS3_time ),
                    PacketField( "ctime",  None, NFS3_time ),
    ]

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
                    ConditionalField( PacketField( "mode", None, NFS3_mode ),
                                      lambda pkt: pkt.set_mode != 0 ),
                    IntField( "set_uid", 0 ),
                    ConditionalField( PacketField( "uid", None, NFS3_uid ),
                                      lambda pkt: pkt.set_uid != 0 ),
                    IntField( "set_gid", 0 ),
                    ConditionalField( PacketField( "gid", None, NFS3_gid ),
                                      lambda pkt: pkt.set_gid != 0 ),
                    IntField( "set_size", 0 ),
                    ConditionalField( PacketField( "size", None, NFS3_size ),
                                      lambda pkt: pkt.set_size != 0 ),
                    IntField( "set_atime", 0 ),
                    ConditionalField( PacketField( "atime", None, NFS3_time ),
                                      lambda pkt: pkt.set_atime != 0 ),
                    IntField( "set_mtime", 0 ),
                    ConditionalField( PacketField( "mtime", None, NFS3_time ),
                                      lambda pkt: pkt.set_mtime != 0 ), ]

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
    fields_desc = [ IntField( "attribs_follow", 0 ),
                    PacketField( "attr", None, NFS3_wcc_attr ) ]

# Page 24

class NFS3_wcc_data( PacketNoPad ):
    name = "wcc_data"
    fields_desc = [ PacketField( "before", None, NFS3_pre_op_attr ),
                    PacketField( "after",  None, NFS3_post_op_attr ) ]


# Page 27
# struct diropargs3 {
#     nfs_fh3 dir;
#     filename3 name;
# };
def nfs3_diropargs( pre="" ):
    return nfs3_fhandle() + nfs3_fname()

class NFS3_dir_op_args( PacketNoPad ):
    name = "dir_op_args"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "name", None, NFS3_fname ), ]

# Page 31
class NFS3_sattrguard( PacketNoPad ):
    fields_desc = [ IntField( "check", 0 ),
                    ConditionalField( PacketField( "obj_ctime", None, NFS3_time ),
                                      lambda pkt:pkt.check != 0 ) ]

# Page 61
def nfs3_symlink_data( pre="", cond=None ):
    fields = nfs3_sattr("symlink.") + nfs3_path("symlink.")
    return generate_fields( fields, cond )

class NFS3_symlink_data( PacketNoPad ):
    name = "symlink_data"
    fields_desc = [ PacketField( "attr", None, NFS3_sattr ),
                    PacketField( "path", None, NFS3_path ), ]


######################################################################
## NFS Procedures: These are the NFS functions built on top of
## RPC. They're enumerated in the standard starting on page 28. The
## RPC layer (RPC_Call below) tells the scapy parser which Call-side
## class to use based on the procedure number. That same class also
## maps the transaction ID to the type of response required, thus
## enabling correct parsing of responses.
######################################################################


######################################################################
## Procedure 0: NULL
######################################################################

class NFS3_NULL_Call( Packet ):
    name = "NFS NULL call"
    fields_desc = []


class NFS3_NULL_Reply( Packet ):
    name = "NFS NULL response"
    fields_desc = []

    
######################################################################
## Procedure 1: GETTATTR
######################################################################
class NFS3_GETATTR_Call( Packet ):
    name = "NFS GETATTR call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ), ]

class NFS3_GETATTR_ReplyOk( Packet ):
    name = "NFS GETATTR Reply OK"
    fields_desc = [ PacketField( "fattr", None, NFS3_fattr ), ]

class NFS3_GETATTR_Reply( Packet ):
    name = "NFS GETATTR reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]
    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_GETATTR_ReplyOk
        Packet.guess_payload_class(self, payload)

######################################################################
## Procedure 2: SETATTR
######################################################################
class NFS3_SETATTR_Call( Packet ):
    name = "NFS SETATTR call"
    fields_desc = [ PacketField( "handle",   None, NFS3_fhandle ),
                    PacketField( "new_attr", None, NFS3_sattr ),
                    PacketField( "guard",    None, NFS3_sattrguard ), ]

    
class NFS3_SETATTR_Reply( Packet ):
    name = "NFS SETATTR reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ),
                    PacketField( "obj_wcc", None, NFS3_wcc_data ), ]


######################################################################
## Procedure 3: LOOKUP
######################################################################
class NFS3_LOOKUP_Call( Packet ):
    name = "NFS LOOKUP call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ) ]

class NFS3_LOOKUP_ReplyOk( PacketNoPad ):
    name = "NFS LOOKUP reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "obj",    None, NFS3_post_op_attr ),
                    PacketField( "dir",    None, NFS3_post_op_attr ), ]

class NFS3_LOOKUP_ReplyFail( Packet ):
    name = "NFS LOOKUP reply Fail"
    fields_desc = [ PacketField( "dir", None, NFS3_post_op_attr ), ]

class NFS3_LOOKUP_Reply( Packet ):
    name = "NFS LOOKUP reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_LOOKUP_ReplyOk
        else:
            return NFS3_LOOKUP_ReplyFail


######################################################################
## Procedure 4: ACCESS
######################################################################
class NFS3_ACCESS_Call( PacketNoPad ):
    name = "NFS ACCESS call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    XIntField( "check_access", 0 ), ]

class NFS3_ACCESS_ReplyOk( Packet ):
    name = "NFS ACCESS reply OK"
    fields_desc = [ PacketField( "obj" , None,    NFS3_post_op_attr ),
                    XIntField( "access", 0 ),                          ]
    
class NFS3_ACCESS_ReplyFail( Packet ):
    name = "NFS ACCESS reply fail"
    fields_desc = [ PacketField( "obj", None, NFS3_post_op_attr ) ]
    
class NFS3_ACCESS_Reply( Packet ):
    name = "NFS ACCESS reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]
    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_ACCESS_ReplyOk
        else:
            return NFS3_ACCESS_ReplyFail

######################################################################
## Procedure 5: READLINK
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
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_READLINK_ReplyOk
        else:
            return NFS3_READLINK_ReplyFail

######################################################################
## Procedure 6 (section 3.3.6): READ
######################################################################
class NFS3_READ_Call( Packet ):
    name = "NFS READ call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "offset", None, NFS3_offset ),
                    PacketField( "count", None, NFS3_count ), ]

    
class NFS3_READ_ReplyOk( PacketNoPad ):
    name = "NFS READ reply OK"
    fields_desc = [ PacketField( "file", None, NFS3_post_op_attr ),
                    FieldLenField( "count", 0, fmt="I", length_of="data" ),
                    IntField( "eof", 0 ),
                    IntField( "data_len", 0 ),
                    StrLenField( "data", 0, length_from=lambda pkt: pkt.count ),
                    PadField( StrLenField( "pad", "", length_from=lambda pkt: pkt.count),
                              4, padwith=b'\x00' ), ]

class NFS3_READ_ReplyFail( Packet ):
    name = "NFS READ reply Fail"
    fields_desc = [ PacketField( "file", None, NFS3_post_op_attr ), ]
    
class NFS3_READ_Reply( Packet ):
    name = "NFS READ reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_READ_ReplyOk
        else:
            return NFS3_READ_ReplyFail


######################################################################
## Procedure 7 (section 3.3.7): WRITE
######################################################################
class NFS3_WRITE_Call( Packet ):
    name = "NFS WRITE call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "offset", None, NFS3_offset ),
                    PacketField( "count", None, NFS3_count ),
                    IntEnumField( "stable", 0, nfs_const.Nfs3_ValStableMap ),
                    IntField( "data_len", 0 ),
                    StrLenField( "data", 0, length_from=lambda pkt: pkt.data_len ),
                    StrFixedLenField( "pad", "", length_from=lambda pkt:(-pkt.data_len % 4) ), ]
    def extract_padding(self, p):
        return "", p
                    
class NFS3_WRITE_ReplyOk( Packet ):
    name = "NFS WRITE reply OK"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ),
                    XIntField( "count", 0 ),
                    IntEnumField( "committed", 0, nfs_const.Nfs3_ValStableMap ),
                    PacketField( "writeverf", None, NFS3_writeverf ) ]
    
class NFS3_WRITE_ReplyFail( Packet ):
    name = "NFS WRITE reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_WRITE_Reply( Packet ):
    name = "NFS WRITE reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_WRITE_ReplyOk
        else:
            return NFS3_WRITE_ReplyFail

######################################################################
## Procedure 8 (section 3.3.8): CREATE
######################################################################
class NFS3_CREATE_Call( Packet ):
    name = "NFS CREATE call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ),
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
                    PacketField( "attr"  , None, NFS3_post_op_attr ),
                    PacketField( "wcc", None, NFS3_wcc_data ),    ]
    
class NFS3_CREATE_ReplyFail( Packet ):
    name = "NFS CREATE reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]

class NFS3_CREATE_Reply( Packet ):
    name = "NFS CREATE reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_CREATE_ReplyOk
        else:
            return NFS3_CREATE_ReplyFail

######################################################################
## Procedure 9 (section 3.3.9): MKDIR
######################################################################
class NFS3_MKDIR_Call( Packet ):
    name = "NFS MKDIR call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ),
                    PacketField( "attr"  , None, NFS3_sattr ), ]

class NFS3_MKDIR_ReplyOk( Packet ):
    name = "NFS MKDIR reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_post_op_fh ),
                    PacketField( "attr"  , None, NFS3_post_op_attr ),
                    PacketField( "wcc",    None, NFS3_wcc_data ),    ]

class NFS3_MKDIR_ReplyFail( Packet ):
    name = "NFS MKDIR reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_MKDIR_Reply( Packet ):
    name = "NFS MKDIR reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_MKDIR_ReplyOk
        else:
            return NFS3_MKDIR_ReplyFail

######################################################################
## Procedure 10 (section 3.3.10): SYMLINK
######################################################################
class NFS3_SYMLINK_Call( Packet ):
    name = "NFS SYMLINK call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ),
                    PacketField( "link"  , None, NFS3_symlink_data ), ]

class NFS3_SYMLINK_ReplyOk( Packet ):
    name = "NFS SYMLINK reply OK"
    fields_desc = [ PacketField( "handle", None, NFS3_post_op_fh ),
                    PacketField( "attr"  , None, NFS3_post_op_attr ),
                    PacketField( "wcc",    None, NFS3_wcc_data ),    ]

class NFS3_SYMLINK_ReplyFail( Packet ):
    name = "NFS SYMLINK reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]
    
class NFS3_SYMLINK_Reply( Packet ):
    name = "NFS SYMLINK reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_SYMLINK_ReplyOk
        else:
            return NFS3_SYMLINK_ReplyFail

######################################################################
## Procedure 11 (section 3.3.11): MKNOD *** NOT IMPLEMENTED FOR NOW ***
######################################################################
class NFS3_MKNOD_Call( Packet ):
    name = "NFS MKNOD call"
    fields_desc = []

class NFS3_MKNOD_Reply( Packet ):
    name = "NFS MKNOD reply"
    fields_desc = [ ]


######################################################################
## Procedure 12 (section 3.3.12): REMOVE
######################################################################
class NFS3_REMOVE_Call( Packet ):
    name = "NFS REMOVE call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ), ]

class NFS3_REMOVE_Reply( Packet ):
    name = "NFS REMOVE reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ),
                    PacketField( "wcc",    None, NFS3_wcc_data ),    ]

######################################################################
## Procedure 13 (section 3.3.13): RMDIR
######################################################################
class NFS3_RMDIR_Call( Packet ):
    name = "NFS RMDIR call"
    fields_desc = [ PacketField( "args", None, NFS3_dir_op_args ), ]

class NFS3_RMDIR_Reply( Packet ):
    name = "NFS RMDIR reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ),
                    PacketField( "wcc",    None, NFS3_wcc_data ),    ]

######################################################################
## Procedure 14 (section 3.3.14): RENAME
######################################################################
class NFS3_RENAME_Call( Packet ):
    name = "NFS RENAME call"
    fields_desc = [ PacketField( "from", None, NFS3_dir_op_args ),
                    PacketField( "to",   None, NFS3_dir_op_args ), ]

class NFS3_RENAME_Reply( Packet ):
    name = "NFS RENAME reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ),
                    PacketField( "from",    None, NFS3_wcc_data ),
                    PacketField( "to",      None, NFS3_wcc_data ), ]

    
######################################################################
## Procedure 15 (section 3.3.15): LINK
######################################################################
class NFS3_LINK_Call( Packet ):
    name = "NFS LINK call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "link" , None, NFS3_dir_op_args ), ]
    

class NFS3_LINK_Reply( Packet ):
    name = "NFS LINK reply"
    fields_desc = [ PacketField( "file",    None, NFS3_post_op_fh ),
                    PacketField( "linkdir", None, NFS3_wcc_data ), ]

######################################################################
## Procedure 16 (section 3.3.16) READDIR
######################################################################
class NFS3_READDIR_Call( Packet ):
    name = "NFS READDIR call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "cookie" , None, NFS3_cookie ),
                    PacketField( "cookieverf" , None, NFS3_cookieverf ),
                    PacketField( "count" , None, NFS3_count ), ]

class NFS3_READDIR_entry( Packet ):
    name = "NFS READDIR entry"
    fields_desc = [ PacketField( "fileid", None, NFS3_fileid ),
                    PacketField( "fname", None, NFS3_fname ),
                    PacketField( "cookie", None, NFS3_cookie ),
                    IntField( "nextentry", 0) ]
    
    @staticmethod
    def next_entry( pkt, lst, curr, remain ):
        if curr.nextentry:
            return NFS3_READDIR_entry
        else:
            return None

class NFS3_READDIR_ReplyOk( Packet ):
    name = "NFS READDIR reply ok"
    fields_desc = [ PacketField( "dir"  , None, NFS3_post_op_attr ),
                    PacketField( "cookieverf",    None, NFS3_cookieverf ),    ]

    # incomplete: some elements omitted 

    #         .....      [ PacketListField( "entries", None, cls=NFS3_READDIR_entry,
    #                                  next_cls_cb=NFS3_READDIR_entry.next_entry ),
    #                 IntField( "eof", 0) ]

class NFS3_READDIR_ReplyFail( Packet ):
    name = "NFS READDIR reply fail"
    fields_desc = [ PacketField( "dir", None, NFS3_post_op_attr ), ]

class NFS3_READDIR_Reply( Packet ):
    name = "NFS READDIR reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_READDIR_ReplyOk
        else:
            return NFS3_READDIR_ReplyFail


######################################################################
## Procedure 17 (section 3.3.17): READDIRPLUS
######################################################################
        
class NFS3_READDIRPLUS_Call( Packet ):
    name = "NFS READDIRPLUS call"
    fields_desc = [ PacketField( "handle", None, NFS3_fhandle ),
                    PacketField( "cookie" , None, NFS3_cookie ),
                    PacketField( "cookieverf" , None, NFS3_cookieverf ),
                    PacketField( "dircount" , None, NFS3_count ),
                    PacketField( "maxcount" , None, NFS3_count ), ]
    
class NFS3_READDIRPLUS_entry( Packet ):
    name = "NFS READDIRPLUS entry"
    fields_desc = [ PacketField( "fileid", None, NFS3_fileid ),
                    PacketField( "fname", None, NFS3_fname ),
                    PacketField( "cookie", None, NFS3_cookie ),
                    PacketField( "file",    None, NFS3_post_op_fh ),
                    IntField( "nextentry", 0) ]
        
    @staticmethod
    def next_entry( pkt, lst, curr, remain ):
        if curr.nextentry:
            return NFS3_READDIRPLUS_entry
        else:
            return None

class NFS3_READDIRPLUS_ReplyOk( Packet ):
    name = "NFS READDIRPLUS reply ok"
    fields_desc = [ PacketField( "dir"  , None, NFS3_post_op_attr ),
                    PacketField( "cookieverf",    None, NFS3_cookieverf ),    ]

    # incomplete: some elements omitted

    #       ....    [ PacketListField( "entries", None, cls=NFS3_READDIR_entry,
    #                                  next_class_cb=NFS3_READDIRPLUS_entry.next_entry ),
    #                 IntField( "eof", 0) ]

class NFS3_READDIRPLUS_ReplyFail( Packet ):
    name = "NFS READDIRPLUS reply fail"
    fields_desc = [ PacketField( "dir"  , None, NFS3_post_op_attr ), ]

class NFS3_READDIRPLUS_Reply( Packet ):
    name = "NFS READDIRPLUS reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_READDIRPLUS_ReplyOk
        else:
            return NFS3_READDIRPLUS_ReplyFail

######################################################################
## Procedure 18 (section 3.3.18): FSSTAT
######################################################################

class NFS3_FSSTAT_Call( Packet ):
    name = "NFS FSSTAT call"
    fields_desc = [ PacketField( "root", None, NFS3_fhandle ), ]

class NFS3_FSSTAT_ReplyOk( Packet ):
    name = "NFS FSSTAT reply OK"
    fields_desc = [ PacketField( "dir"  , None, NFS3_post_op_attr ),

                    PacketField( "tbytes", None, NFS3_size ),
                    PacketField( "fbytes", None, NFS3_size ),
                    PacketField( "abytes", None, NFS3_size ),
                    
                    PacketField( "tfiles", None, NFS3_size ),
                    PacketField( "ffiles", None, NFS3_size ),
                    PacketField( "afiles", None, NFS3_size ),

                    IntField( "invarsec", 0 ), ]
    

class NFS3_FSSTAT_ReplyFail( Packet ):
    name = "NFS FSSTAT reply fail"
    fields_desc = [ PacketField( "fs"  , None, NFS3_post_op_attr ), ]
    
class NFS3_FSSTAT_Reply( Packet ):
    name = "NFS FSSTAT reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_FSSTAT_ReplyOk
        else:
            return NFS3_FSSTAT_ReplyFail


######################################################################
## Procedure 19 (section 3.3.19): FSINFO
######################################################################

class NFS3_FSINFO_Call( Packet ):
    name = "NFS FSINFO call"
    fields_desc = [ PacketField( "root", None, NFS3_fhandle ), ]
    
class NFS3_FSINFO_ReplyFail( Packet ):
    name = "NFS FSINFO reply fail"
    fields_desc = [ PacketField( "fs"  , None, NFS3_post_op_attr ), ]
        
class NFS3_FSINFO_ReplyOk( PacketNoPad ):
    name = "NFS FSINFO reply OK"
    fields_desc = [ PacketField( "fs"  , None, NFS3_post_op_attr ),
                    XIntField( "rtmax",  0 ),
                    XIntField( "rtpref", 0 ),
                    XIntField( "rtmult", 0 ),

                    XIntField( "wtmax",  0 ),
                    XIntField( "wtpref", 0 ),
                    XIntField( "wtmult", 0 ),

                    XIntField( "dtmult", 0 ),
                    
                    PacketField( "maxfilesize", None, NFS3_size ),
                    PacketField( "time_delta", None, NFS3_time ),
                    XIntField( "properties", 0 ), ] # nfs_const.Nfs3_ValFsinfoMap

class NFS3_FSINFO_Reply( Packet ):
    name = "NFS FSINFO reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_FSINFO_ReplyOk
        else:
            return NFS3_FSINFO_ReplyFail

######################################################################
## Procedure 20 (section 3.3.20): PATHCONF
######################################################################

class NFS3_PATHCONF_Call( Packet ):
    name = "NFS PATHCONF call"
    fields_desc = [ PacketField( "path", None, NFS3_fhandle ), ]
    
class NFS3_PATHCONF_ReplyOk( Packet ):
    name = "NFS PATHCONF reply OK"
    fields_desc = [ PacketField( "dir"  , None, NFS3_post_op_attr ),
                    XIntField( "linkmax", 0 ),
                    XIntField( "name_max", 0 ),
                    XIntField( "notrunc", 0 ),
                    XIntField( "chown_restricted", 0 ),
                    XIntField( "case_insensitive", 0 ),
                    XIntField( "case_preserving", 0 ), ]
    
class NFS3_PATHCONF_ReplyFail( Packet ):
    name = "NFS PATHCONF reply fail"
    fields_desc = [ PacketField( "pathconf"  , None, NFS3_post_op_attr ), ]
    
class NFS3_PATHCONF_Reply( Packet ):
    name = "NFS PATHCONF reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_PATHCONF_ReplyOk
        else:
            return NFS3_PATHCONF_ReplyFail


######################################################################
## Procedure 21 (section 3.3.21): COMMIT
######################################################################
class NFS3_COMMIT_Call( Packet ):
    name = "NFS COMMIT call"
    fields_desc = [ PacketField( "root", None, NFS3_fhandle ),
                    PacketField( "offset", None, NFS3_offset ),
                    PacketField( "count", None, NFS3_count ), ]

class NFS3_COMMIT_ReplyOk( Packet ):
    name = "NFS COMMIT reply OK"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ),
                    PacketField( "writeverf", None, NFS3_writeverf ) ]

class NFS3_COMMIT_ReplyFail( Packet ):
    name = "NFS COMMIT reply fail"
    fields_desc = [ PacketField( "wcc", None, NFS3_wcc_data ), ]

class NFS3_COMMIT_Reply( Packet ):
    name = "NFS COMMIT reply"
    fields_desc = [ IntEnumField( "nfs_status", 0, nfs_const.Nfs3_ValStatMap ), ]

    def guess_payload_class( self, payload ):
        if self.nfs_status == nfs_const.NFS3_OK:
            return NFS3_COMMIT_ReplyOk
        else:
            return NFS3_COMMIT_ReplyFail


## Lookup table: Procedure value --> Call handler
nfs3_call_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL' ]        : NFS3_NULL_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR' ]     : NFS3_GETATTR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR' ]     : NFS3_SETATTR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP' ]      : NFS3_LOOKUP_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS' ]      : NFS3_ACCESS_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK' ]    : NFS3_READLINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ' ]        : NFS3_READ_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE' ]       : NFS3_WRITE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE' ]      : NFS3_CREATE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR' ]       : NFS3_MKDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK' ]     : NFS3_SYMLINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD' ]       : NFS3_MKNOD_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE' ]      : NFS3_REMOVE_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR' ]       : NFS3_RMDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME' ]      : NFS3_RENAME_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK' ]        : NFS3_LINK_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR' ]     : NFS3_READDIR_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : NFS3_READDIRPLUS_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT' ]      : NFS3_FSSTAT_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO' ]      : NFS3_FSINFO_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF' ]    : NFS3_PATHCONF_Call,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT' ]      : NFS3_COMMIT_Call,
}

## Lookup table: Procedure value --> Response handler
nfs3_reply_lookup = {
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_NULL' ]        : NFS3_NULL_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_GETATTR' ]     : NFS3_GETATTR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SETATTR' ]     : NFS3_SETATTR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LOOKUP' ]      : NFS3_LOOKUP_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_ACCESS' ]      : NFS3_ACCESS_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READLINK' ]    : NFS3_READLINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READ' ]        : NFS3_READ_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_WRITE' ]       : NFS3_WRITE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_CREATE' ]      : NFS3_CREATE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKDIR' ]       : NFS3_MKDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_SYMLINK' ]     : NFS3_SYMLINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_MKNOD' ]       : NFS3_MKNOD_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_REMOVE' ]      : NFS3_REMOVE_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RMDIR' ]       : NFS3_RMDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_RENAME' ]      : NFS3_RENAME_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_LINK' ]        : NFS3_LINK_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIR' ]     : NFS3_READDIR_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_READDIRPLUS' ] : NFS3_READDIRPLUS_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSSTAT' ]      : NFS3_FSSTAT_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_FSINFO' ]      : NFS3_FSINFO_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_PATHCONF' ]    : NFS3_PATHCONF_Reply,
    nfs_const.Nfs3_ProcedureValMap[ 'NFSPROC3_COMMIT' ]      : NFS3_COMMIT_Reply,
}

        
######################################################################
## RPC pre-headers. UDP packets use the RPC header first, TCP use the
## RPC RecordMarker, which is always followed by the RPC header.
######################################################################
frag_header = [ BitField( "last",          0, 8  ),
                BitFieldLenField( "len",   0, 24 ) ]

rpc_base = [ XIntField( "xid",          0 ),
             IntEnumField( "direction", 0,
                           nfs_const.ValMsgTypeMap ) ]

# RPC Header
class RPC_Header( Packet ):
    name = "RPC Header (NF)"
    fields_desc = [ XIntField( "xid",          0 ),
                    IntEnumField( "direction", 0,
                                  nfs_const.ValMsgTypeMap ) ]
    
class RPC_RecordMarker( Packet ):
    name = "RPC Header (F)"
    fields_desc = [ BitField( "last",          0, 8  ),
                    BitFieldLenField( "len",   0, 24 ) ]

    # Always followed by RPC Header
    def guess_payload_class( self, payload ):
        return RPC_Header
    
    
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

    #IntEnumField( "cred_flav",    0, nfs_const.ValAuthMap ),
    fields_desc = [ #IntField(     "cred_len",     0 ), # maybe should be in RPC_Auth?
        XIntField( "stamp", 0 ), 
                    FieldLenField( "name_len", 0, fmt='I', length_of="machine_name" ),
                    StrLenField( "machine_name", 0, length_from=lambda pkt: pkt.name_len),
                    #PadField( StrField( "pad", ""), 4 ),
                    StrFixedLenField( "pad", "", length_from=lambda pkt:(-pkt.name_len % 4) ),
                    #                       length_from=lambda pkt: pkt.name_len),
                    #          align=4, padwith=b"\x00" ),
                    #length_from=lambda pkt: pkt.name_len ),
                    #          4 , padwith=b'\x00' ),
                    IntField( "uid", 0 ),
                    IntField( "gid", 0 ),
                    FieldLenField( "aux_gid_ct", 0, count_of="aux_gids", fmt='I' ),
                    FieldListField( "aux_gids", 0, IntField( "GID", 0 ),
                                    count_from = lambda pkt: pkt.aux_gid_ct), ]

    #def guess_payload_class( self, payload ):
    #    #import pdb;pdb.set_trace()
    #    return NFS3_Verifier
    #    #return None

    #@staticmethod
    #def get_fields(cond=None):
    #    return generate_fields( RPC_AuthSys.fields_desc, cond )
    
class RPC_Auth( Packet ):
    name = "RPC Auth"
    fields_desc = [ IntEnumField( "cred_flav",    0, nfs_const.ValAuthMap ), ]
                    #IntField(     "cred_len",     0 ),
                    #FieldLenField(     "cred_len",     0, fmt='I' ),
                    #ConditionalField(
                    #    PacketLenField( "sys", None, RPC_AuthSys,
                    #                    length_from=lambda pkt: pkt.cred_len ),
                    #    lambda pkt: (pkt.cred_len > 0 and
                    #                 pkt.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ] ) ) ]
                        
    #] #ConditionalField( PacketField( "sys", None, RPC_AuthSys ),
    #                  lambda pkt: (pkt.cred_len > 0 and
    #                               pkt.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ] ) ) ]

    def guess_payload_class( self, payload ):
        import pdb;pdb.set_trace()
        if self.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ]:
            return RPC_AuthSys
        return Packet.guess_payload_class( self, payload )
                    
    def extract_padding(self, p):
        return "", p

    #RPC_AuthSys.get_fields( lambda pkt:(pkt.cred_len > 0 and
    #                                                    pkt.cred_flav ==
    #                                                    nfs_const.AuthValMap[ 'AUTH_UNIX' ] ) )
    #def guess_payload_class( self, payload ):
    #    #import pdb;pdb.set_trace()
    #    return NFS3_Verifier

class RPC_Call( Packet ):
    """ RPC request: NFS or PORTMAP, but *NOT* MOUNT """ # ?????
    name = "RPC call"
    fields_desc = [
        IntField(     "rpc_version",  0 ),
        IntEnumField( "prog",         0,    nfs_const.ValProgramMap ),
        IntField(     "prog_version", 0 ),

        # proc should be an enum, but that's too much work given
        # scapy's constraints (the lookup depends on prog and proc).
        IntField(     "proc",         0 ),

        IntEnumField( "cred_flav",    0, nfs_const.ValAuthMap ),
        # It'd be cleaner to include RCP_Auth and direct the parser to
        # Auth Packets via guess_paylaod_class(). But, that doesn't work.
        #PacketField( "auth", None, RPC_Auth ),

        # cred_len is a part of the auth field(s) but it's easier to
        # have it here, and compatible with AUTH_UNIX and AUTH_NULL.
        IntField(     "cred_len",     0 ),
        ConditionalField( PacketLenField( "sys", None, RPC_AuthSys,
                                          length_from=lambda pkt: pkt.cred_len ),
                          lambda pkt: (pkt.cred_len > 0 and
                                       pkt.cred_flav == nfs_const.AuthValMap[ 'AUTH_UNIX' ]) ),

        PacketField(  "verifier", None, RPC_verifier ) ]

    def guess_payload_class( self, payload ):
        if self.prog == nfs_const.ProgramValMap['portmapper']:
            cls = PORTMAP_Call
        elif self.prog == nfs_const.ProgramValMap['mountd']:
            cls = MOUNT_Call
        elif self.prog == nfs_const.ProgramValMap['nfs']:
            cls = nfs3_call_lookup[ self.proc ]
        else:
            cls = Packet.guess_payload_class( self, payload )

        logging.debug( "XID {} --> {}".format( self.underlayer.xid, cls.__name__ ) )
        return cls

    def post_dissection( self, pkt ):
        """ Figures out the response class, maps the XID to it in global dict """
        resp_class = None
        if self.prog == nfs_const.ProgramValMap['portmapper']:
            resp_class = PORTMAP_Reply
        elif self.prog == nfs_const.ProgramValMap['mountd']:
            resp_class = MOUNT_Reply
        elif self.prog == nfs_const.ProgramValMap['nfs']:
            resp_class = nfs3_reply_lookup[ self.proc ]

        if resp_class:
            xid = self.underlayer.xid
            logging.debug( "XID {:x} --> {}".format( xid, resp_class.__name__ ) )
            xid_reply_map[ xid ] = resp_class
        else:
            pass

class RPC_ReplySuccess( Packet ):
    name = "RPC success"
    fields_desc = [ FieldLenField( "data_len", 0, length_of="data" ),
                    StrLenField( "data", "", length_from=lambda pkt: pkt.data_len), ]
    def extract_padding(self, p):
        return "", p

class RPC_ReplyMismatch( Packet ):
    name = "RPC mismatch"
    fields_desc = [ IntField( "low",  0 ),
                    IntField( "high", 0 ), ]

class RPC_Reply( Packet ):
    name = "RPC reply"
    fields_desc = [ IntField( "state", 0),
                    PacketField( "verify", None, RPC_verifier ),
                    IntField( "accept_state", 0), ]

    def guess_payload_class( self, payload ):
        xid = self.underlayer.xid
        resp_class = xid_reply_map.get( xid, None )
        if resp_class:
            del xid_reply_map[ xid ]

            if self.accept_state == nfs_const.AcceptStatValMap[ 'SUCCESS' ]:
                print( "XID {:x} --> {}".format( self.underlayer.xid, resp_class.__name__ ) )
                return resp_class
            elif self.accept_state == nfs_const.AcceptStatValMap[ 'PROG_MISMATCH' ]:
                return RPC_ReplyMismatch

        return Packet.guess_payload_class(self, payload)
        
class PORTMAP_Call( Packet ):
    name = "PORTMAP call"
    fields_desc = [ IntField( "program", 0 ),
                    IntField( "version", 0 ),
                    IntField( "proto",   0 ),
                    IntField( "port",    0 ) ]

class PORTMAP_Reply( Packet ):
    name = "PORTMAP reply"
    fields_desc = [
        IntField( "port", 0 ) ]
    
    
class MOUNT_Call( PacketNoPad ):
    name = "NFS MOUNT call"
    fields_desc = [
        FieldLenField( "path_len", 0, length_of="path", fmt='I' ),
        StrLenField( "path", 0, length_from=lambda pkt: pkt.path_len ),
        PadField( StrLenField( "pad", "",
                               length_from=lambda pkt: pkt.path_len ), 4, padwith=b'\x00' ),
    ]

class MOUNT_Reply( Packet ):
    name = "NFS MOUNT reply"
    fields_desc = [ IntField( "fhs_status", 0 ),
                    PacketField( "handle", None, NFS3_fhandle ),
                    FieldLenField( "auth_flavor_ct", 0, count_of="auth_flavors", fmt='I' ),
                    FieldListField( "auth_flavors", 0,
                                    IntEnumField( "flavor", 0, nfs_const.ValAuthMap ),
                                    count_from=lambda pkt: pkt.auth_flavor_ct ) ]

##
## N.B. bindings for replies are insufficient. The script needs to
## track XIDs and associate replies with their associated request
## types.
##

# Universal bindings
for p in (111, 955, 2049):
    bind_layers( UDP, RPC_Header, sport=p )
    bind_layers( UDP, RPC_Header, dport=p )
    
    bind_layers( TCP, RPC_RecordMarker,  sport=p )
    bind_layers( TCP, RPC_RecordMarker,  dport=p )

# RPC -> Call or Reply?
bind_layers( RPC_Header,  RPC_Call,  direction=nfs_const.MsgTypeValMap['CALL'] )
bind_layers( RPC_Header,  RPC_Reply, direction=nfs_const.MsgTypeValMap['REPLY'] )

