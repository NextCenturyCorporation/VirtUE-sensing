'''
proc_info.py - provides the various window sensors data including process, threads, handles, and other operating system object information
'''
import sys
from enum import IntEnum

from ctypes import c_bool, c_char_p, c_wchar_p, c_void_p, c_ushort, c_short, c_size_t, c_byte, c_ubyte, c_char, c_wchar
from ctypes import c_int, c_uint, c_long, c_ulong, c_int8, c_uint8, c_int16, c_uint16, c_int32, c_uint32, c_int64, c_uint64, c_longlong, c_ulonglong
from ctypes import c_float, c_double, c_longdouble
from ctypes import cast, create_string_buffer, addressof, POINTER, GetLastError, cdll, byref, sizeof, Structure, WINFUNCTYPE, windll
from json import JSONEncoder, JSONDecoder, dumps
from ctypes.wintypes import HANDLE, ULONG, PULONG, LONG, LARGE_INTEGER, BYTE, BOOLEAN
import pywintypes
from ntsecuritycon import SE_SECURITY_NAME, SE_CREATE_PERMANENT_NAME, SE_DEBUG_NAME
from win32con import SE_PRIVILEGE_ENABLED
from win32api import OpenProcess, DuplicateHandle, GetCurrentProcess
from win32security import LookupPrivilegeValue, OpenProcessToken, AdjustTokenPrivileges, TOKEN_ALL_ACCESS

#pylint: disable=C0303


PROCESS_DUP_HANDLE = 0x0040
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
     
class CtypesEnum(IntEnum):
    """A ctypes-compatible IntEnum superclass."""
    @classmethod
    def from_param(cls, obj):
        return int(obj)

class SmartStructure(Structure):
    '''
    Implement a base class that takes care of JSON serialization/deserialization, 
    internal object instance state and other housekeeping chores
    '''
    pointer_types = [c_void_p]
    string_types = [c_char_p, c_wchar_p]
    integer_types = [c_bool, c_byte, c_ubyte, c_short,  c_ushort, c_short, c_size_t, c_byte, c_ubyte, c_char, c_wchar,
                    c_int, c_uint, c_long, c_ulong, c_int8, c_uint8, c_int16, c_uint16, c_int32, c_uint32, c_int64, c_uint64, c_longlong, c_ulonglong]
    float_types = [c_float, c_double, c_longdouble]
    
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
    
    def ToDict(self):
        '''
        returns a dictionary object representative of this object instance internal state
        '''
        instance = {}
        szfields = len(type(self)._fields_)        
        for ndx in range(0, szfields):            
            this_name = type(self)._fields_[ndx][0]
            this_value = getattr(self, this_name)                           
            instance[this_name] = this_value   
        return instance        
    
    def Encode(self):
        '''
        Encode an instance of this class into a JSON representation
        '''
        encoder = JSONEncoder()
        result = {}
        anonymous = getattr(self, '_anonymous_', [])        
        clazz = getattr(self, '__class__', None)        
        result['__class__'] = clazz.__name__
        for _name, _type in getattr(self, '_fields_', []):
            value = getattr(self, _name)            
            # don't Encode private fields
            if _name.startswith('_'):
                continue
            if _name in anonymous:
                result.update(encoder.encode(value))
            else:
                try:
                    result[_name] = encoder.encode(value)
                except TypeError as _te:
                    value = getattr(self,_name)
                    if isinstance(value, UNICODE_STRING):
                        result[_name] = value.Buffer
                    print("value={0}\n".format(value,))                        
                    print("result={0}\n".format(result,))                        
        encoded_data = dumps(result, indent=4)
        return encoded_data
    
    @classmethod                  
    def Decode(clz, obj):
        '''
        Decode a JSON representation into an instance of this class
        '''
        if not issubclass(clz, SmartStructure) or not isinstance(obj, str):
            return None
        
        decoder = JSONDecoder()
        data = decoder.decode(obj)
        clazzname = data["__class__"]
        if clazzname != clz.__name__:
            raise TypeError("{0} != {1} - class or type mismatch on deserialization!"
                            .format(clazzname, clz.__name__,))
        clazz = clz()
        for _name, _type in clazz._fields_:            
            value_as_a_string = data[_name]
            if _type in SmartStructure.integer_types:              
                value = _type(int(value_as_a_string))
            elif _type in SmartStructure.float_types:
                value = _type(float(value_as_a_string))
            elif _type in SmartStructure.pointer_types:
                if value_as_a_string in ["null","None"]:
                    value = _type(int(0))
                else:
                    value = _type(int(value_as_a_string))                
            elif _type in SmartStructure.string_types:
                value = value_as_a_string
            else:
                value = _type()

            setattr(clazz, _name, value)
            value = getattr(clazz, _name)        
        return clazz    
    
