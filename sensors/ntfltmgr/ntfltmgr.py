'''
ntfltmgr.py - interface with the mini-port filter manager via python
'''
import sys
import time
import json
import logging
from enum import IntEnum
from collections import namedtuple
from ctypes import c_longlong, c_ulonglong, c_void_p, HRESULT, POINTER, Structure
from ctypes import cast, create_string_buffer, byref, sizeof, WINFUNCTYPE, windll

from ctypes.wintypes import WPARAM, DWORD, LPCWSTR, LPDWORD, LPVOID, LPCVOID
from ctypes.wintypes import LPHANDLE, ULONG, WCHAR, USHORT, WORD, HANDLE, BYTE, BOOL

S_OK = 0

ULONG_PTR = WPARAM
NTSTATUS = DWORD
PVOID = c_void_p
ULONGLONG = c_ulonglong
LONGLONG = c_longlong

logger = logging.getLogger("ntfltmgr")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

class SaviorStruct(Structure):
    '''
    Implement a base class that takes care of JSON serialization/deserialization, 
    internal object instance state and other housekeeping chores
    '''

    def __str__(self):
        '''
        returns a string that shows this instances internal state
        '''
        szfields = len(type(self)._fields_)
        state = "{0}@{1}: ".format(type(self),hex(id(self)),)
        for ndx in range(0, szfields):            
            this_name = type(self)._fields_[ndx][0]
            this_value = getattr(self, this_name)                           
            state += "{0}={1},".format( this_name, this_value,)
        state = state[:len(state)-1]
        return state

    __repr__ = __str__

    
    @classmethod
    def GetProbeDataHeader(cls, msg_pkt):
        '''
        accepts raw packet data from the driver and returns
        the ProbeDataHeader in the form of a named tuple
        '''     
        offset = sizeof(FILTER_MESSAGE_HEADER)
        length = offset + sizeof(ProbeDataHeader)               
        info = cast(msg_pkt[offset:length], 
                POINTER(ProbeDataHeader))        
        pdh = GetProbeDataHeader(DataType(info.contents.Type), 
                                 info.contents.DataSz, 
                                 info.contents.CurrentGMT)        
        return pdh
    
    @classmethod
    def GetFilterMessageHeader(cls, msg_pkt):
        '''
        accepts raw packet data from the driver and returns
        the FILTER_MESSAGE_HEADER in the form of a named tuple
        '''        
        pdh = FilterMessageHeader(msg_pkt.ReplyLength, 
                                  msg_pkt.MessageId,
                                  msg_pkt.Message) 
        return pdh

class CtypesEnum(IntEnum):
    '''
    A ctypes-compatible IntEnum superclass
    '''
    @classmethod
    def from_param(cls, obj):
        '''
        convert to integer value
        '''
        return int(obj)

class PARAM_FLAG(CtypesEnum):
    '''
    Parameter Flag Enumerations
    '''
    FIN   = (1 << 0)
    FOUT  = (1 << 1)
    FLCID = (1 << 2)
    COMBINED = FIN | FOUT | FLCID


class CLIENT_ID(SaviorStruct):
    '''
    The CLIENT ID Structure
    '''
    _fields_ = [
        ("UniqueProcess", c_void_p),
        ("UniqueThread", c_void_p)
    ]

class INSTANCE_INFORMATION_CLASS(CtypesEnum):
    '''
    Type of filter instance information requested
    '''
    InstanceBasicInformation   = 0x0
    InstanceFullInformation    = 0x1
    InstancePartialInformation = 0x2
    InstanceAggregateStandardInformation = 0x3

class FILTER_INFORMATION_CLASS(CtypesEnum):
    '''
    Type of filter driver information requested
    '''
    FilterFullInformation              = 0x0
    FilterAggregateBasicInformation    = 0x1
    FilterAggregateStandardInformation = 0x2

class FILTER_FULL_INFORMATION(SaviorStruct):
    '''
    contains full information for a minifilter driver
    '''
    _fields_ = [
        ("NextEntryOffset", ULONG),
        ("FrameID", ULONG),
        ("NumberOfInstances", ULONG),
        ("FilterNameLength", USHORT),
        ("FilterNameBuffer", WCHAR)
    ]
