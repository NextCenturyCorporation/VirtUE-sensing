'''
ntfltmgr.py - interface with the mini-port filter manager via python
'''
import sys
import json
import uuid
import logging
import curio

from enum import IntEnum, Flag
from collections import namedtuple
import ctypes
from ctypes import c_longlong, c_ulonglong, c_void_p, HRESULT, POINTER, Structure, Union
from ctypes import cast, create_string_buffer, byref, sizeof, WINFUNCTYPE, windll, memmove

from ctypes.wintypes import DWORD, LPCWSTR, LPDWORD, LPVOID, LPCVOID, BOOLEAN, CHAR
from ctypes.wintypes import LPHANDLE, ULONG, WCHAR, USHORT, WORD, HANDLE, BYTE, BOOL, LONG, UINT

CommandPort = "\\WVUCommand"
EventPort = "\\WVUPort"
S_OK = 0
MAXPKTSZ = 0x10000  # max packet size
MAXSENSORNAMESZ = 64
SIZE_T = c_ulonglong
NTSTATUS = DWORD
PVOID = c_void_p
ULONGLONG = c_ulonglong
LONGLONG = c_longlong

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

ERROR_INSUFFICIENT_BUFFER = 0x7a
ERROR_INVALID_PARAMETER = 0x57
ERROR_NO_MORE_ITEMS = 0x103
ERROR_IO_PENDING = 0x3e5

MAXRSPSZ = 0x1000
MAXCMDSZ = 0x1000

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
    def GetSensorDataHeader(cls, msg_pkt):
        '''
        accepts raw packet data from the driver and returns
        the SensorDataHeader in the form of a named tuple
        '''
        info = cast(msg_pkt, POINTER(SensorDataHeader))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.sensor_id.Data))
        pdh = GetSensorDataHeader(str(sensor_id),
                                 SensorType(info.contents.sensor_type),
                                 info.contents.DataSz,
                                 info.contents.CurrentGMT,
                                 msg_pkt)
        return pdh

    @staticmethod
    def LongLongToHex(num):
        '''
        convert a long long to a hex number
        '''
        return hex(0x10000000000000000 - abs(num))

    @staticmethod
    def DecodeString(slc):
        '''
        given a byte array slice, decode it
        '''
        ValueName = ''
        try:
            ValueName = bytes(slc).decode('utf-16')
        except UnicodeDecodeError as _err:
            try:
                ValueName = bytes(slc).decode('utf-8')
            except UnicodeDecodeError as _err:
                try:
                    ary = bytes(slc)
                    ValueName = "".join(map(chr, ary[::2]))
                except ValueError as _verr:
                    ValueName = "<Cannot Decode>"
        return ValueName

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

class PARAM_FLAG(Flag):
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
        ("UniqueProcess", ULONGLONG),
        ("UniqueThread", ULONGLONG)
    ]

class INSTANCE_INFORMATION_CLASS(CtypesEnum):
    '''
    filter instance information requested
    '''
    InstanceBasicInformation   = 0x0
    InstanceFullInformation    = 0x1
    InstancePartialInformation = 0x2
    InstanceAggregateStandardInformation = 0x3

