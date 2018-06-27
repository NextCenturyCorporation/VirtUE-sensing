'''
ntfltmgr.py - interface with the mini-port filter manager via python
'''
import sys
import json
import logging
from enum import IntEnum
from collections import namedtuple
from ctypes import c_int, c_longlong, c_ulonglong, c_void_p, HRESULT, POINTER, Structure
from ctypes import cast, create_string_buffer, byref, sizeof, WINFUNCTYPE, windll

from ctypes.wintypes import WPARAM, DWORD, LPCWSTR, LPDWORD, LPVOID, LPCVOID
from ctypes.wintypes import LPHANDLE, ULONG, WCHAR, USHORT, WORD, HANDLE, BYTE, BOOL

S_OK = 0

SIZE_T = WPARAM
NTSTATUS = DWORD
PVOID = c_void_p
ULONGLONG = c_ulonglong
LONGLONG = c_longlong

logger = logging.getLogger("ntfltmgr")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

ERROR_INSUFFICIENT_BUFFER = 0x7a
ERROR_INVALID_PARAMETER = 0x57
ERROR_NO_MORE_ITEMS = 0x103

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
        info = cast(msg_pkt, POINTER(ProbeDataHeader))        
        pdh = GetProbeDataHeader(DataType(info.contents.ProbeId), 
                                 info.contents.DataSz, 
                                 info.contents.CurrentGMT,
                                 msg_pkt)
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
    ProbeId of filter instance information requested
    '''
    InstanceBasicInformation   = 0x0
    InstanceFullInformation    = 0x1
    InstancePartialInformation = 0x2
    InstanceAggregateStandardInformation = 0x3

class FILTER_INFORMATION_CLASS(CtypesEnum):
    '''
    ProbeId of filter driver information requested
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
        ("Internal", c_ulonglong),
        ("InternalHigh", c_ulonglong),
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

FilterReplyHeader = namedtuple('FilterReplyHeader', ['Status', 'MessageId', 'Remainder'])    
    
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
    
FilterMessageHeader = namedtuple('FilterMessageHeader', ['ReplyLength', 'MessageId', 'Remainder'])
        

class DataType(CtypesEnum):
    '''
    ProbeId of filter driver data to unpack
    '''
    NONE           = 0x0000
    LoadedImage    = 0x0001
    ProcessCreate  = 0x0002
    ProcessDestroy = 0x0003
    ThreadCreate   = 0x0004
    ThreadDestroy  = 0x0005        
    ProcessListValidationFailed = 0x0006
    
class LIST_ENTRY(SaviorStruct):
    '''
    Two Way Linked List - forward declare an incomplete type
    '''
LIST_ENTRY._fields_ = [
    ("space", BYTE * 16)
]


GetProbeDataHeader = namedtuple('GetProbeDataHeader',  
        ['ProbeId', 'DataSz', 'CurrentGMT', 'Packet'])
    
class ProbeDataHeader(SaviorStruct):
    '''
    Probe Data Header
    '''
    _fields_ = [
        ('ProbeId', USHORT),
        ('DataSz', USHORT),
        ('CurrentGMT', LONGLONG),
        ('ListEntry', LIST_ENTRY)
    ]


GetProcessListValidationFailed = namedtuple('ProcessListValidationFailed',
        ['ProbeId', 'DataSz', 'CurrentGMT', 'Status', 'ProcessId', 'EProcess'])
class ProcessListValidationFailed(SaviorStruct):
    '''
    Probe Data Header
    '''
    _fields_ = [
        ("Header", ProbeDataHeader),
        ("Status", NTSTATUS),
        ("ProcessId", HANDLE),
        ("EProcess", PVOID)
    ]
    
    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        process_list_validation_failed = GetProcessListValidationFailed(
            info.contents.ReportId,           
            info.contents.Status,
            info.contents.ProcessId,
            info.contents.EProcess)
        return process_list_validation_failed

    
GetLoadedImageInfo = namedtuple('GetLoadedImageInfo',  
        ['ProbeId', 'DataSz', 'CurrentGMT', 
        'ProcessId', 'EProcess', 'ImageBase', 'ImageSize', 'FullImageName'])