class UNICODE_STRING(SmartStructure):
    _fields_ = [ 
        ("Length", c_ushort),
        ("MaximumLength", c_ushort),
        ("Buffer", c_wchar_p)
        ]

class PARAM_FLAG(CtypesEnum):
    '''
    Parameter Flag Enumerations
    '''
    FIN   = (1 << 0)
    FOUT  = (1 << 1)
    FLCID = (1 << 2)
    COMBINED = FIN | FOUT | FLCID
    
class OBJECT_INFORMATION(CtypesEnum):
    '''
    Object Information Enumeration
    '''
    ObjectBasicInformation = 0x0000
    ObjectNameInformation = 0x0001
    ObjectTypeInformation = 0x0002 
    
class SYSTEM_INFORMATION_CLASS(CtypesEnum):
    SystemBasicInformation=0x0000,
    SystemProcessorInformation=0x0001
    SystemPerformanceInformation=0x0002
    SystemTimeOfDayInformation=0x0003
    SystemPathInformation=0x0004
    SystemProcessInformation=0x0005
    SystemCallCountInformation=0x0006
    SystemDeviceInformation=0x0007
    SystemProcessorPerformanceInformation=0x0008
    SystemFlagsInformation=0x0009
    SystemCallTimeInformation=0x000A
    SystemModuleInformation=0x000B
    SystemLocksInformation=0x000C
    SystemStackTraceInformation=0x000D
    SystemPagedPoolInformation=0x000E
    SystemNonPagedPoolInformation=0x000F
    SystemHandleInformation=0x0010
    SystemObjectInformation=0x0011
    SystemPageFileInformation=0x0012
    SystemVdmInstemulInformation=0x0013
    SystemVdmBopInformation=0x0014
    SystemFileCacheInformation=0x0015
    SystemPoolTagInformation=0x0016
    SystemInterruptInformation=0x0017
    SystemDpcBehaviorInformation=0x0018
    SystemFullMemoryInformation=0x0019
    SystemLoadGdiDriverInformation=0x001A
    SystemUnloadGdiDriverInformation=0x001B
    SystemTimeAdjustmentInformation=0x001C
    SystemSummaryMemoryInformation=0x001D
    SystemMirrorMemoryInformation=0x001E
    SystemPerformanceTraceInformation=0x001F
    SystemCrashDumpInformation=0x0020
    SystemExceptionInformation=0x0021
    SystemCrashDumpStateInformation=0x0022
    SystemKernelDebuggerInformation=0x0023
    SystemContextSwitchInformation=0x0024
    SystemRegistryQuotaInformation=0x0025
    SystemExtendServiceTableInformation=0x0026
    SystemPrioritySeperation=0x0027
    SystemVerifierAddDriverInformation=0x0028
    SystemVerifierRemoveDriverInformation=0x0029
    SystemProcessorIdleInformation=0x002A
    SystemLegacyDriverInformation=0x002B
    SystemCurrentTimeZoneInformation=0x002C
    SystemLookasideInformation=0x002D
    SystemTimeSlipNotification=0x002E
    SystemSessionCreate=0x002F
    SystemSessionDetach=0x0030
    SystemSessionInformation=0x0031
    SystemRangeStartInformation=0x0032
    SystemVerifierInformation=0x0033
    SystemVerifierThunkExtend=0x0034
    SystemSessionProcessInformation=0x0035
    SystemLoadGdiDriverInSystemSpace=0x0036
    SystemNumaProcessorMap=0x0037
    SystemPrefetcherInformation=0x0038
    SystemExtendedProcessInformation=0x0039
    SystemRecommendedSharedDataAlignment=0x003A
    SystemComPlusPackage=0x003B
    SystemNumaAvailableMemory=0x003C
    SystemProcessorPowerInformation=0x003D
    SystemEmulationBasicInformation=0x003E
    SystemEmulationProcessorInformation=0x003F
    SystemExtendedHandleInformation=0x0040
    SystemLostDelayedWriteInformation=0x0041
    SystemBigPoolInformation=0x0042
    SystemSessionPoolTagInformation=0x0043
    SystemSessionMappedViewInformation=0x0044
    SystemHotpatchInformation=0x0045
    SystemObjectSecurityMode=0x0046
    SystemWatchdogTimerHandler=0x0047
    SystemWatchdogTimerInformation=0x0048
    SystemLogicalProcessorInformation=0x0049
    SystemWow64SharedInformationObsolete=0x004A
    SystemRegisterFirmwareTableInformationHandler=0x004B
    SystemFirmwareTableInformation=0x004C
    SystemModuleInformationEx=0x004D
    SystemVerifierTriageInformation=0x004E
    SystemSuperfetchInformation=0x004F
    SystemMemoryListInformation=0x0050
    SystemFileCacheInformationEx=0x0051
    SystemThreadPriorityClientIdInformation=0x0052
    SystemProcessorIdleCycleTimeInformation=0x0053
    SystemVerifierCancellationInformation=0x0054
    SystemProcessorPowerInformationEx=0x0055
    SystemRefTraceInformation=0x0056
    SystemSpecialPoolInformation=0x0057
    SystemProcessIdInformation=0x0058
    SystemErrorPortInformation=0x0059
    SystemBootEnvironmentInformation=0x005A
    SystemHypervisorInformation=0x005B
    SystemVerifierInformationEx=0x005C
    SystemTimeZoneInformation=0x005D
    SystemImageFileExecutionOptionsInformation=0x005E
    SystemCoverageInformation=0x005F
    SystemPrefetchPatchInformation=0x0060
    SystemVerifierFaultsInformation=0x0061
    SystemSystemPartitionInformation=0x0062
    SystemSystemDiskInformation=0x0063
    SystemProcessorPerformanceDistribution=0x0064
    SystemNumaProximityNodeInformation=0x0065
    SystemDynamicTimeZoneInformation=0x0066
    SystemCodeIntegrityInformation=0x0067
    SystemProcessorMicrocodeUpdateInformation=0x0068
    SystemProcessorBrandString=0x0069
    SystemVirtualAddressInformation=0x006A
    SystemLogicalProcessorAndGroupInformation=0x006B
    SystemProcessorCycleTimeInformation=0x006C
    SystemStoreInformation=0x006D
    SystemRegistryAppendString=0x006E
    SystemAitSamplingValue=0x006F
    SystemVhdBootInformation=0x0070
    SystemCpuQuotaInformation=0x0071
    SystemNativeBasicInformation=0x0072
    SystemErrorPortTimeouts=0x0073
    SystemLowPriorityIoInformation=0x0074
    SystemBootEntropyInformation=0x0075
    SystemVerifierCountersInformation=0x0076
    SystemPagedPoolInformationEx=0x0077
    SystemSystemPtesInformationEx=0x0078
    SystemNodeDistanceInformation=0x0079
    SystemAcpiAuditInformation=0x007A
    SystemBasicPerformanceInformation=0x007B
    SystemQueryPerformanceCounterInformation=0x007C
    SystemSessionBigPoolInformation=0x007D
    SystemBootGraphicsInformation=0x007E
    SystemScrubPhysicalMemoryInformation=0x007F
    SystemBadPageInformation=0x0080
    SystemProcessorProfileControlArea=0x0081
    SystemCombinePhysicalMemoryInformation=0x0082
    SystemEntropyInterruptTimingInformation=0x0083
    SystemConsoleInformation=0x0084
    SystemPlatformBinaryInformation=0x0085
    SystemThrottleNotificationInformation=0x0086
    SystemHypervisorProcessorCountInformation=0x0087
    SystemDeviceDataInformation=0x0088
    SystemDeviceDataEnumerationInformation=0x0089
    SystemMemoryTopologyInformation=0x008A
    SystemMemoryChannelInformation=0x008B
    SystemBootLogoInformation=0x008C
    SystemProcessorPerformanceInformationEx=0x008D
    SystemSpare0=0x008E
    SystemSecureBootPolicyInformation=0x008F
    SystemPageFileInformationEx=0x0090
    SystemSecureBootInformation=0x0091
    SystemEntropyInterruptTimingRawInformation=0x0092
    SystemPortableWorkspaceEfiLauncherInformation=0x0093
    SystemFullProcessInformation=0x0094
    MaxSystemInfoClass=0x0095
    