class FILTER_INFORMATION_CLASS(CtypesEnum):
    '''
    filter driver information requested
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
        ("Internal", ULONGLONG),
        ("InternalHigh", ULONGLONG),
        ("Pointer", ULONGLONG),
        ("hEvent", ULONGLONG)
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

class KeyValueInformationClass(CtypesEnum):
    '''
    Key Value Informatoin Class
    '''
    KeyValueBasicInformation = 0
    KeyValueFullInformation = 1
    KeyValuePartialInformation = 2
    KeyValueFullInformationAlign64 = 3
    KeyValuePartialInformationAlign64 = 4
    KeyValueLayerInformation = 5
    MaxKeyValueInfoClass = 6  # MaxKeyValueInfoClass should always be the last enum

class RegNotifyClass(CtypesEnum):
    '''
    Hook selector for registry kernel mode callbacks
    '''
    RegNtDeleteKey = 0
    RegNtPreDeleteKey = RegNtDeleteKey
    RegNtSetValueKey = 1
    RegNtPreSetValueKey = RegNtSetValueKey
    RegNtDeleteValueKey = 2
    RegNtPreDeleteValueKey = RegNtDeleteValueKey
    RegNtSetInformationKey = 3
    RegNtPreSetInformationKey = RegNtSetInformationKey
    RegNtRenameKey = 4
    RegNtPreRenameKey = RegNtRenameKey
    RegNtEnumerateKey = 5
    RegNtPreEnumerateKey = RegNtEnumerateKey
    RegNtEnumerateValueKey = 6
    RegNtPreEnumerateValueKey = RegNtEnumerateValueKey
    RegNtQueryKey = 7
    RegNtPreQueryKey = RegNtQueryKey
    RegNtQueryValueKey = 8
    RegNtPreQueryValueKey = RegNtQueryValueKey
    RegNtQueryMultipleValueKey = 9
    RegNtPreQueryMultipleValueKey = RegNtQueryMultipleValueKey
    RegNtPreCreateKey = 10
    RegNtPostCreateKey = 11
    RegNtPreOpenKey = 12
    RegNtPostOpenKey = 13
    RegNtKeyHandleClose = 14
    RegNtPreKeyHandleClose = RegNtKeyHandleClose
    # .Net only
    RegNtPostDeleteKey = 15
    RegNtPostSetValueKey = 16
    RegNtPostDeleteValueKey = 17
    RegNtPostSetInformationKey = 18
    RegNtPostRenameKey = 19
    RegNtPostEnumerateKey = 20
    RegNtPostEnumerateValueKey = 21
    RegNtPostQueryKey = 22
    RegNtPostQueryValueKey = 23
    RegNtPostQueryMultipleValueKey = 24
    RegNtPostKeyHandleClose = 25
    RegNtPreCreateKeyEx = 26
    RegNtPostCreateKeyEx = 27
    RegNtPreOpenKeyEx = 28
    RegNtPostOpenKeyEx = 29
    # new to Windows Vista
    RegNtPreFlushKey = 30
    RegNtPostFlushKey = 31
    RegNtPreLoadKey = 32
    RegNtPostLoadKey = 33
    RegNtPreUnLoadKey = 34
    RegNtPostUnLoadKey = 35
    RegNtPreQueryKeySecurity = 36
    RegNtPostQueryKeySecurity = 37
    RegNtPreSetKeySecurity = 38
    RegNtPostSetKeySecurity = 39
    # per-object context cleanup
    RegNtCallbackObjectContextCleanup = 40
    # new in Vista SP2
    RegNtPreRestoreKey = 41
    RegNtPostRestoreKey = 42
    RegNtPreSaveKey = 43
    RegNtPostSaveKey = 44
    RegNtPreReplaceKey = 45
    RegNtPostReplaceKey = 46
    # new to Windows 10
    RegNtPreQueryKeyName = 47
    RegNtPostQueryKeyName = 48
    MaxRegNtNotifyClass = 49 # should always be the last enum

class KPROCESSOR_MODE(IntEnum):
    '''
    Processor Mode
    '''
    KernelMode = 0x0
    UserMode = 0x1

    @classmethod
    def from_param(cls, obj):
        '''
        convert to integer value
        '''
        return CHAR(obj)

class SensorType(CtypesEnum):
    '''
    Sensor Type of filter driver data to unpack
    '''
    NONE           = 0x0000
    ImageLoad    = 0x0001
    ProcessCreate  = 0x0002
    ProcessDestroy = 0x0003
    ThreadCreate   = 0x0004
    ThreadDestroy  = 0x0005
    ProcessListValidationFailed = 0x0006
    RegQueryValueKeyInfo = 0x0007
    RegCreateKeyInfo = 0x0008
    RegSetValueKeyInfo = 0x0009
    RegOpenKeyInfo = 0x000A
    RegDeleteValueKeyInfo = 0x000B
    RegRenameKeyInfo = 0x000C
    RegPostOperationInfo = 0x1001     # post operation handlers start at 0x1001

class SensorAttributeFlags(Flag):
    '''
    Sensor Attribute Flag Enumerations
    '''
    # No attributes
    NoAttributes = 0
    # Real Time Sensor that emits events as it happens
    RealTime = (1 << 1)
    # Emits events at regular time intervals
    Temporal = (1 << 2)
    # Indicates the sensor will start at driver load
    EnabledAtStart = (1 << 3)
    # Set if Sensor Type is Dynamic (Loaded by external operation)
    DynamicSensor = (1 << 4)

class RegObjectType(IntEnum):
    '''
    Registry Object Type
    '''
    RegNone = 0 # No value type
    RegSz = 1 # Unicode nul terminated string
    RegExpandSz = 2 # Unicode nul terminated string
    # (with environment variable references)
    RegBinary = 3 # Free form binary
    RegDWord = 4 # 32-bit number
    RegDWordLE = 4 # 32-bit number (same as REG_DWORD)
    RegDWordBE = 5 # 32-bit number
    RegLink = 6 # Symbolic Link (unicode)
    RegMultiSz = 7 # Multiple Unicode strings
    RegResourceList = 8 # Resource list in the resource map
    RegFullResourceDescriptor = 9 # Resource list in the hardware description
    RegResourceRequirementsList = 10
    RegQWord = 11 # 64-bit number
    RegQWordLE = 11 # 64-bit number (same as REG_QWORD)

GetUUID = namedtuple('GetUUID',  ['Data1', 'Data2', 'Data3', 'Data4'])
class UUID(SaviorStruct):
    '''
    UUID Data Structure Definition
    '''
    _fields_ = [
        ('Data1', UINT),
        ('Data2', USHORT),
        ('Data3', USHORT),
        ('Data4', BYTE * 8)
    ]


class _UUID(Union):
    '''
    Convenience Union for UUID
    '''
    _fields_ = [
        ('UUID', UUID),
        ('Data', BYTE * 16)
    ]
class LIST_ENTRY(SaviorStruct):
    '''
    Two Way Linked List - forward declare an incomplete type
    '''
LIST_ENTRY._fields_ = [
    ("space", BYTE * 16)
]

GetSensorDataHeader = namedtuple('GetSensorDataHeader',
        ['sensor_id', 'sensor_type', 'DataSz', 'CurrentGMT', 'Packet'])
class SensorDataHeader(SaviorStruct):
    '''
    Sensor Data Header
    '''
    _fields_ = [
        ('sensor_id', _UUID),
        ('sensor_type', USHORT),
        ('DataSz', USHORT),
        ('CurrentGMT', LONGLONG),
        ('ListEntry', LIST_ENTRY)
    ]


GetRegCreateKeyInfo = namedtuple('GetRegCreateKeyInfo',
    ['sensor_id', 'sensor_type', 'CurrentGMT',
    'ProcessId', 'EProcess',
    'RootObject', 'Options', 'SecurityDescriptor', 'SecurityQualityOfService',
    'DesiredAccess', 'GrantedAccess', 'Version', 'Wow64Flags',
    'Attributes', 'CheckAccessMode', 'CompleteName'])
class RegCreateKeyInfo(SaviorStruct):
    '''
    Sensor Data Header
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("RootObject", LONGLONG),
        ("Options", ULONG),
        ("SecurityDescriptor", LONGLONG),
        ("SecurityQualityOfService", LONGLONG),
        ("DesiredAccess", ULONG),
        ("GrantedAccess", ULONG),
        ("Version", LONGLONG),
        ("Wow64Flags", ULONG),
        ("Attributes", ULONG),
        ("CheckAccessMode", CHAR),
        ("CompleteNameSz", USHORT),
        ("CompleteName", BYTE * 1)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.CompleteNameSz
        offset = type(info.contents).CompleteName.offset
        sb = create_string_buffer(msg_pkt.Packet, len(msg_pkt.Packet))
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        CompleteName = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        key_nfo = GetRegCreateKeyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.RootObject),
            info.contents.Options,
            cls.LongLongToHex(info.contents.SecurityDescriptor),
            cls.LongLongToHex(info.contents.SecurityQualityOfService),
            info.contents.DesiredAccess,
            info.contents.GrantedAccess,
            cls.LongLongToHex(info.contents.Version),
            info.contents.Wow64Flags,
            info.contents.Attributes,
            KPROCESSOR_MODE(int(info.contents.CheckAccessMode[0])).name,
            CompleteName)
        return key_nfo