class LoadedImageInfo(SaviorStruct):
    '''
    Probe Data Header
    '''    
    _fields_ = [
        ("Header", ProbeDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("ImageBase", LONGLONG),
        ("ImageSize", ULONGLONG),
        ("FullImageNameSz", USHORT),
        ("FullImageName", BYTE * 1)
    ]
    
    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''        
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.FullImageNameSz
        offset = type(info.contents).FullImageName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ModuleName = "".join(map(chr, slc[::2]))
        img_nfo = GetLoadedImageInfo(
            DataType(info.contents.Header.ProbeId).name,
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            info.contents.EProcess,
            info.contents.ImageBase, 
            info.contents.ImageSize,
            ModuleName)
        return img_nfo


GetProcessCreateInfo = namedtuple('GetProcessCreateInfo',  
        ['ProbeId', 'DataSz', 'CurrentGMT', 
        'ParentProcessId', 'ProcessId', 'EProcess', 'UniqueProcess', 
        'UniqueThread', 'FileObject', 'CreationStatus', 'CommandLineSz', 
        'CommandLine'])
    
class ProcessCreateInfo(SaviorStruct ):
    '''
    ProcessCreateInfo Definition
    '''
    _fields_ = [
        ("Header", ProbeDataHeader),
        ("ParentProcessId", HANDLE),
        ("ProcessId", HANDLE),
        ("EProcess", PVOID),
        ("CreatingThreadId", CLIENT_ID),
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
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.CommandLineSz
        offset = type(info.contents).CommandLine.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        CommandLine = "".join(map(chr, slc[::2]))
        create_info = GetProcessCreateInfo(
            DataType(info.contents.Header.ProbeId).name,
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ParentProcessId,
            info.contents.ProcessId,
            info.contents.EProcess,
            info.contents.CreatingThreadId.UniqueProcess,
            info.contents.CreatingThreadId.UniqueThread,
            info.contents.FileObject,
            info.contents.CreationStatus,
            info.contents.CommandLineSz,
            CommandLine)
        return create_info

GetProcessDestroyInfo = namedtuple('GetProcessDestroyInfo',  
        ['ProbeId', 'DataSz', 'CurrentGMT', 
         'ProcessId', 'EProcess'])
    
class ProcessDestroyInfo(SaviorStruct):
    '''
    ProcessDestroyInfo Definition
    '''
    _fields_ = [
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
        info = cast(msg_pkt.Packet, POINTER(cls))
        create_info = GetProcessDestroyInfo(
            DataType(info.contents.Header.ProbeId).name,
            info.contents.Header.DataSz,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            info.contents.EProcess)
        return create_info
    

_FilterReplyMessageProto = WINFUNCTYPE(HRESULT, HANDLE, POINTER(FILTER_REPLY_HEADER), DWORD)
_FilterReplyMessageParamFlags = (0, "hPort"), (0,  "lpReplyBuffer"), (0, "dwReplyBufferSize")
_FilterReplyMessage = _FilterReplyMessageProto(("FilterReplyMessage", windll.fltlib), _FilterReplyMessageParamFlags)
def FilterReplyMessage(hPort, status, msg_id, msg, msg_len):
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
        
    if (msg is None or not hasattr(msg, "__len__") or len(msg) <= 0
        or msg_len < len(msg) or msg_len <= sizeof(FILTER_REPLY_HEADER)):
        raise ValueError("Parameter msg or msg_len is invalid!")
    
    if isinstance(msg, str):
        txt = msg
        msg = bytearray()
        msg.extend(map(ord, txt))                     
    reply_buffer = create_string_buffer(bytes(msg), msg_len)
    info = cast(reply_buffer, POINTER(FILTER_REPLY_HEADER))    
    info.contents.Status = status
    info.contents.MessageId = msg_id           

    try:
        res = _FilterReplyMessage(hPort, byref(info.contents), msg_len)
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
        res = _FilterGetMessage(hPort, byref(info.contents), msg_len, 
                cast(None, POINTER(OVERLAPPED)))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        logger.exception("OSError: FilterGetMessage failed to Get Message - Error %d", lasterror)
        raise
    except Exception as exc:
        print("Exception: FilterGetMessage failed to Get Message - Error %s" % (exc,))
        raise

    ReplyLen = info.contents.ReplyLength
    MessageId = info.contents.MessageId
    msg_pkt = FilterMessageHeader(ReplyLen, MessageId,
            sb[sizeof(FILTER_MESSAGE_HEADER):])
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


_FilterInstanceFindClose = windll.fltlib.FilterInstanceFindClose
_FilterInstanceFindClose.argtypes = [HANDLE]
_FilterInstanceFindClose.rettype = HRESULT
def FilterInstanceFindClose(hFilterInstanceFind):
    '''
    @brief closes the specified minifilter instance search handle
    @param hFilterInstanceFind the filter search handle as returend by a 
    previous call to FilterInstanceFindFirst
    @returns HRESULT
    '''

    res = _FilterInstanceFindClose(hFilterInstanceFind.value)
    return res


_FilterFindFirstProto = WINFUNCTYPE(HRESULT, FILTER_INFORMATION_CLASS, 
        c_void_p, DWORD, LPDWORD, LPHANDLE)
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

_FilterFindClose = windll.fltlib.FilterFindClose
_FilterFindClose.argtypes = [HANDLE]
_FilterFindClose.rettype = HRESULT
def FilterFindClose(hFilterFind):
    '''
    @brief closes the specified minifilter search handle
    @param hFilterFind the filter search handle as returend by a previous call to FilterFindFirst
    @returns HRESULT
    '''

    res = _FilterFindClose(hFilterFind.value)
    return res

_FilterCreateProto = WINFUNCTYPE(HRESULT, LPCWSTR, LPHANDLE)
_FilterCreateParamFlags = (0,  "lpFilterName"), (0, "hFilter", 0)
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


_FilterClose = windll.fltlib.FilterClose
_FilterClose.argtypes = [LPHANDLE]
_FilterClose.rettype = HRESULT
def FilterClose(hFilter):
    '''
    @param hFilter handle as returned by a previoius call to FilterCreate
    @returns HRESULT
    '''
    res = _FilterClose(hFilter)
    return res

_CloseHandle = windll.kernel32.CloseHandle
_CloseHandle.argtypes = [HANDLE]
_CloseHandle.rettype = BOOL
def CloseHandle(handle):
    '''    
    closes an operating system handle
    @param handle handle as returned by a previoius call supported create functions
    @returns True if suceeded else false
    '''
    success = _CloseHandle(handle)
    return success


def test_packet_decode():
    '''
    Test WinVirtUE packet decode
    '''
    MAXPKTSZ = 0x400  # max packet size
    (_res, hFltComms,) = FilterConnectCommunicationPort("\\WVUPort")
    while True:
        (_res, msg_pkt,) = FilterGetMessage(hFltComms, MAXPKTSZ)
        response = ("Response to Message Id {0}\n".format(msg_pkt.MessageId,))
        print(response)
        FilterReplyMessage(hFltComms, 0, msg_pkt.MessageId, response, msg_pkt.ReplyLength)
        pdh = SaviorStruct.GetProbeDataHeader(msg_pkt.Remainder)
        if pdh.ProbeId == DataType.LoadedImage:            
            msg_data = LoadedImageInfo.build(pdh)
        elif pdh.ProbeId == DataType.ProcessCreate:
            msg_data = ProcessCreateInfo.build(pdh)
        elif pdh.ProbeId == DataType.ProcessDestroy:
            msg_data = ProcessDestroyInfo.build(pdh)
        elif pdh.ProbeId == DataType.ProcessListValidationFailed:
            msg_data = ProcessListValidationFailed.build(pdh)
        else:
            print("Unknown or unsupported data type %s encountered\n" % (pdh.ProbeId,))
            continue
        print(json.dumps(msg_data._asdict(), indent=4))

    CloseHandle(hFltComms)
  
class WVU_COMMAND(CtypesEnum):
    '''
    Winvirtue Commands
    '''
    Echo = 0x0
    EnableProtection  = 0x1
    DisableProtection = 0x2        
    EnableUnload = 0x3
    DisableUnload = 0x4
    EnumerateProbes = 0x5
    ConfigureProbe = 0x6
    
class WVU_RESPONSE(CtypesEnum):
    '''
    Winvirtue Command Responses
    '''
    NORESPONSE = 0x0
    WVUSuccess = 0x1
    WVUFailure = 0x2

GetCommandMessage = namedtuple('GetCommandMessage',  ['Command', 'DataSz', 'Data'])
    
class COMMAND_MESSAGE(SaviorStruct):
    '''
    Command Message as sent to the driver
    '''
    _fields_ = [        
        ("Command", ULONG),
        ("DataSz", SIZE_T),
        ("Data", BYTE * 1) 
    ]

    @classmethod
    def build(cls, cmd, data):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        sb = create_string_buffer(sizeof(cls) + len(data) - 1)
        info = cast(sb, POINTER(cls))
        info.contents.Command = cmd
        info.contents.DataSz = len(data)        
        if info.contents.DataSz > 0:
            length = info.contents.DataSz
            offset = type(info.contents).Data.offset
            sb[offset:length] = data
            
        command_packet = GetCommandMessage(
            info.contents.Command,
            info.contents.DataSz,
            sb)
        return command_packet  

GetResponseMessage = namedtuple('GetResponseMessage',  ['Response', 'Status', 'DataSz', 'Data'])

class RESPONSE_MESSAGE(SaviorStruct):
    '''
    Response Message as receieved from driver
    '''
    _fields_ = [        
        ("Response", ULONG),
        ("Status", NTSTATUS),
        ("DataSz", SIZE_T),
        ("Data", BYTE * 1) 
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        sb = ''
        info = cast(msg_pkt.Packet, POINTER(cls))
        if info.contents.DataSz > 0:
            length = info.contents.DataSz
            offset = type(info.contents).Data.offset
            sb = create_string_buffer(msg_pkt.Packet[offset:length])        
        command_packet = GetCommandMessage(
            info.contents.Size,
            info.contents.Command,
            sb)
        return command_packet     

MAXRSPSZ = 0x1000
MAXCMDSZ = 0x1000
    
_FilterSendMessageProto = WINFUNCTYPE(HRESULT, HANDLE, LPVOID, DWORD, LPVOID, DWORD, LPDWORD)
_FilterSendMessageParamFlags = (0, "hPort"), (0,  "lpInBuffer"), (0, "dwInBufferSize"), (1, "lpOutBuffer"), (0, "dwOutBufferSize"), (1, "lpBytesReturned")
_FilterSendMessage = _FilterSendMessageProto(("FilterSendMessage", windll.fltlib), _FilterSendMessageParamFlags)
def FilterSendMessage(hPort, cmd_buf):
    '''    
    @brief The FilterSendMessage function sends a message to a kernel-mode minifilter. 
    @param hPort port handle returned by a previous call to 
    FilterConnectCommunicationPort. This parameter is required and cannot be NULL.
    @param cmd_buf command buffer
    @returns S_OK if successful. Otherwise, it returns an error value
    @returns response buffer - can be empty / zero length
    '''    
    res = HRESULT()
    bytes_returned = DWORD()
    rsp_buf = create_string_buffer(MAXRSPSZ)
        
    if cmd_buf is None or not hasattr(cmd_buf, "__len__") or len(cmd_buf) <= 0:
        raise ValueError("Parameter cmd_buf is invalid!")

    try:
        res = _FilterSendMessage(hPort, cmd_buf, len(cmd_buf), byref(rsp_buf), len(rsp_buf), byref(bytes_returned))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        print(osr)
        logger.exception("FilterSendMessage Failed on Message Reply - Error %s", str(osr))
        res = lasterror    
    
    bufsz = (bytes_returned.value 
            if bytes_returned.value < MAXRSPSZ 
            else MAXRSPSZ)
    response = create_string_buffer(rsp_buf.raw[0:bufsz], bufsz)
    return res, response

def Echo(hFltComms):
    '''
    Send and receive an echo probe
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.Echo
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def EnableProtection(hFltComms):
    '''
    Enable Full Protection
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.EnableProtection
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg
    
def DisableProtection(hFltComms):
    '''
    Disable Full Protection
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.DisableProtection
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg
    
def EnableUnload(hFltComms):
    '''
    @note this functionality might not function on release targets
    Enable Driver Unload 
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.EnableUnload
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg
    
def DisbleUnload(hFltComms):
    '''
    @note this functionality might not function on release targets
    Disable Driver Unload 
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.DisableUnload
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg
    
def EnumerateProbes(hFltComms, Filter=None):
    '''
    Enumerate Probes
    @note by default, all probes are enumerated and returned
    '''

    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.EnumerateProbes
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def ConfigureProbe(hFltComms, ProbeName, ConfigurationData):
    '''
    Configure a specific probe with the provided 
    configuration data
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))          
    
    cmd_msg.contents.Command = WVU_COMMAND.ConfigureProbe
    cmd_msg.contents.DataSz = 0
    
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def test_command_response():
    '''
    Test WinVirtUE command response
    '''
    import pdb;pdb.set_trace()

    (res, hFltComms,) = FilterConnectCommunicationPort("\\WVUCommand")

    (res, rsp_msg,) = DisableProtection(hFltComms)
    print("_res={0}, bytes returned={1}, Response={2}, Status={3}\n"
          .format(_res, len(rsp_buf), rsp_msg.contents.Response, 
              rsp_msg.contents.Status))

    (res, rsp_msg,) = EnableProtection(hFltComms)
    print("_res={0}, bytes returned={1}, Response={2}, Status={3}\n"
          .format(_res, len(rsp_buf), rsp_msg.contents.Response, 
              rsp_msg.contents.Status))

    CloseHandle(hFltComms)    
      
def main():
    '''
    let's test some stuff
    '''
    test_command_response()
    
    test_packet_decode()  
    
    sys.exit(0)
    
    

if __name__ == '__main__':        
    main()
