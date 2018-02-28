#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

##
## NFS/RPC constants, especially NFSv3 
## https://tools.ietf.org/html/rfc1813
##


##
## RPC RFC 1831
##

MsgTypeValMap = {
    'CALL'  : 0,
    'REPLY' : 1,
}

ValMsgTypeMap = {v: k for k, v in MsgTypeValMap.items()}


# Reply: accepted of rejected
ReplyStateValMap = {
    'MSG_ACCEPTED' : 0,
    'MSG_DENIED'   : 1,
}

ValReplyStateMap = {v: k for k, v in ReplyStateValMap.items()}

# enum accept_stat
ValAcceptStatMap = {
    0 : 'SUCCESS',
    1 : 'PROG_UNAVAIL',
    2 : 'PROG_MISMATCH',
    3 : 'PROC_UNAVAIL',
    4 : 'GARBAGE_ARGS',
    5 : 'SYSTEM_ERR',
}

AcceptStatValMap = {v: k for k, v in ValAcceptStatMap.items()}

# Auth types
ValAuthMap = {
    0 : 'AUTH_NULL',
    1 : 'AUTH_UNIX',
}

AuthValMap = {v: k for k, v in ValAuthMap.items()}


# Reasons why a call message was rejected:
ValRejectMap = {
    0 : 'RPC_MISMATCH',
    1 : 'AUTH_ERROR',
}

RejectValMap = {v: k for k, v in ValRejectMap.items()}

# Auth stat
ValAuthFailMap = {
    0 : 'AUTH_OK',
    1 : 'AUTH_BADCRED',
    2 : 'AUTH_REJECTEDCRED',
    3 : 'AUTH_BADVERF',
    4 : 'AUTH_REJECTEDVERF',
    5 : 'AUTH_TOOWEAK',
    6 : 'AUTH_INVALIDRESP',
    7 : 'AUTH_FAILED',
}

AuthFailValMap = {v: k for k, v in ValAuthFailMap.items()}


ValPortMapperMap = {
    0: 'PMAPPROC_NULL',
    1: 'PMAPPROC_SET',
    2: 'PMAPPROC_UNSET',
    3: 'PMAPPROC_GETPORT',
    4: 'PMAPPROC_DUMP',
    5: 'PMAPPROC_CALLIT'
}

PortMapperValMap = {v: k for k, v in ValPortMapperMap.items()}


ValProgramMap = {
    100000 : 'portmapper',
    100001 : 'rstat_svc',
    100002 : 'rusersd',
    100003 : 'nfs',
    100004 : 'ypserv',
    100005 : 'mountd',
    100006 : 'Remote',
    100007 : 'ypbind',
    100008 : 'walld',
    100009 : 'yppasswd',
    100010 : 'etherstatd',
    100011 : 'rquotad',
    100012 : 'sprayd',
    100013 : '3270_mapper',
    100014 : 'rje_mapper',
    100015 : 'selection_svc',
    100016 : 'database_svc',
    100017 : 'rexd',
    100018 : 'alis',
    100019 : 'sched',
    100020 : 'llockmgr',
    100021 : 'nlockmgr',
    100022 : 'x25.inr',
    100023 : 'statmon',
    100024 : 'status',
    100025 : 'Select_Lib',
    100026 : 'bootparam',
    100027 : 'Mazewars',
    100028 : 'ypupdated',
    100029 : 'Key_Server',
    100030 : 'SecureLogin',
    100031 : 'NFS_FwdInit',
    100032 : 'NFS_FwdTrns',
    100033 : 'SunLink_MAP',
    100034 : 'Net_Monitor',
    100035 : 'DataBase',
    100036 : 'Passwd_Auth',
    100037 : 'TFS_Service',
    100038 : 'NSE_Service',
    100039 : 'NSE_Daemon',
    100069 : 'ypxfrd',
    150001 : 'pcnfsd',
    200000 : 'PyramidLock',
    200001 : 'PyramidSys5',
    200002 : 'CADDS_Image',
    300001 : 'ADTFileLock'
}

ProgramValMap = {v: k for k, v in ValProgramMap.items()}


##
## NFS v3 RFC 1813
##

NFS3_FHSIZE         = 64
NFS3_COOKIEVERFSIZE =  8
NFS3_CREATEVERFSIZE =  8
NFS3_WRITEVERFSIZE  =  8


Nfs3_ValProcedureMap = {
    0  : 'NFSPROC3_NULL',
    1  : 'NFSPROC3_GETATTR',
    2  : 'NFSPROC3_SETATTR',
    3  : 'NFSPROC3_LOOKUP',
    4  : 'NFSPROC3_ACCESS',
    5  : 'NFSPROC3_READLINK',
    6  : 'NFSPROC3_READ',
    7  : 'NFSPROC3_WRITE',
    8  : 'NFSPROC3_CREATE',
    9  : 'NFSPROC3_MKDIR',
    10 : 'NFSPROC3_SYMLINK',
    11 : 'NFSPROC3_MKNOD',
    12 : 'NFSPROC3_REMOVE',
    13 : 'NFSPROC3_RMDIR',
    14 : 'NFSPROC3_RENAME',
    15 : 'NFSPROC3_LINK',
    16 : 'NFSPROC3_READDIR',
    17 : 'NFSPROC3_READDIRPLUS',
    18 : 'NFSPROC3_FSSTAT',
    19 : 'NFSPROC3_FSINFO',
    20 : 'NFSPROC3_PATHCONF',
    21 : 'NFSPROC3_COMMIT',
}