GetRegQueryValueKeyInfo = namedtuple('GetRegQueryValueKeyInfo',
        ['sensor_id', 'sensor_type', 'CurrentGMT',
        'ProcessId', 'EProcess', 'Class', 'Object', 'KeyValueInformationClass', 'ValueName'])
class RegQueryValueKeyInfo(SaviorStruct):
    '''
    Sensor Data Header
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("Class", UINT),
        ("_blank", UINT),
        ("Object", LONGLONG),
        ("KeyValueInformationClass", UINT),
        ("ValueNameLength", USHORT),
        ("ValueName", BYTE * 1)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.ValueNameLength
        offset = type(info.contents).ValueName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ValueName = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        key_info = GetRegQueryValueKeyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            RegNotifyClass(info.contents.Class).name,
            cls.LongLongToHex(info.contents.Object),
            KeyValueInformationClass(info.contents.KeyValueInformationClass).name,
            ValueName)
        return key_info


GetRegRenameKeyInfo = namedtuple('GetRegRenameKeyInfo',
                                ['sensor_id', 'sensor_type', 'CurrentGMT',
                                 'ProcessId', 'EProcess', 'Object', 'NewName'])
class RegRenameKeyInfo(SaviorStruct):
    '''
    Registry Rename Value Key
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("Object", LONGLONG),
        ("NewNameLength", USHORT),
        ("NewName", BYTE * 1)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.NewNameLength
        offset = type(info.contents).NewName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        NewName = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))

        key_info = GetRegRenameKeyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.Object),
            NewName)
        return key_info