class CLIENT_ID(SmartStructure):
    '''
    The CLIENT ID Structure
    '''
    _fields_ = [
        ("UniqueProcess", c_void_p),
        ("UniqueThread", c_void_p)
        ]

class SYSTEM_THREAD_INFORMATION(SmartStructure):
    '''
    The SYSTEM_THREAD_INFORMATION Structure
    '''
    _fields_ = [
        ("KernelTime", LARGE_INTEGER),
        ("UserTime", LARGE_INTEGER),
        ("CreateTime", LARGE_INTEGER),
        ("WaitTime", ULONG),
        ("StartAddress", c_void_p),
        ("ClientId", CLIENT_ID),
        ("Priority", LONG),
        ("BasePriority", LONG),
        ("ContextSwitches", ULONG),
        ("ThreadState", ULONG),
        ("WaitReason", ULONG),
        ]    

class SYSTEM_HANDLE_FLAGS(CtypesEnum):
    PROTECT_FROM_CLOSE = 1
    INHERIT = 2    
    
class SYSTEM_HANDLE_TABLE_ENTRY_INFO(SmartStructure):
    _fields_ = [ 
        ("UniqueProcessId", c_ushort),
        ("CreatorBackTraceIndex", c_ushort),
        ("ObjectTypeIndex", c_ubyte),
        ("HandleAttributes", c_ubyte),
        ("HandleValue", c_ushort),
        ("Object", c_void_p),
        ("GrantedAccess", ULONG)
        ]    
