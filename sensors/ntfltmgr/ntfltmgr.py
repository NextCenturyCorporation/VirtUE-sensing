'''
ntfltmgr.py - interface with the mini-port filter manager via python
'''
import sys
import logging
from enum import IntEnum
from collections import namedtuple
from ctypes import c_int, c_uint, c_long, c_ulong, c_int8, c_uint8, c_int16, c_uint16, c_int32, c_uint32, c_int64, c_uint64, c_longlong, c_ulonglong, c_void_p
from ctypes import c_float, c_double, c_longdouble
from ctypes import cast, create_string_buffer, addressof, HRESULT, POINTER, GetLastError, cdll, byref, sizeof, Structure, WINFUNCTYPE, windll, create_unicode_buffer

from ctypes.wintypes import DWORD, LPCWSTR, LPDWORD, LPHANDLE, ULONG, WCHAR, USHORT, HANDLE, BYTE, BOOL

S_OK = 0

logger = logging.getLogger("ntfltmgr")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

class SaviorStruct(Structure):
    '''
    Implement a base class that takes care of JSON serialization/deserialization, 
    internal object instance state and other housekeeping chores
    '''

    def __init__(self):
        '''
        Initialize this instance
        '''
        pass

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

class CtypesEnum(IntEnum):
    '''
    A ctypes-compatible IntEnum superclass
    '''
    @classmethod
    def from_param(cls, obj):
        return int(obj)

class PARAM_FLAG(CtypesEnum):
    '''
    Parameter Flag Enumerations
    '''
    FIN   = (1 << 0)
    FOUT  = (1 << 1)
    FLCID = (1 << 2)
    COMBINED = FIN | FOUT | FLCID

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

ERROR_INSUFFICIENT_BUFFER = 0x7a
ERROR_INVALID_PARAMETER = 0x57
ERROR_NO_MORE_ITEMS = 0x103

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
    filter_name = "".join(map(chr, slc[::2]))
    return hFilterFind, filter_name

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
    array_of_info = bytearray(memoryview(sb)[offset:length+offset])
    slc = (BYTE * length).from_buffer(array_of_info)
    filter_name = "".join(map(chr, slc[::2]))
    return S_OK, filter_name

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
    @note Do not use this function.  
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
    @param handle handle as returned by a previoius call supported create functions
    @returns True if suceeded else false
    '''
    success = _CloseHandle(handle)
    return success

def main(argv):
    '''
    let's test some stuff
    '''
    stats = {}
    (handle, name,) = FilterFindFirst()
    (hFltInstFindFirst, info,) = FilterInstanceFindFirst(name)
    if hFltInstFindFirst is not None:
        print(info)
    stats[name] = 1
    while True:

        print("==================================================")
        print("Finding instances for filter {0}".format(name,))
        (hFltInstFindFirst, info,) = FilterInstanceFindFirst(name)
        hFilter = FilterCreate(name)
        print("FilterCreate({0}) returned {1}\n".format(name, hFilter,))
        res = CloseHandle(hFilter)
        print("CloseHandle({0}) returned {1}\n".format(hFilter, res,))
        if(not hFltInstFindFirst):
            print("No Instances Defined for Filter {0}".format(name,))
            stats[name] = 0
            (hr, name) = FilterFindNext(handle)
            if hr != S_OK:
                break
            stats[name] = 1
            continue
        print(info)
        while True:
            hr, info = FilterInstanceFindNext(hFltInstFindFirst)
            if hr != S_OK:
                break
            stats[name] += 1
            print(info)
        (hr, name) = FilterFindNext(handle)
        if hr != S_OK:
            break
        stats[name] = 1

    _res = FilterInstanceFindClose(hFltInstFindFirst)
    _res = FilterFindClose(handle)
    print(stats)




if __name__ == '__main__':
    main(sys.argv)