GetRegPostOperationInfo = namedtuple('GetRegPostOperationInfo',
            ['sensor_id', 'sensor_type', 'CurrentGMT',
             'ProcessId', 'EProcess', 'Object',
             'Status', 'PreInformation', 'ReturnStatus'])
class RegPostOperationInfo(SaviorStruct):
    '''
    Registry Post Operations
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("Object", LONGLONG),
        ("Status", NTSTATUS),
        ("PreInformation", LONGLONG),
        ("ReturnStatus", NTSTATUS)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))

        key_info = GetRegPostOperationInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.Object),
            hex(info.contents.Status),
            cls.LongLongToHex(info.contents.PreInformation),
            hex(info.contents.ReturnStatus))
        return key_info

GetRegDeleteValueKeyInfo = namedtuple('GetRegDeleteValueKeyInfo',
                                      ['sensor_id', 'sensor_type', 'CurrentGMT',
                                    'ProcessId', 'EProcess', 'Object', 'ValueName'])
class RegDeleteValueKeyInfo(SaviorStruct):
    '''
    Registry Delete Value Key
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("Object", LONGLONG),
        ("ValueNameLength", USHORT),
        ("ValueName", BYTE * 1)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.ValueNameLength
        offset = type(info.contents).ValueName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ValueName = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))

        key_info = GetRegDeleteValueKeyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.Object),
            ValueName)
        return key_info

GetRegSetValueKeyInfo = namedtuple('GetRegSetValueKeyInfo',
             ['sensor_id', 'sensor_type', 'CurrentGMT',
              'ProcessId', 'EProcess', 'Object', 'Type', 'ValueName'])

class RegSetValueKeyInfo(SaviorStruct):
    '''
    Registry Set Value Key
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("Object", LONGLONG),
        ("Type", UINT),
        ("ValueNameLength", USHORT),
        ("ValueName", BYTE * 1)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.ValueNameLength
        offset = type(info.contents).ValueName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ValueName = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        obj_type_name = ''
        try:
            obj_type_name = RegObjectType(info.contents.Type).name
        except ValueError as _verr:
            obj_type_name = info.contents.Type

        key_info = GetRegSetValueKeyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.Object),
            obj_type_name,
            ValueName)
        return key_info

GetProcessListValidationFailed = namedtuple('ProcessListValidationFailed',
        ['sensor_id', 'sensor_type', 'CurrentGMT', 'Status',
            'ProcessId', 'EProcess'])

class ProcessListValidationFailed(SaviorStruct):
    '''
    Sensor Data Header
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("Status", NTSTATUS),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        process_list_validation_failed = GetProcessListValidationFailed(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.Status,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess))
        return process_list_validation_failed


GetImageLoadInfo = namedtuple('GetImageLoadInfo',
        ['sensor_id', 'sensor_type', 'CurrentGMT',
        'ProcessId', 'EProcess', 'ImageBase', 'ImageSize', 'FullImageName'])
class ImageLoadInfo(SaviorStruct):
    '''
    Sensor Data Header
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("ImageBase", LONGLONG),
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
        info = cast(msg_pkt.Packet, POINTER(cls))
        length = info.contents.FullImageNameSz
        offset = type(info.contents).FullImageName.offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        ModuleName = "".join(map(chr, slc[::2]))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        img_nfo = GetImageLoadInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.ImageBase),
            info.contents.ImageSize,
            ModuleName)
        return img_nfo

GetThreadCreateInfo = namedtuple('GetThreadCreateInfo',
                                  ['sensor_id', 'sensor_type', 'CurrentGMT',
         'ProcessId', 'ThreadId', 'Win32StartAddress', 'StartAddress','IsStartAddressValid'])
class ThreadCreateInfo(SaviorStruct ):
    '''
    ThreadCreateInfoInfo Definition
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("ThreadId", LONGLONG),
        ("Win32StartAddress", LONGLONG),
        ("StartAddress", LONGLONG),
        ("IsStartAddressValid", BOOLEAN)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        create_info = GetThreadCreateInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            info.contents.ThreadId,
            cls.LongLongToHex(info.contents.Win32StartAddress),
            cls.LongLongToHex(info.contents.StartAddress),
            info.contents.IsStartAddressValid)
        return create_info

GetThreadDestroyInfo = namedtuple('GetThreadDestroyInfo',
                                     ['sensor_id', 'sensor_type', 'CurrentGMT',
                                   'ProcessId', 'ThreadId'])