class SYSTEM_HANDLE_INFORMATION(SmartStructure):
    _fields_ = [ 
        ("NumberOfHandles", ULONG),
        ("Handles", SYSTEM_HANDLE_TABLE_ENTRY_INFO)
        ]      

class SYSTEM_PROCESS_INFORMATION(SmartStructure):
    _fields_ = [
        ("NextEntryOffset", ULONG),
        ("NumberOfThreads", ULONG),        
        ("WorkingSetPrivate", LARGE_INTEGER),
        ("HardFaultCount", ULONG),
        ("NumberOfThreadsHighWatermark", ULONG),
        ("CycleTime", c_ulonglong),
        ("CreateTime", LARGE_INTEGER),
        ("UserTime", LARGE_INTEGER),
        ("KernelTime", LARGE_INTEGER),        
        ("ImageName", UNICODE_STRING),
        ("BasePriority", LONG),
        ("UniqueProcessId", HANDLE),
        ("InheritedFromUniqueProcessId", HANDLE),
        ("HandleCount", ULONG),
        ("SessionId", ULONG),
        ("UniqueProcessKey", c_void_p),
        ("PeakVirtualSize", c_void_p),
        ("VirtualSize", c_void_p),
        ("PageFaultCount", ULONG),
        ("PeakWorkingSetSize", c_void_p),
        ("WorkingSetSize", c_void_p),
        ("QuotaPeakPagedPoolUsage", c_void_p),
        ("QuotaPagedPoolUsage", c_void_p),        
        ("QuotaPeakNonPagedPoolUsage", c_void_p),
        ("QuotaNonPagedPoolUsage", c_void_p),        
        ("PagefileUsage", c_void_p),
        ("PeakPagefileUsage", c_void_p),        
        ("PrivatePageCount", c_size_t),
        ("ReadOperationCount", LARGE_INTEGER), 
        ("WriteOperationCount", LARGE_INTEGER), 
        ("OtherOperationCount", LARGE_INTEGER), 
        ("ReadTransferCount", LARGE_INTEGER), 
        ("WriteTransferCount", LARGE_INTEGER), 
        ("OtherTransferCount", LARGE_INTEGER)
        ]