FilterFullInformation = namedtuple('FilterFullInformation',
                                   ['FrameID', 'NumberOfInstances', 'FilterName'])
class INSTANCE_FULL_INFORMATION(SaviorStruct):
    '''
    contains full information for the minifilter instance
    '''
    _fields_ = [
        ("NextEntryOffset",ULONG),
        ("InstanceNameLength",USHORT),
        ("InstanceNameBufferOffset",USHORT),
        ("AltitudeLength",USHORT),
        ("AltitudeBufferOffset",USHORT),
        ("VolumeNameLength",USHORT),
        ("VolumeNameBufferOffset",USHORT),
        ("FilterNameLength",USHORT),
        ("FilterNameBufferOffset",USHORT)
    ]

FilterInstanceInformation = namedtuple('FilterInstanceInformation', 
                                       ['Instancename', 'Altitude', 'VolumeName', 'FilterName'])

class OVERLAPPED(SaviorStruct):
    '''
    Contains information used in asynchronous (or overlapped) input and output (I/O).
    '''    
    _fields_ = [
        ("Internal", ULONG_PTR),
        ("InternalHigh", ULONG_PTR),
        ("Pointer", PVOID),
        ("hEvent", HANDLE)
    ]

class SECURITY_ATTRIBUTES(SaviorStruct):
    '''
    Windows Security Attributes
    '''
    _fields_ = [
        ("nLength", DWORD),
        ("lpSecurityDescriptor", LPVOID),    
        ("bInheritHandle", BOOL)
    ]

class FILTER_REPLY_HEADER(SaviorStruct):
    '''
    contains message reply header information.  It is a container for a reply 
    that the application sends in response to a message received from a 
    kernel-mode minifilter or minifilter instance
    '''
    _fields_ = [
       ('Status', NTSTATUS),
       ('MessageId', ULONGLONG)
    ]

FilterReplyHeader = namedtuple('FilterReplyHeader', ['Status', 'MessageId', 'Message'])    
    
class FILTER_MESSAGE_HEADER(SaviorStruct):
    '''
    Contains message header information. To receive messages from a kernel-mode 
    minifilter, a user-mode application typically defines a custom message 
    structure. This structure typically consists of this header structure, 
    followed by an application-defined structure to hold the actual message data.
    '''
    _fields_ = [
       ('ReplyLength', ULONG),
       ('MessageId', ULONGLONG)       
    ]
    
FilterMessageHeader = namedtuple('FilterMessageHeader', ['ReplyLength', 'MessageId', 'Message'])
        

class DataType(CtypesEnum):
    '''
    Type of filter driver data to unpack
    '''
    NONE           = 0x0000
    LoadedImage    = 0x0001
    ProcessCreate  = 0x0002
    ProcessDestroy = 0x0003
    ThreadCreate   = 0x0004
    ThreadDestroy  = 0x0005    

class LIST_ENTRY(SaviorStruct):
    '''
    Two Way Linked List - forward declare an incomplete type
    '''
    pass

LIST_ENTRY._fields_ = [
    ("Flink", POINTER(LIST_ENTRY)),
    ("Blink", POINTER(LIST_ENTRY))
]

GetProbeDataHeader = namedtuple('GetProbeDataHeader',  ['Type', 'DataSz', 'CurrentGMT'])
    
class ProbeDataHeader(SaviorStruct):
    '''
    Probe Data Header
    '''
    _fields_ = [
        ("Type", USHORT), 
        ("DataSz", USHORT), 
        ("CurrentGMT", LONGLONG),
        ("ListEntry", LIST_ENTRY)
    ]

    
GetLoadedImageInfo = namedtuple('GetLoadedImageInfo',  ['ReplyLength', 'MessageId', 'Type', 'DataSz', 'CurrentGMT',  'ProcessId', 'EProcess', 'ImageBase', 'ImageSize', 'FullImageName'])
    