class ThreadDestroyInfo(SaviorStruct ):
    '''
    ThreadDestroyInfoInfo Definition
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("ThreadId", LONGLONG),
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        create_info = GetThreadDestroyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            info.contents.ThreadId,)
        return create_info

GetProcessCreateInfo = namedtuple('GetProcessCreateInfo',
        ['sensor_id', 'sensor_type', 'CurrentGMT',
        'ParentProcessId', 'ProcessId', 'EProcess', 'UniqueProcess',
        'UniqueThread', 'FileObject', 'CreationStatus', 'CommandLineSz',
        'CommandLine'])

class WVU_COMMAND(CtypesEnum):
    '''
    Winvirtue Commands
    '''
    Echo = 0x0
    EnableProbe  = 0x1
    DisableProbe = 0x2
    EnableUnload = 0x3
    DisableUnload = 0x4
    EnumerateSensors = 0x5
    ConfigureSensor = 0x6
    OneShotKill = 0x7

class WVU_RESPONSE(CtypesEnum):
    '''
    Winvirtue Command Responses
    '''
    NORESPONSE = 0x0
    WVUSuccess = 0x1
    WVUFailure = 0x2

GetCommandMessage = namedtuple('GetCommandMessage',  ['Command', 'sensor_id', 'DataSz', 'Data'])

class COMMAND_MESSAGE(SaviorStruct):
    '''
    Command Message as sent to the driver
    '''
    _fields_ = [
        ("Command", ULONG),
        ("sensor_id", _UUID),
        ("DataSz", SIZE_T),
        ("Data", BYTE * 1)
    ]

    @classmethod
    def build(cls, cmd, sensor_id, data):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        sb = create_string_buffer(sizeof(cls) + len(data) - 1)
        info = cast(sb, POINTER(cls))
        info.contents.Command = cmd
        memmove(info.contents.sensor_id, sensor_id.bytes,
                len(info.contents.sensor_id.Data))
        info.contents.DataSz = len(data)
        if info.contents.DataSz > 0:
            length = info.contents.DataSz
            offset = type(info.contents).Data.offset
            sb[offset:length] = data

        command_packet = GetCommandMessage(
            info.contents.Command,
            info.contents.sensor_id,
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

class SensorStatusHeader(SaviorStruct):
    '''
    The sensor status message header
    '''
    _fields_ = [
        ("NumberOfEntries", DWORD)
    ]


GetSensorStatus = namedtuple('GetSensorStatus',
                             ['sensor_id', 'LastRunTime', 'RunInterval',
         'OperationCount', 'Attributes', 'Enabled', 'SensorName'])
class SensorStatus(SaviorStruct):
    '''
    The SensorStatus message
    '''
    _fields_ = [
        ("sensor_id", _UUID),
        ("LastRunTime", LONGLONG),
        ("RunInterval", LONGLONG),
        ("OperationCount", LONG),
        ("Attributes", USHORT),
        ("Enabled", BOOLEAN),
        ("SensorName", BYTE * MAXSENSORNAMESZ),
    ]

    saf_name = "SensorAttributeFlags."

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt, POINTER(cls))
        length = MAXSENSORNAMESZ
        offset = type(info.contents).SensorName.offset
        sb = create_string_buffer(msg_pkt)
        array_of_chars = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_chars)
        lst = [ch for ch in slc if ch]
        SensorName = "".join(map(chr, lst))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.sensor_id.Data))
        attrib_flags = str(SensorAttributeFlags(info.contents.Attributes))
        ndx = attrib_flags.find(cls.saf_name)
        if ndx == 0:
            attrib_flags = attrib_flags[len(cls.saf_name):]

        sensor_status = GetSensorStatus(
            sensor_id,
            info.contents.LastRunTime,
            info.contents.RunInterval,
            info.contents.OperationCount,
            attrib_flags,
            str(bool(info.contents.Enabled)),
            SensorName)
        return sensor_status

class ProcessCreateInfo(SaviorStruct ):
    '''
    ProcessCreateInfo Definition
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ParentProcessId", LONGLONG),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG),
        ("CreatingThreadId", CLIENT_ID),
        ("FileObject", LONGLONG),
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
        offset = type(info.contents).CommandLine.offset
        length = msg_pkt.DataSz - offset
        sb = create_string_buffer(msg_pkt.Packet)
        array_of_info = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_info)
        CommandLine = cls.DecodeString(slc)
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        create_info = GetProcessCreateInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ParentProcessId,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess),
            cls.LongLongToHex(info.contents.CreatingThreadId.UniqueProcess),
            cls.LongLongToHex(info.contents.CreatingThreadId.UniqueThread),
            cls.LongLongToHex(info.contents.FileObject),
            info.contents.CreationStatus,
            info.contents.CommandLineSz,
            CommandLine)
        return create_info