class SYSTEM_BASIC_INFORMATION(SmartStructure):
    _fields_ = [
        ("Reserved", ULONG),
        ("TimerResolution", ULONG),
        ("PageSize", ULONG),
        ("NumberOfPhysicalPages", ULONG),
        ("LowestPhysicalPageNumber", ULONG),
        ("HighestPhysicalPageNumber", ULONG),
        ("AllocationGranularity", ULONG),
        ("MinimumUserModeAddress", c_void_p),
        ("MaximumUserModeAddress", c_void_p),         
        ("ActiveProcessorsAffinityMask", c_void_p),
        ("NumberOfProcessors", BYTE)
        ]
                           
    
class GENERIC_MAPPING(SmartStructure):
    _fields_ = [("GenericRead", ULONG),
                ("GenericWrite", ULONG),
                ("GenericExecute", ULONG),
                ("GenericAll", ULONG)
                ]               
    
class OBJECT_TYPE_INFORMATION(SmartStructure):
    _fields_ = [("Name", UNICODE_STRING ),
                ("TotalNumberOfObjects", ULONG),
                ("TotalNumberOfHandles", ULONG),
                ("TotalNonPagedPoolUsage", ULONG),                
                ("TotalNamePoolUsage", ULONG),
                ("TotalHandleTableUsage", ULONG),
                ("HighWaterNumberOfObjects", ULONG),
                ("HighWaterNumberOfHandles", ULONG),
                ("HighWaterPagedPoolUsage", ULONG),
                ("HighWaterNonPagedPoolUsage", ULONG),
                ("HighWaterNamePoolUsage", ULONG),
                ("HighWaterHandleTableUsage", ULONG),
                ("InvalidAttributes", ULONG),                
                ("GenericMapping", GENERIC_MAPPING ),                
                ("ValidAccessMask", ULONG),
                ("SecurityRequired", BOOLEAN ),        
                ("MaintainHandleCount", BOOLEAN),
                ("PoolType", ULONG),
                ("DefaultPagedPoolCharge", ULONG),
                ("DefaultNonPagedPoolCharge", ULONG)
                ]     
class SaviorJSONEncoder(JSONEncoder):
    '''
    Encode Savior Objects in JSON
    '''
    def default(self, obj):
        '''
        Handles non-standard object types for Savior
        '''
        if isinstance(obj, UNICODE_STRING):
            return [obj.Length, obj.MaximumLength, obj.Buffer]
        # Let the base class default method raise the TypeError
        return JSONEncoder.default(self, obj)

STATUS_SUCCESS=0x0
STATUS_INFO_LENGTH_MISMATCH=0xc0000004
STATUS_INVALID_PARAMETER=0xC000000D
STATUS_BUFFER_TOO_SMALL=0xC0000023
STATUS_BUFFER_OVERFLOW=0x80000005
    
