import sys
import pywintypes

from enum import Enum, IntEnum
from ctypes import byref, sizeof, Structure, c_void_p, c_ushort, c_wchar_p, c_size_t, WINFUNCTYPE, pointer, windll, c_ubyte, c_int32, c_ulonglong, cast, create_string_buffer, addressof, POINTER, GetLastError
from ctypes.wintypes import HANDLE, ULONG, PULONG, LONG, LARGE_INTEGER, BYTE, SHORT, BOOLEAN
from ntsecuritycon import SE_SECURITY_NAME, SE_CREATE_PERMANENT_NAME, SE_DEBUG_NAME
from win32con import SE_PRIVILEGE_ENABLED
from win32api import OpenProcess, DuplicateHandle, GetCurrentProcess
from win32security import LookupPrivilegeValue, OpenProcessToken, AdjustTokenPrivileges, TOKEN_ALL_ACCESS

PARAMFLAG_FIN   = 0x1
PARAMFLAG_FOUT  = 0x2
PARAMFLAG_FLCID = 0x4
PARAMFLAG_COMBINED = PARAMFLAG_FIN | PARAMFLAG_FOUT | PARAMFLAG_FLCID

PROCESS_DUP_HANDLE = 0x0040
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
                           
class CtypesEnum(IntEnum):
    """A ctypes-compatible IntEnum superclass."""
    @classmethod
    def from_param(cls, obj):
        return int(obj)


class UNICODE_STRING(Structure):
    _fields_ = [ 
        ("Length", c_ushort),
        ("MaximumLength", c_ushort),
        ("Buffer", c_wchar_p)
        ]

class SmartStructure(Structure):
    
    def __init__(self):
        '''
        Initialize this instance
        '''
        pass
    
    def __str__(self):
        '''
        create a string that shows this instances internal state
        '''
        szfields = len(type(self)._fields_)
        state = ""
        for ndx in range(0, szfields):            
            this_name = type(self)._fields_[ndx][0];
            this_value = getattr(self, this_name)                           
            state += "{0}={1},".format( this_name, this_value,)
            
        state = state[:len(state)-1]
        return state
        
    __repr__ = __str__
    
    
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
    _fields_ = [("TypeName", UNICODE_STRING ),
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
    
STATUS_SUCCESS=0x0
STATUS_INFO_LENGTH_MISMATCH=0xc0000004
    
prototype = WINFUNCTYPE(LONG, SYSTEM_INFORMATION_CLASS, c_void_p, ULONG, PULONG, use_last_error=True)
paramflags = (PARAMFLAG_FIN, "SystemInformationClass"), (PARAMFLAG_FIN, "SystemInformation"), (PARAMFLAG_FIN, "SystemInformationLength"), (PARAMFLAG_FOUT, "ReturnLength")
NtQuerySystemInformation = prototype(("NtQuerySystemInformation", windll.ntdll), paramflags)

def GetSystemBasicInformation():
    '''
    System Basic Information
    '''   
    return_length = ULONG()        
    sbi = SYSTEM_BASIC_INFORMATION()    
    res = windll.ntdll.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemBasicInformation.value,
                                                byref(sbi),
                                                sizeof(sbi),
                                                byref(return_length))
    return sbi
    
def GetSystemProcessInformation(pid=None):
    '''
    System Process Information
    '''    
    if not pid or not isinstance(pid, int) or 0 != pid % 4:
        return None
    
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
    sys_proc_info_list = []
    while True:
        buf = (BYTE * sz_spi).from_buffer_copy(array_of_spi[begin:end])
        spi = cast(buf, SystemProcInfo)
        if not not pid and isinstance(pid, int) and pid == spi.contents.UniqueProcessId:
            sys_proc_info_list.append(spi.contents)
            return sys_proc_info_list
        elif not pid:
            sys_proc_info_list.append(spi.contents)        
        
        next_entry_offset = spi.contents.NextEntryOffset
        if(0 == next_entry_offset):
            break
        begin = begin + next_entry_offset
        end = begin + sz_spi        
    
    return sys_proc_info_list

def GetSystemHandleInformation(pid=None):
    '''
    System Handle Information
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
    system_handle_information_list = []

    for ndx in range(0, SystemHandleInfo.contents.NumberOfHandles): 
        begin = sz_shtei * ndx
        end = begin + sz_shtei
        buf = (BYTE * sz_shtei).from_buffer_copy(array_of_shtei[begin:end])
        shi = cast(buf, HdlEntry)
        if not not pid and isinstance(pid, int) and pid == shi.contents.UniqueProcessId:
            system_handle_information_list.append(shi.contents)
        elif not pid:
            system_handle_information_list.append(shi.contents)
                
    return system_handle_information_list

def _GetProcessHandle(pid):
    '''
    '''
    if not pid or not isinstance(pid, int) or 0 != pid % 4:
        return None    
    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)         
    )    
    privilege = ('SeSecurityPrivilege')
    bDisableAllPrivileges  = False
    hCurrentProcess = GetCurrentProcess()
    
    hProcToken = OpenProcessToken(hCurrentProcess, TOKEN_ALL_ACCESS)
    if not hProcToken:
        return None
        
    bSuccess = AdjustTokenPrivileges(hProcToken,  bDisableAllPrivileges , new_privs)
    if not bSuccess:
        hProcToken.Close()
        return None
                   
    process_handle = OpenProcess(PROCESS_DUP_HANDLE, False, pid)
    if not process_handle:
        hProcToken.Close()        
        return None       
    return process_handle

def GetHandleInformation(pid=None):
    '''
    '''
    phi = GetSystemProcessInformation(pid)
    if not phi:
        return None
    
    process_handle = _GetProcessHandle(pid)
    if not process_handle:
        return None    
    
    current_process_handle = GetCurrentProcess()
        
    shi = GetSystemHandleInformation(pid)
    if not shi:
        return None
    
    duped_handle_list = []
    for ndx in range(0, len(shi)):
        try:            
            duped_handle = DuplicateHandle(process_handle, shi[ndx].HandleValue, current_process_handle, PROCESS_DUP_HANDLE, False, 0)
        except pywintypes.error as err:
            print("Handle {0}@{1} Caught a pywintypes.err Error {2}\n".format(shi[ndx],ndx,err,))            
        else:
            duped_handle_list.append(duped_handle)

    return duped_handle_list
        
        
if __name__ == "__main__":    
    
    duped_handle_list = GetHandleInformation(1408)            
    if not not duped_handle_list:            
        for ndx in range(0, len(duped_handle_list)):
            handle_to_be_closed = duped_handle_list[ndx]
            handle_to_be_closed.Close()
    sys.exit(0)
            
    sbi = GetSystemBasicInformation()
    print("System Basic Info = {0}\n".format(sbi,))
    
    spi = GetSystemProcessInformation()
    for ndx in range(0, len(spi)):
        print("Process Information {0} = {1}\n".format(ndx, spi[ndx]),)
    
    shi = GetSystemHandleInformation(4)        
    for ndx in range(0, len(spi)):
        print("Handle Information {0} = {1}\n".format(ndx, shi[ndx]),)    
    
   
    