class LoadedImageInfo(SaviorStruct):
    '''
    Probe Data Header
    '''
    _fields_ = [
        ("FltMsgHeader", FILTER_MESSAGE_HEADER),
        ("Header", ProbeDataHeader),
        ("ProcessId", PVOID),
        ("EProcess", PVOID),
        ("ImageBase", PVOID),
        ("ImageSize", LONGLONG),
        ("FullImageNameSz", USHORT),
        ("FullImageName", BYTE * 1)
    ]
    
    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt, POINTER(cls))
        length = info.contents.FullImageNameSz
        offset = type(info.contents).FullImageName.offset
        sb = create_string_buffer(msg_pkt.Message)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ModuleName = "".join(map(chr, slc[::2]))
        img_nfo = GetLoadedImageInfo(
            info.contents.FltMsgHeader.ReplyLength, 
            info.contents.FltMsgHeader.MessageId,
            DataType(info.contents.Header.Type).name, 
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            info.contents.EProcess,
            info.contents.ImageBase, 
            info.contents.ImageSize,
            ModuleName)
        return img_nfo


GetProcessCreateInfo = namedtuple('GetProcessCreateInfo',  ['ReplyLength', 
    'MessageId', 'Type', 'DataSz', 'CurrentGMT', 'ParentProcessId', 'ProcessId', 'EProcess', 
    'UniqueProcess', 'UniqueThread', 'FileObject', 'CreationStatus', 'CommandLine'])
    
class ProcessCreateInfo(SaviorStruct):
    '''
    ProcessCreateInfo Definition
    '''
    _fields_ = [
        ("FltMsgHeader", FILTER_MESSAGE_HEADER),
        ("Header", ProbeDataHeader),
        ("ParentProcessId", HANDLE),
        ("ProcessId", HANDLE),
        ("EProcess", PVOID),
        ("CreatingThreadId", CLIENT_ID),
        ("FileObject", PVOID),
        ("CreationStatus", NTSTATUS),
        ("FileObject", PVOID),
        ("CreationStatus", NTSTATUS),        
        ("CommandLineSz", USHORT),
        ("CommandLine", BYTE * 1)        
    ]
    
    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt, POINTER(cls))
        length = info.contents.CommandLineSz
        offset = type(info.contents).CommandLine.offset
        sb = create_string_buffer(msg_pkt.Message)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        CommandLine = "".join(map(chr, slc[::2]))
        create_info = GetLoadedImageInfo(
            info.contents.FltMsgHeader.ReplyLength, 
            info.contents.FltMsgHeader.MessageId,
            DataType(info.contents.Header.Type).name, 
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ParentProcessId,
            info.contents.ProcessId,
            info.contents.EProcess,
            info.contents.CreatingThreadId.UniqueProcess, 
            info.contents.CreatingThreadId.UniqueThread,
            info.contents.FileObject,
            info.contents.CreationStatus,
            info.contents.EProcess,            
            CommandLine)
        return create_info

GetProcessDestroyInfo = namedtuple('GetProcessDestroyInfo',  ['ReplyLength', 
    'MessageId', 'Type', 'DataSz', 'CurrentGMT', 'ProcessId', 'EProcess'])
    
class ProcessDestroyInfo(SaviorStruct):
    '''
    ProcessDestroyInfo Definition
    '''
    _fields_ = [
        ("FltMsgHeader", FILTER_MESSAGE_HEADER),
        ("Header", ProbeDataHeader),
        ("ProcessId", HANDLE),
        ("EProcess", PVOID) 
    ]
    
    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt, POINTER(cls))
        create_info = GetLoadedImageInfo(
            info.contents.FltMsgHeader.ReplyLength, 
            info.contents.FltMsgHeader.MessageId,
            DataType(info.contents.Header.Type).name, 
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ParentProcessId,
            info.contents.ProcessId,
            info.contents.EProcess)
        return create_info
    
        
ERROR_INSUFFICIENT_BUFFER = 0x7a
ERROR_INVALID_PARAMETER = 0x57
ERROR_NO_MORE_ITEMS = 0x103

logger = logging.getLogger("ntfltmgr")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