prototype = WINFUNCTYPE(LONG, SYSTEM_INFORMATION_CLASS, c_void_p, ULONG, PULONG, use_last_error=True)
paramflags = (PARAM_FLAG.FIN, "SystemInformationClass"), (PARAM_FLAG.FIN, "SystemInformation"), (PARAM_FLAG.FIN, "SystemInformationLength"), (PARAM_FLAG.FOUT, "ReturnLength")
NtQuerySystemInformation = prototype(("NtQuerySystemInformation", windll.ntdll), paramflags)

def _change_privileges(disable_privilege, privs):
    '''
    change privilges common code
    '''
    bSuccess = False
    hProcToken = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS)
    try:
        if not hProcToken:
            return None            
        bSuccess = AdjustTokenPrivileges(hProcToken,  disable_privilege , privs)    
    finally:
        if not not hProcToken:
            hProcToken.Close()
    return bSuccess        

def acquire_privileges(privs):
    '''
    attempt to acquire privileges
    '''
    bSuccess = _change_privileges(False, privs)
    return bSuccess

def release_privileges(privs):
    '''
    attempt to release previously acquired privileges
    '''
    bSuccess = _change_privileges(True, privs)
    return bSuccess

def get_thread_objects(proc_obj, spi_ary):
    '''
    Give a process object and the process information array yield thread objects for this 
    process object
    '''    
    SystemThreadInfo = POINTER(SYSTEM_THREAD_INFORMATION)
    sz_sti = sizeof(SYSTEM_THREAD_INFORMATION)    
    sz_thd_buf = proc_obj.NumberOfThreads * sz_sti                                
    begin = 0
    
    for n in range(0, proc_obj.NumberOfThreads):
        buf = (BYTE * sz_sti).from_buffer_copy(spi_ary[begin:begin + sz_sti])
        spt = cast(buf, SystemThreadInfo)
        yield spt.contents        
        begin = begin + sz_sti

def get_process_objects(pid=None):
    '''
    process object iterator
    '''        
    return_length = ULONG()            
    sysprocinfo = SYSTEM_PROCESS_INFORMATION()
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemProcessInformation,
                                                byref(sysprocinfo),
                                                sizeof(sysprocinfo),
                                                byref(return_length))
    
    buf = create_string_buffer(return_length.value)
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemProcessInformation,
                                                buf,
                                                sizeof(buf),
                                                byref(return_length))
    if STATUS_SUCCESS != res:
        return None
    
    SystemProcInfo = POINTER(SYSTEM_PROCESS_INFORMATION)    
    sz_sti = sizeof(SYSTEM_THREAD_INFORMATION)        
    sz_spi = sizeof(SYSTEM_PROCESS_INFORMATION)
    array_of_spi = bytearray(memoryview(buf))    
    
    begin = 0
    end = sz_spi
    while True:
        buf = (BYTE * sz_spi).from_buffer_copy(array_of_spi[begin:end])
        spi = cast(buf, SystemProcInfo)                                       
        sz_thd_data = end + (spi.contents.NumberOfThreads * sz_sti)
        if not not pid and isinstance(pid, int) and pid == spi.contents.UniqueProcessId:            
            yield spi.contents, array_of_spi[end:sz_thd_data]            
            break
        elif not pid:
            yield spi.contents, array_of_spi[end:sz_thd_data]
        next_entry_offset = spi.contents.NextEntryOffset
        if(0 == next_entry_offset):
            break        
        begin = begin + next_entry_offset
        end = begin + sz_spi        

def _get_process_handle(pid):
    '''
    Given a PID return its process handle
    '''
    if not pid or not isinstance(pid, int) or 0 != pid % 4:
        return None    
                   
    process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)   
    if not process_handle:    
        return None               
    
    IsCritical = BOOLEAN()
    success = cdll.kernel32.IsProcessCritical(process_handle.handle, byref(IsCritical))
    process_handle.close()
    del process_handle
    if not success:        
        return None            
    if True == IsCritical:        
        return None  
    
    try:
        process_handle = OpenProcess(PROCESS_DUP_HANDLE, False, pid)        
    except pywintypes.error as err:
        lasterr = GetLastError()
        print("Error opening PID {0} Last Error {1} - {2}\n".format(pid, lasterr, err,))
        process_handle = None
               
    return process_handle  