GetProcessDestroyInfo = namedtuple('GetProcessDestroyInfo',
        ['sensor_id', 'sensor_type', 'CurrentGMT',
         'ProcessId', 'EProcess'])

class ProcessDestroyInfo(SaviorStruct):
    '''
    ProcessDestroyInfo Definition
    '''
    _fields_ = [
        ("Header", SensorDataHeader),
        ("ProcessId", LONGLONG),
        ("EProcess", LONGLONG)
    ]

    @classmethod
    def build(cls, msg_pkt):
        '''
        build named tuple instance representing this
        classes instance data
        '''
        info = cast(msg_pkt.Packet, POINTER(cls))
        sensor_id = uuid.UUID(bytes=bytes(info.contents.Header.sensor_id.Data))
        info.contents.EProcess = (0 if not info.contents.EProcess
                else info.contents.EProcess)
        print(info.contents.EProcess)
        create_info = GetProcessDestroyInfo(
            str(sensor_id),
            SensorType(info.contents.Header.sensor_type).name,
            info.contents.Header.CurrentGMT,
            info.contents.ProcessId,
            cls.LongLongToHex(info.contents.EProcess))
        return create_info



_FilterReplyMessageProto = WINFUNCTYPE(HRESULT, HANDLE, POINTER(FILTER_REPLY_HEADER), DWORD)
_FilterReplyMessageParamFlags = (0, "hPort"), (0,  "lpReplyBuffer"), (0, "dwReplyBufferSize")
_FilterReplyMessage = _FilterReplyMessageProto(("FilterReplyMessage", windll.fltlib), _FilterReplyMessageParamFlags)
def FilterReplyMessage(hPort, status, msg_id, msg, msg_len):
    '''
    replies to a message from a kernel-mode minifilter
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
        logger.exception("FilterReplyMessage Failed on Message Reply - Error %d", lasterror, exc_info=True)
        res = lasterror

    if res:
        logging.error("_FilterReplyMessage: %d", res)
    return res


# HACK: This variable is used in the following function. It's global to
# convince Python to check its value on every access
# (think "volatile, Python-style").
info = None

_FilterGetMessageProto = WINFUNCTYPE(HRESULT, HANDLE, POINTER(FILTER_MESSAGE_HEADER), DWORD, POINTER(OVERLAPPED))
_FilterGetMessageParamFlags = (0, "hPort"), (0,  "lpMessageBuffer"), (0, "dwMessageBufferSize"), (0,  "lpOverlapped", 0)
_FilterGetMessage = _FilterGetMessageProto(("FilterGetMessage", windll.fltlib), _FilterGetMessageParamFlags)
async def FilterGetMessage(hPort, msg_len):
    '''
    Get a message from filter driver.
    @param hPort the port handle from the driver we are communicating with
    @param msg_len the message length that we expect to receive
    @returns hPort Port Handle

    This is a coroutine and implements a crude form of polling. The call to
    _FilterGetMessage() won't block, but if there's no data available it
    will sleep and check the output buffer (see the global "info" above).
    '''
    res = HRESULT()
    # passed to _FilterGetMessage(), which can update it asynchronously
    global info

    if msg_len is None or not isinstance(msg_len, int) or msg_len <= 0:
        raise ValueError("Parameter MsgBuf is invalid!")

    # poverlap = cast(None, POINTER(OVERLAPPED)) # do this if we want to block

    # zeroed overlapped struct causes _FilterGetMessage() to return immediately
    poverlap = ctypes.pointer(OVERLAPPED())

    sb = create_string_buffer(msg_len)
    info = cast(sb, POINTER(FILTER_MESSAGE_HEADER))
    try:
        logger.debug("Awaiting message in FilterGetMessage()")
        res = _FilterGetMessage(hPort, byref(info.contents), msg_len, poverlap)
        if res:
            logging.error("_FilterGetMessage(): %d\n", res)
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        if lasterror == ERROR_IO_PENDING:
            # No data is available. Wait for it to show up.
            while not info.contents.ReplyLength:
                await curio.sleep(0.1)
        else:
            logger.exception("OSError: FilterGetMessage failed to Get Message - Error %d", lasterror, exc_info=True)
            raise
    except Exception as exc:
        logger.exception("Exception: FilterGetMessage failed to Get Message - Error %r", repr(exc), exc_info=True)
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
    _res = HRESULT()

    try:
        _res = _FilterConnectCommunicationPort(PortName, 0, None, 0, None, byref(hPort))
    except OSError as osr:
        lasterror = osr.winerror & 0x0000FFFF
        logger.exception("Failed to connect to port %s error %d", PortName, lasterror, exc_info=True)
        raise
    return hPort

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
    InstanceName = bytes(slc).decode('utf-16')

    length = info.contents.AltitudeLength
    offset = info.contents.AltitudeBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    Altitude = bytes(slc).decode('utf-16')

    length = info.contents.VolumeNameLength
    offset = info.contents.VolumeNameBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    VolumeName = bytes(slc).decode('utf-16')

    length = info.contents.FilterNameLength
    offset = info.contents.FilterNameBufferOffset
    array_of_info = memoryview(sb)[offset:length+offset]
    slc = (BYTE * length).from_buffer(array_of_info)
    FilterName = bytes(slc).decode('utf-16')

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
        logger.exception("FilterSendMessage Failed on Message Reply - Error %s", str(osr), exc_info=True)
        res = lasterror

    bufsz = (bytes_returned.value
            if bytes_returned.value < MAXRSPSZ
            else MAXRSPSZ)
    response = create_string_buffer(rsp_buf.raw[0:bufsz], bufsz)
    return res, response


def _packet_decode_header(msg_pkt):
    pdh = SaviorStruct.GetSensorDataHeader(msg_pkt.Remainder)
    if pdh.sensor_type == SensorType.ImageLoad:
        msg_data = ImageLoadInfo.build(pdh)
    elif pdh.sensor_type == SensorType.ProcessCreate:
        msg_data = ProcessCreateInfo.build(pdh)
    elif pdh.sensor_type == SensorType.ProcessDestroy:
        msg_data = ProcessDestroyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.ProcessListValidationFailed:
        msg_data = ProcessListValidationFailed.build(pdh)
    elif pdh.sensor_type == SensorType.RegQueryValueKeyInfo:
        msg_data = RegQueryValueKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegCreateKeyInfo:
        msg_data = RegCreateKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegSetValueKeyInfo:
        msg_data = RegSetValueKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegOpenKeyInfo:
        msg_data = RegCreateKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegDeleteValueKeyInfo:
        msg_data = RegDeleteValueKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegRenameKeyInfo:
        msg_data = RegRenameKeyInfo.build(pdh)
    elif pdh.sensor_type == SensorType.RegPostOperationInfo:
        msg_data = RegPostOperationInfo.build(pdh)
    elif pdh.sensor_type == SensorType.ThreadCreate:
        msg_data = ThreadCreateInfo.build(pdh)
    elif pdh.sensor_type == SensorType.ThreadDestroy:
        msg_data = ThreadDestroyInfo.build(pdh)
    else:
        logger.warning("Unknown or unsupported sensor type %s encountered\n", pdh.sensor_type)
        msg_data = None

    return msg_data

def packet_decode():
    '''
    Synchronous WinVirtUE packet decode. Broken since FilterGetMessage is now async.
    '''
    hFltComms = FilterConnectCommunicationPort(EventPort)
    while True:
        (_res, msg_pkt,) = FilterGetMessage(hFltComms, MAXPKTSZ)
        response = ("Response to Message Id {0}\n".format(msg_pkt.MessageId,))
        FilterReplyMessage(hFltComms, 0, msg_pkt.MessageId, response, msg_pkt.ReplyLength)
        if _res:
            continue
        msg_data = _packet_decode_header(msg_pkt)
        if not msg_data:
            continue
        yield msg_data._asdict()

    CloseHandle(hFltComms)

async def apacket_decode():
    """
    Async packet generator. Unfortunately a Windows handle can't be
    converted to a waitable object.
    """
    hFltComms = FilterConnectCommunicationPort(EventPort)

    while True:
        (_res, msg_pkt,) = await FilterGetMessage(hFltComms, MAXPKTSZ)
        response = ("Response to Message Id {0}\n".format(msg_pkt.MessageId,))
        FilterReplyMessage(hFltComms, 0, msg_pkt.MessageId, response, msg_pkt.ReplyLength)
        msg_data = _packet_decode_header(msg_pkt)
        if not msg_data:
            continue
        yield msg_data._asdict()

    CloseHandle(hFltComms)


def EnumerateSensors(hFltComms, Filter=None):
    '''
    Enumerate Sensors
    @note by default, all sensors are enumerated and returned
    '''

    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))

    cmd_msg.contents.Command = WVU_COMMAND.EnumerateSensors
    cmd_msg.contents.DataSz = 0

    _res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    header = cast(rsp_buf, POINTER(SensorStatusHeader))
    cnt = header.contents.NumberOfEntries
    sb = create_string_buffer(rsp_buf[sizeof(SensorStatusHeader):])
    length = sizeof(SensorStatus)

    for ndx in range(0, cnt):
        offset = ndx * sizeof(SensorStatus)
        array_of_bytes = memoryview(sb)[offset:length+offset]
        slc = (BYTE * length).from_buffer(array_of_bytes)
        sensor = SensorStatus.build(bytes(slc))
        yield sensor

def Echo(hFltComms):
    '''
    Send and receive an echo sensor
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))

    cmd_msg.contents.Command = WVU_COMMAND.Echo
    cmd_msg.contents.DataSz = 0

    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def EnableProbe(hFltComms, sensor_id=0):
    '''
    Enable Given or All Probes
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))

    cmd_msg.contents.Command = WVU_COMMAND.EnableProbe
    cmd_msg.contents.DataSz = 0
    memmove(cmd_msg.contents.sensor_id.Data, sensor_id.bytes,
            len(cmd_msg.contents.sensor_id.Data))

    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def DisableProbe(hFltComms, sensor_id=0):
    '''
    Disable Given or All Probes
    '''
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE))
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))

    cmd_msg.contents.Command = WVU_COMMAND.DisableProbe
    cmd_msg.contents.DataSz = 0
    memmove(cmd_msg.contents.sensor_id.Data, sensor_id.bytes,
            len(cmd_msg.contents.sensor_id.Data))

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

def DisableUnload(hFltComms):
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


def OneShotKill(hFltComms, pid):
    '''
    Kill a specific Process
    '''
    pid_str = ("{0}".format(str(pid),)).encode()
    length = len(pid_str)
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE) + length)
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))

    cmd_msg.contents.Command = WVU_COMMAND.OneShotKill
    cmd_msg.contents.DataSz = length

    offset = type(cmd_msg.contents).Data.offset
    ary = memoryview(cmd_buf)[offset:offset + len(pid_str)]
    slc = (BYTE * len(pid_str)).from_buffer(ary)
    fit = min(len(pid_str), len(slc))
    memmove(slc, pid_str, fit)

    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def ConfigureSensor(hFltComms, cfgdata, sensor_id=0):
    '''
    Configure a specific sensor with the provided
    configuration data
    '''
    if cfgdata is None or not hasattr(cfgdata, "__len__") or len(cfgdata) <= 0:
        raise ValueError("Parameter cfgdata is invalid!")

    length = len(cfgdata)
    cmd_buf = create_string_buffer(sizeof(COMMAND_MESSAGE) + length)
    cmd_msg = cast(cmd_buf, POINTER(COMMAND_MESSAGE))
    cmd_msg.contents.Command = WVU_COMMAND.ConfigureSensor
    memmove(cmd_msg.contents.sensor_id.Data,
            sensor_id.bytes, len(sensor_id.bytes))
    cmd_msg.contents.DataSz = length
    offset = type(cmd_msg.contents).Data.offset
    ary = memoryview(cmd_buf)[offset:offset + length]
    slc = (BYTE * length).from_buffer(ary)
    data = cfgdata.encode('utf-8')
    fit = min(length, len(slc))
    memmove(slc, data, fit)
    res, rsp_buf = FilterSendMessage(hFltComms, cmd_buf)
    rsp_msg = cast(rsp_buf, POINTER(RESPONSE_MESSAGE))

    return res, rsp_msg

def test_command_response():
    '''
    Test WinVirtUE command response
    '''
    hFltComms = FilterConnectCommunicationPort(CommandPort)
    try:
        #sensors = EnumerateSensors(hFltComms)
        #print("res = {0}\n".format(res,))
        for sensor in EnumerateSensors(hFltComms):
            print("{0}".format(sensor,))
            ConfigureSensor(hFltComms,'{"repeat-interval": 60}', sensor.sensor_id)
    finally:
        CloseHandle(hFltComms)

async def test_packet_decode():
    '''
    test the packet decode
    '''
    async for packet in apacket_decode():
        print(json.dumps(packet, indent=3))

def main(argv):
    '''
    let's test some stuff
    '''
    if len(argv) == 2:
        hFltComms = FilterConnectCommunicationPort(CommandPort)
        try:
            (res, resp_msg,) = OneShotKill(hFltComms, int(argv[1]))
            print("_res={0},resp_msg={1}\n".format(res,resp_msg,))
        finally:
            CloseHandle(hFltComms)

    test_command_response()

    while True:
        curio.run(test_packet_decode, with_monitor=True)

    sys.exit(0)



if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    main(sys.argv)