_FilterReplyMessageProto = WINFUNCTYPE(HRESULT, HANDLE, POINTER(FILTER_REPLY_HEADER), DWORD)
_FilterReplyMessageParamFlags = (0, "hPort"), (0,  "lpReplyBuffer"), (0, "dwReplyBufferSize")
_FilterReplyMessage = _FilterReplyMessageProto(("FilterReplyMessage", windll.fltlib), _FilterReplyMessageParamFlags)
def FilterReplyMessage(hPort, status, msg_id, msg):
    '''    
    replies to a message from a kernel-mode minifilter 
    @note close the handle returned using CloseHandle
    @param hPort port handle returned by a previous call to 
    FilterConnectCommunicationPort. This parameter is required and cannot be NULL.
    @param reply_buffer caller-allocated buffer containing the reply to be sent 
    to the minifilter. The reply must contain a FILTER_REPLY_HEADER structure, 
    but otherwise, its format is caller-defined. This parameter is required and 
    cannot be NULL
    @returns S_OK if successful. Otherwise, it returns an error value
    '''    
    res = HRESULT()
        
    if msg is None or not hasattr(msg, "__len__") or len(msg) <= 0:
        raise ValueError("Parameter msg is invalid!")
    
    if isinstance(msg, str):
        txt = msg
        msg = bytearray()
        msg.extend(map(ord, txt))                    
    msg_len = len(msg)    
    sz_frh = sizeof(FILTER_REPLY_HEADER)
    # What's the messages total length in bytes
    total_len = msg_len + sz_frh
    reply_buffer = create_string_buffer(total_len)
    info = cast(reply_buffer, POINTER(FILTER_REPLY_HEADER))    
    info.contents.Status = status
    info.contents.MessageId = msg_id        
    reply_buffer[sz_frh:sz_frh + msg_len] = msg    

    try:
        res = _FilterReplyMessage(hPort, info, total_len)
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        logger.exception("FilterReplyMessage Failed on Message Reply - Error %d", lasterror)
        res = lasterror

    return res

_FilterGetMessageProto = WINFUNCTYPE(HRESULT, HANDLE, POINTER(FILTER_MESSAGE_HEADER), DWORD, POINTER(OVERLAPPED))
_FilterGetMessageParamFlags = (0, "hPort"), (0,  "lpMessageBuffer"), (0, "dwMessageBufferSize"), (0,  "lpOverlapped", 0)
_FilterGetMessage = _FilterGetMessageProto(("FilterGetMessage", windll.fltlib), _FilterGetMessageParamFlags)
def FilterGetMessage(hPort, msg_len):
    '''    
    Get a message from filter driver     
    @param hPort the port handle from the driver we are communicating with
    @param msg_len the message length that we expect to receive
    @returns hPort Port Handle
    '''    
    res = HRESULT()
    
    if msg_len is None or not isinstance(msg_len, int) or msg_len <= 0:
        raise ValueError("Parameter MsgBuf is invalid!")
    
    sb = create_string_buffer(msg_len)        
    info = cast(sb, POINTER(FILTER_MESSAGE_HEADER))
    try:
        res = _FilterGetMessage(hPort, byref(info.contents), msg_len, cast(None, POINTER(OVERLAPPED)))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        logger.exception("FilterGetMessage failed to Get Message - Error %d", lasterror)
        raise

    ReplyLen = info.contents.ReplyLength
    MessageId = info.contents.MessageId
    msg_pkt = FilterMessageHeader(ReplyLen, MessageId, sb[sizeof(FILTER_MESSAGE_HEADER):])
    return res, msg_pkt

_FilterConnectCommunicationPortProto = WINFUNCTYPE(HRESULT, LPCWSTR, DWORD, LPCVOID, WORD, LPDWORD, POINTER(HANDLE))
_FilterConnectCommunicationPortParamFlags = (0, "lpPortName"), (0,  "dwOptions", 0), (0, "lpContext", 0), (0,  "dwSizeOfContext", 0), (0, "lpSecurityAttributes", 0), (1, "hPort")
_FilterConnectCommunicationPort = _FilterConnectCommunicationPortProto(("FilterConnectCommunicationPort", windll.fltlib), 
                                                         _FilterConnectCommunicationPortParamFlags)