def get_process_ids():
    '''
    process id iterator
    '''        
    return_length = ULONG()            
    sysprocinfo = SYSTEM_PROCESS_INFORMATION()
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemProcessInformation,
                                                byref(sysprocinfo),
                                                sizeof(sysprocinfo),
                                                byref(return_length))
    
    buf = create_string_buffer(return_length.value)
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemProcessInformation,
                                                buf,
                                                sizeof(buf),
                                                byref(return_length))
    if STATUS_SUCCESS != res:
        return None
    
    SystemProcInfo = POINTER(SYSTEM_PROCESS_INFORMATION)
    sz_spi = sizeof(SYSTEM_PROCESS_INFORMATION)
    array_of_spi = bytearray(memoryview(buf))
    
    begin = 0
    end = sz_spi
    pid = 0
    while True:
        buf = (BYTE * sz_spi).from_buffer_copy(array_of_spi[begin:end])
        spi = cast(buf, SystemProcInfo)
        if not spi.contents.UniqueProcessId:
            pid = 0
        else:
            pid = spi.contents.UniqueProcessId                           
        yield pid
        next_entry_offset = spi.contents.NextEntryOffset
        if(0 == next_entry_offset):
            break        
        begin = begin + next_entry_offset
        end = begin + sz_spi        


def get_system_handle_information(pid=None):
    '''
    System Handle Iterator with pid filter
    '''
    
    if not pid or not isinstance(pid, int) or 0 != pid % 4:
        return None
    
    return_length = ULONG()            
    syshdnfo = SYSTEM_HANDLE_INFORMATION()
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemHandleInformation,
                                                byref(syshdnfo),
                                                sizeof(syshdnfo),
                                                byref(return_length))
    
    buf = create_string_buffer(return_length.value)
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemHandleInformation,
                                                buf,
                                                sizeof(buf),
                                                byref(return_length))
    if STATUS_SUCCESS != res:
        return None

    SysHdlNfo = POINTER(SYSTEM_HANDLE_INFORMATION)
    HdlEntry = POINTER(SYSTEM_HANDLE_TABLE_ENTRY_INFO)

    SystemHandleInfo = cast(buf, SysHdlNfo)
    
    sz_shtei = sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO)
    initial_offset = addressof(SystemHandleInfo.contents.Handles) - addressof(SystemHandleInfo.contents) 
    array_of_shtei = bytearray(memoryview(buf)[initial_offset:])
    
    for ndx in range(0, SystemHandleInfo.contents.NumberOfHandles): 
        begin = sz_shtei * ndx
        end = begin + sz_shtei
        buf = (BYTE * sz_shtei).from_buffer_copy(array_of_shtei[begin:end])
        shi = cast(buf, HdlEntry)
        if not not pid and isinstance(pid, int) and pid == shi.contents.UniqueProcessId:
            yield shi.contents
            break
        elif not pid:
            yield shi.contents
        else:
            continue    

def get_handle_information(pid=None):
    '''
    syste handle iterator with a pid filter
    '''    
    process_handle = _get_process_handle(pid)
    if not process_handle:
        return []
    
    current_process_handle = GetCurrentProcess()

    try:
        for shi in get_system_handle_information(pid):
            try:            
                duped_handle = DuplicateHandle(process_handle, shi.HandleValue, current_process_handle, PROCESS_DUP_HANDLE, False, 0)                
            except pywintypes.error as _err:
                pass
            else:
                yield duped_handle                
    finally:
        process_handle.close()