Nfs3_ProcedureValMap = {v: k for k, v in Nfs3_ValProcedureMap.items() }

Nfs3_AccessValMap = {
    'ACCESS_READ'    : 0x0001,
    'ACCESS_LOOKUP'  : 0x0002,
    'ACCESS_MODIFY'  : 0x0004,
    'ACCESS_EXTEND'  : 0x0008,
    'ACCESS_DELETE'  : 0x0010,
    'ACCESS_EXECUTE' : 0x0020,
}

Nfs3_ValAccessMap = {v: k for k,v in  Nfs3_AccessValMap.items() }


Nfs3_StableValMap = {
    'UNSTABLE' : 0,
    'DATA_SYNC' : 1,
    'FILE_SYNC' : 2
}
Nfs3_ValStableMap = {v: k for k,v in  Nfs3_StableValMap.items() }

Nfs3_CreateModeValMap = {
    'UNCHECKED' : 0,
    'GUARDED'   : 1,
    'EXCLUSIVE' : 2,
}
Nfs3_ValCreateModeMap = {v: k for k,v in  Nfs3_CreateModeValMap.items() }

Nfs3_FtypeValMap = {
    'NF3REG'  : 1,
    'NF3DIR'  : 2,
    'NF3BLK'  : 3,
    'NF3CHR'  : 4,
    'NF3LNK'  : 5,
    'NF3SOCK' : 6,
    'NF3FIFO' : 7,
}
    
Nfs3_ValFtypeMap = {v: k for k, v in Nfs3_FtypeValMap.items() }

Nfs3_ValModeMap = {
    0x00800 : 'SETUID',
    0x00400 : 'SETGID', 
    0x00200 : 'STSAVE',

    0x00100 : 'OWNER_R',
    0x00080 : 'OWNER_W',
    0x00040 : 'OWNER_X',

    0x00020 : 'GROUP_R',
    0x00010 : 'GROUP_W', 
    0x00008 : 'GROUP_X',

    0x00004 : 'OTHER_R',
    0x00002 : 'OTHER_W',
    0x00001 : 'OTHER_X',
}

Nfs3_ModeValMap = {v: k for k, v in Nfs3_ValModeMap.items() }

Nfs3_FsinfoValMap = {
    'FSF_LINK'        : 0x0001,
    'FSF_SYMLINK'     : 0x0002,
    'FSF_HOMOGENEOUS' : 0x0008,
    'FSF_CANSETTIME'  : 0x0010 }

Nfs3_ValFsinfoMap = {v: k for k, v in Nfs3_FsinfoValMap.items() }


Nfs3_StatValMap = {
    'NFS3_OK'             : 0,
    'NFS3ERR_PERM'        : 1,
    'NFS3ERR_NOENT'       : 2,
    'NFS3ERR_IO'          : 5,
    'NFS3ERR_NXIO'        : 6,
    'NFS3ERR_ACCES'       : 13,
    'NFS3ERR_EXIST'       : 17,
    'NFS3ERR_XDEV'        : 18,
    'NFS3ERR_NODEV'       : 19,
    'NFS3ERR_NOTDIR'      : 20,
    'NFS3ERR_ISDIR'       : 21,
    'NFS3ERR_INVAL'       : 22,
    'NFS3ERR_FBIG'        : 27,
    'NFS3ERR_NOSPC'       : 28,
    'NFS3ERR_ROFS'        : 30,
    'NFS3ERR_MLINK'       : 31,
    'NFS3ERR_NAMETOOLONG' : 63,
    'NFS3ERR_NOTEMPTY'    : 66,
    'NFS3ERR_DQUOT'       : 69,
    'NFS3ERR_STALE'       : 70,
    'NFS3ERR_REMOTE'      : 71,
    'NFS3ERR_BADHANDLE'   : 10001,
    'NFS3ERR_NOT_SYNC'    : 10002,
    'NFS3ERR_BAD_COOKIE'  : 10003,
    'NFS3ERR_NOTSUPP'     : 10004,
    'NFS3ERR_TOOSMALL'    : 10005,
    'NFS3ERR_SERVERFAULT' : 10006,
    'NFS3ERR_BADTYPE'     : 10007,
    'NFS3ERR_JUKEBOX'     : 10008,
}

Nfs3_ValStatMap = {v: k for k, v in Nfs3_StatValMap.items() }

# Helpful shortcut
NFS3_OK = Nfs3_StatValMap[ 'NFS3_OK' ]


##
## Mount
##

Mount3_ProcedureValMap = {
    'MOUNTPROC3_NULL'    : 0,
    'MOUNTPROC3_MNT'     : 1,
    'MOUNTPROC3_DUMP'    : 2,
    'MOUNTPROC3_UMNT'    : 3,
    'MOUNTPROC3_UMNTALL' : 4,
    'MOUNTPROC3_EXPORT'  : 5,
}

Mount3_ValProcedureMap = {v: k for k, v in Mount3_ProcedureValMap.items() }


Mount3_StatValMap = {
    'MNT3_OK'             : 0,
    'MNT3ERR_PERM'        : 1,
    'MNT3ERR_NOENT'       : 2,
    'MNT3ERR_IO'          : 5,
    'MNT3ERR_ACCES'       : 13,
    'MNT3ERR_NOTDIR'      : 20,
    'MNT3ERR_INVAL'       : 22,
    'MNT3ERR_NAMETOOLONG' : 63,
    'MNT3ERR_NOTSUPP'     : 10004,
    'MNT3ERR_SERVERFAULT' : 10006,
}

Mount3_ValStatMap = {v: k for k, v in Mount3_StatValMap.items()}