def FilterConnectCommunicationPort(PortName):
    '''    
    returns information about a mini filter driver instance and is used as a 
    starting point for scanning the instances 
    @note close the handle returned using CloseHandle
    @param PortName fully qualified name of the communication server port
    @returns hPort Port Handle
    '''
    hPort = HANDLE(-1)
    res = HRESULT()

    try:
        res = _FilterConnectCommunicationPort(PortName, 0, None, 0, None, byref(hPort))        
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        logger.exception("Failed to connect to port %s error %d", PortName, lasterror)
        raise
    else:
        time.sleep(1)
        return res, hPort
def _build_filter_instance_info(buf):
    '''
    Create the FilterInstanceInformation instance
    '''
    sb = create_string_buffer(buf.raw)
    info = cast(sb, POINTER(INSTANCE_FULL_INFORMATION))

    length = info.contents.InstanceNameLength
    offset = info.contents.InstanceNameBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    InstanceName = "".join(map(chr, slc[::2]))

    length = info.contents.AltitudeLength
    offset = info.contents.AltitudeBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    Altitude = "".join(map(chr, slc[::2]))

    length = info.contents.VolumeNameLength
    offset = info.contents.VolumeNameBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    VolumeName = "".join(map(chr, slc[::2]))

    length = info.contents.FilterNameLength
    offset = info.contents.FilterNameBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    FilterName = "".join(map(chr, slc[::2]))

    fii = FilterInstanceInformation(InstanceName, Altitude, VolumeName, FilterName)

    return fii

_FilterInstanceFindFirstProto = WINFUNCTYPE(HRESULT, LPCWSTR, INSTANCE_INFORMATION_CLASS, c_void_p, DWORD, LPDWORD, LPHANDLE)
_FilterInstanceFindFirstParamFlags = (0, "lpFilterName"), (0,  "dwInformationClass"), (1, "lpBuffer"), (0,  "dwBufferSize"), (1, "lpBytesReturned"), (1, "lpFilterInstanceFind")
_FilterInstanceFindFirst = _FilterInstanceFindFirstProto(("FilterInstanceFindFirst", windll.fltlib), 
        _FilterInstanceFindFirstParamFlags)
def FilterInstanceFindFirst(filter_name):
    '''
    returns information about a mini filter driver instance and is used as a 
    starting point for scanning the instances 
    @param filter_name name of thje minifilter driver that owns the instance
    @returns Handle to be utilized by FilterFindNext and FilterFindClose
    @returns filter data
    '''
    infocls=INSTANCE_INFORMATION_CLASS.InstancePartialInformation
    BytesReturned = DWORD()
    ifi = INSTANCE_FULL_INFORMATION()
    hFilterInstanceFind = HANDLE(-1)

    res = HRESULT()

    try:
        res = _FilterInstanceFindFirst(filter_name, infocls, byref(ifi), sizeof(ifi), 
                byref(BytesReturned), byref(hFilterInstanceFind))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        if lasterror == ERROR_NO_MORE_ITEMS:
            return None, None
        if lasterror != ERROR_INSUFFICIENT_BUFFER:       
            raise

    buf = create_string_buffer(BytesReturned.value)
    res = _FilterInstanceFindFirst(filter_name, infocls, buf, sizeof(buf), byref(BytesReturned), 
            byref(hFilterInstanceFind))

    if res != S_OK:
        return None, None

    fii = _build_filter_instance_info(buf)

    return hFilterInstanceFind, fii

_FilterInstanceFindNextProto = WINFUNCTYPE(HRESULT, HANDLE, FILTER_INFORMATION_CLASS, c_void_p, DWORD, LPDWORD)
_FilterInstanceFindNextParamFlags = (0,  "hFilterInstanceFind"), (0, "dwInformationClass"), (1, "lpBuffer"), (0,  "dwBufferSize"), (1, "lpBytesReturned")
_FilterInstanceFindNext = _FilterInstanceFindNextProto(("FilterInstanceFindNext", windll.fltlib), _FilterInstanceFindNextParamFlags)