def _get_object_info(handle, obj_info_enum):
    '''
    common code to retrieve either object type or object information
    '''
    if (not handle 
        or not hasattr(handle, "handle") 
        or not isinstance(obj_info_enum, OBJECT_INFORMATION)):
        return None
    
    return_length = ULONG()            
    object_information = OBJECT_TYPE_INFORMATION()
    res = windll.ntdll.NtQueryObject(handle.handle,
                                     obj_info_enum,
                                     object_information,
                                     sizeof(object_information),
                                     byref(return_length))
    if STATUS_SUCCESS != res:
        object_information = create_string_buffer(return_length.value)
        res = windll.ntdll.NtQueryObject(handle.handle,
                                         obj_info_enum,
                                         object_information,
                                         sizeof(object_information),
                                         byref(return_length))
    
    if STATUS_SUCCESS != res:
        return None   
    
    ObjTypNfo = POINTER(OBJECT_TYPE_INFORMATION)
    sz_oti = sizeof(OBJECT_TYPE_INFORMATION)
    array_of_oti = bytearray(memoryview(object_information))    
    buf = (BYTE * sz_oti).from_buffer_copy(array_of_oti)
    object_information = cast(buf, ObjTypNfo)   
    object_name = object_information.contents.Name.Buffer    
    return object_name

def get_nt_object_name_info(handle):
    '''
    given a handle return the objects name
    '''
    object_value = _get_object_info(handle, OBJECT_INFORMATION.ObjectNameInformation)
    return object_value

def get_nt_object_type_info(handle):
    '''
    given a handle return the objects type
    '''
    object_value = _get_object_info(handle, OBJECT_INFORMATION.ObjectTypeInformation)
    return object_value

def get_basic_system_information():
    '''
    System Basic Information
    '''   
    return_length = ULONG()        
    sbi = SYSTEM_BASIC_INFORMATION()    
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemBasicInformation.value,
                                                byref(sbi),
                                                sizeof(sbi),
                                                byref(return_length))
    if res != 0:
        sbi = None
    return sbi

if __name__ == "__main__": 
    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)         
    )  
    
    success = acquire_privileges(new_privs)
    if not success:
        print("Failed to acquire privs!\n")
        sys.exit(-1)    
        
    pid = 1184
    for proc_obj, spi_ary in get_process_objects(pid):            
        print(proc_obj)
        for thd_obj in get_thread_objects(proc_obj, spi_ary):
            print(thd_obj)
        proc_data = proc_obj.Encode()
        for hdlinfo in get_system_handle_information(pid):            
            type_name = get_nt_object_type_info(hdlinfo)            
            object_name = get_nt_object_name_info(hdlinfo)
            hdl = proc_obj.ToDict()
            hdl["type_name"] = type_name
            hdl["object_name"] = object_name
            serdata = hdlinfo.Encode()
            print(serdata)
            new_test_instance = SYSTEM_HANDLE_TABLE_ENTRY_INFO.Decode(serdata)
            print(hdlinfo)
        new_proc_obj = SYSTEM_PROCESS_INFORMATION.Decode(proc_data)
        print(new_proc_obj)         
                    
    sys.exit(0)
    
    
    #try:        
        #sbi = get_basic_system_information()
        #print("System Basic Info = {0}\n".format(sbi,))
    
        #for pid in get_process_ids():                    
            #try:
                #proc_obj = get_process_objects(pid)
                #print("Process ID {0} Handle Information: {1}\n".format(pid,proc_obj,))                
                #for dupHandle in get_handle_information(pid):
                    #type_name = get_nt_object_type_info(dupHandle)
                    #object_name = get_nt_object_name_info(dupHandle)            
                    #print("\tHandle={0},type_name={1},object_name={2}\n".format(dupHandle, type_name,object_name,))           
                    #dupHandle.Close()            
            #except pywintypes.error as err:
                #print("Unable to get handle information from pid {0} - {1}\n"
                      #.format(pid, err,))
    #finally:
        #success = release_privileges(new_privs)        
      
    
   
    