def FilterInstanceFindNext(hFilterInstanceFind):
    '''
    continues a filter search started by a call to FilterInstanceFindFirst
    @param hFilterInstanceFind the filter search handle as returend by a previous 
    call to FilterInstanceFindFirst
    @param infocls The type of filter driver information requested.    
    @returns filter_name this filters name
    '''
    infocls=INSTANCE_INFORMATION_CLASS.InstancePartialInformation
    BytesReturned = DWORD()
    ffi = FILTER_FULL_INFORMATION()
    res = HRESULT()

    try:
        res = _FilterInstanceFindNext(hFilterInstanceFind.value, infocls, byref(ffi), sizeof(ffi), byref(BytesReturned))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        if lasterror == ERROR_NO_MORE_ITEMS:
            return lasterror, None
        if lasterror != ERROR_INSUFFICIENT_BUFFER:       
            raise

    buf = create_string_buffer(BytesReturned.value)
    res = _FilterInstanceFindNext(hFilterInstanceFind, infocls, buf, sizeof(buf), byref(BytesReturned))

    if res != S_OK:
        return res, None

    fii = _build_filter_instance_info(buf)

    return S_OK, fii

_FilterInstanceFindCloseProto = WINFUNCTYPE(HRESULT, HANDLE, DWORD)
_FilterInstanceFindCloseParamFlags = (0,  "hFilterInstanceFind"), (0, "biff", 0)
_FilterInstanceFindClose = _FilterInstanceFindCloseProto(("FilterInstanceFindClose", windll.fltlib), _FilterInstanceFindCloseParamFlags)

def FilterInstanceFindClose(hFilterInstanceFind):
    '''
    closes the specified minifilter instance search handle
    @param hFilterInstanceFind the filter search handle as returend by a 
    previous call to FilterInstanceFindFirst
    @returns HRESULT
    '''

    res = _FilterInstanceFindClose(hFilterInstanceFind.value)
    return res


_FilterFindFirstProto = WINFUNCTYPE(HRESULT, FILTER_INFORMATION_CLASS, c_void_p, DWORD, 
                                    LPDWORD, LPHANDLE)
_FilterFindFirstParamFlags = (0,  "dwInformationClass"), (1, "lpBuffer"), (0,  "dwBufferSize"), (1, "lpBytesReturned"), (1, "lpFilterFind")
_FilterFindFirst = _FilterFindFirstProto(("FilterFindFirst", windll.fltlib), 
                                          _FilterFindFirstParamFlags)

def FilterFindFirst(infocls=FILTER_INFORMATION_CLASS.FilterFullInformation):
    '''
    returns information about a filter driver (minifilter driver instance or 
    legacy filter driver) and is used to begin scanning the filters in the 
    global list of registered filters
    @param infocls The type of filter driver information requested.    
    @returns Handle to be utilized by FilterFindNext and FilterFindClose
    @returns filter_name this filters name
    '''
    BytesReturned = DWORD()
    ffi = FILTER_FULL_INFORMATION()
    hFilterFind = HANDLE(-1)

    res = HRESULT()

    try:
        res = _FilterFindFirst(infocls, byref(ffi), sizeof(ffi), byref(BytesReturned), 
                byref(hFilterFind))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        if lasterror == ERROR_NO_MORE_ITEMS:
            return None, None
        if lasterror != ERROR_INSUFFICIENT_BUFFER:       
            raise

    buf = create_string_buffer(BytesReturned.value)
    res = _FilterFindFirst(infocls, buf, sizeof(buf), byref(BytesReturned), byref(hFilterFind))

    if res != S_OK:
        return None, None

    sb = create_string_buffer(buf.raw)
    info = cast(sb, POINTER(FILTER_FULL_INFORMATION))        
    offset = type(info.contents).FilterNameBuffer.offset
    length = info.contents.FilterNameLength
    array_of_info = bytearray(memoryview(sb)[offset:length+offset])    
    slc = (BYTE * length).from_buffer(array_of_info)
    FilterName = "".join(map(chr, slc[::2]))
    
    fltfullinfo = FilterFullInformation(info.contents.FrameID, info.contents.NumberOfInstances, FilterName)
    
    return hFilterFind, fltfullinfo

_FilterFindNextProto = WINFUNCTYPE(HRESULT, HANDLE, FILTER_INFORMATION_CLASS, c_void_p, DWORD, LPDWORD)
_FilterFindNextParamFlags = (0,  "hFilterFind"), (0, "dwInformationClass"), (1, "lpBuffer"), (0,  "dwBufferSize"), (1, "lpBytesReturned")
_FilterFindNext = _FilterFindNextProto(("FilterFindNext", windll.fltlib), _FilterFindNextParamFlags)

def FilterFindNext(hFilterFind, infocls=FILTER_INFORMATION_CLASS.FilterFullInformation):
    '''
    continues a filter search started by a call to FilterFindFirst
    @param hFilterFind the filter search handle as returend by a previous call to FilterFindFirst
    @param infocls The type of filter driver information requested.    
    @returns filter_name this filters name
    '''
    BytesReturned = DWORD()
    ffi = FILTER_FULL_INFORMATION()
    res = HRESULT()

    try:
        res = _FilterFindNext(hFilterFind.value, infocls, byref(ffi), sizeof(ffi), byref(BytesReturned))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        if lasterror == ERROR_NO_MORE_ITEMS:
            return lasterror, None
        if lasterror != ERROR_INSUFFICIENT_BUFFER:       
            raise

    buf = create_string_buffer(BytesReturned.value)
    res = _FilterFindNext(hFilterFind, infocls, buf, sizeof(buf), byref(BytesReturned))

    if res != S_OK:
        return res, None

    sb = create_string_buffer(buf.raw)
    info = cast(sb, POINTER(FILTER_FULL_INFORMATION))
    
    offset = type(info.contents).FilterNameBuffer.offset
    length = info.contents.FilterNameLength
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    Filtername = "".join(map(chr, slc[::2]))
    
    fltfullinfo = FilterFullInformation(info.contents.FrameID, info.contents.NumberOfInstances, Filtername)  
    
    return res, fltfullinfo

_FilterFindCloseProto = WINFUNCTYPE(HRESULT, HANDLE, DWORD)
_FilterFindCloseParamFlags = (0,  "hFilterFind"), (0, "biff", 0)
_FilterFindClose = _FilterFindCloseProto(("FilterFindClose", windll.fltlib), _FilterFindCloseParamFlags)

def FilterFindClose(hFilterFind):
    '''
    closes the specified minifilter search handle
    @param hFilterFind the filter search handle as returend by a previous call to FilterFindFirst
    @returns HRESULT
    '''

    res = _FilterFindClose(hFilterFind.value)
    return res

_FilterCreateProto = WINFUNCTYPE(HRESULT, LPCWSTR, LPHANDLE)
_FilterCreateParamFlags = (0,  "lpFilterName"), (0, "hFilter", 1)
_FilterCreate = _FilterCreateProto(("FilterCreate", windll.fltlib), _FilterCreateParamFlags)

def FilterCreate(filter_name):
    '''
    closes the specified minifilter search handle
    @param filter_name the minifilters name
    @returns HRESULT, hFilter
    '''
    hFilter = HANDLE(-1)

    _res = _FilterCreate(filter_name, byref(hFilter))

    return hFilter


_FilterCloseProto = WINFUNCTYPE(HRESULT, LPHANDLE, DWORD)
_FilterCloseParamFlags = (0,  "hFilter"), (0, "biff", 0)
_FilterClose= _FilterCloseProto(("FilterClose", windll.fltlib), _FilterCloseParamFlags)

def FilterClose(hFilter):
    '''
    @note Do not use this function. As you might of noticed above, the close
    handle prototype is defined with two parameter.  This is an apparent bug with
    the ctypes runtime as single parameter functions are flagged as an
    error.  If you add a 'fake' parameter ('biff' in this case), then all seems to be well. 
    closes an open minifilter handle
    @param hFilter handle as returned by a previoius call to FilterCreate
    @returns HRESULT
    '''
    res = _FilterClose(hFilter)
    return res

_CloseHandleProto = WINFUNCTYPE(BOOL, HANDLE, DWORD)
_CloseHandleParamFlags = (0,  "hFilter"), (0, "biff", 0)
_CloseHandle= _FilterCloseProto(("CloseHandle", windll.kernel32), _CloseHandleParamFlags)

def CloseHandle(handle):
    '''    
    closes an operating system handle
    @note  As you might of noticed above, the close handle prototype is defined with two parameter.
    This is an apparent bug with the ctypes runtime as single parameter functions are flagged as an
    error.  If you add a 'fake' parameter ('biff' in this case), then all seems to be well.
    @param handle handle as returned by a previoius call supported create functions
    @returns True if suceeded else false
    '''
    success = _CloseHandle(handle)
    return success


def test_filter_instance():
    '''
    test user space filter instance manipulation
    '''
    stats = {}
    (handle, info,) = FilterFindFirst()
    (hFltInstFindFirst, info,) = FilterInstanceFindFirst(info.FilterName)
    if hFltInstFindFirst is not None:
        print(info)
    stats[info.FilterName] = 1
    while True:
        print("==================================================")
        print("Finding instances for filter {0}".format(info,))
        (hFltInstFindFirst, inst_info,) = FilterInstanceFindFirst(info.FilterName)
        if not hFltInstFindFirst:
            print("No Instances Defined for Filter {0}".format(info,))
            stats[info.FilterName] = 0
            (hr, info) = FilterFindNext(handle)
            if hr != S_OK:
                break
            stats[info.FilterName] = 1
            continue
        print(info)
        hFilter = FilterCreate(info.FilterName)
        print("FilterCreate({0}) returned {1}\n".format(info, hFilter,))
        res = CloseHandle(hFilter)
        print("CloseHandle({0}) returned {1}\n".format(hFilter, res,))        
        while True:
            (hr, inst_info,) = FilterInstanceFindNext(hFltInstFindFirst)
            if hr != S_OK:
                break
            stats[inst_info.FilterName] += 1
            print(inst_info)
        (hr, info) = FilterFindNext(handle)
        if hr != S_OK:
            break
        stats[info.FilterName] = 1
    _res = FilterInstanceFindClose(hFltInstFindFirst)
    _res = FilterFindClose(handle)
    print(stats)
    
    
def test_packet_decode():
    '''
    Test WinVirtUE packet decode
    '''
    MAXPKTSZ = 0x200  # max packet size
    (_res, hFltComms,) = FilterConnectCommunicationPort("\\WVUPort")
    while True:

        (_res, msg_pkt,) = FilterGetMessage(hFltComms, MAXPKTSZ)
        
        fmh = SaviorStruct.GetFilterMessageHeader(msg_pkt)
        response = ("Response to Message Id {0}\n".format(fmh.MessageId,))
        FilterReplyMessage(hFltComms, 0, fmh.MessageId, response)                

        import pdb;pdb.set_trace()        
        pdh = SaviorStruct.GetProbeDataHeader(fmh.Message)        
        if pdh.Type == DataType.LoadedImage:            
            msg_data = LoadedImageInfo.build(fmh.Message)
        elif pdh.Type == DataType.ProcessCreate:
            msg_data = ProcessCreateInfo.build(fmh.Message)
        elif pdh.Type == DataType.ProcessDestroy:
            msg_data = ProcessDestroyInfo.build(fmh.Message)
        else:
            print("Unknown or unsupported data type %s encountered\n" % (pdh.Type,))
            continue
        print("Decoded a %s data message\n" % (pdh.Type.name,))
        print(json.dumps(msg_data._asdict(), indent=4))

    CloseHandle(hFltComms)
    
    
def main():
    '''
    let's test some stuff
    '''
    
    test_packet_decode()
    
    #test_filter_instance()     
    
    sys.exit(0)
    
    

if __name__ == '__main__':
    main()
