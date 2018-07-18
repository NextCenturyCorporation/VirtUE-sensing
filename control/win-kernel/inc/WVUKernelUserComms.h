/**
* @file WVUKernelUserComms.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Windows VirtUE User/Kernel Communications
*/
#pragma once

#if defined(NTDDI_VERSION)
#include <ntddk.h>
#else
#include <windows.h>
#endif
#define MAXSENSORNAMESZ 64
#define MAXMESSAGELEN 1024

//
//  Command type enumeration, please see COMMAND_MESSAGE below
//
typedef enum _WVU_COMMAND : ULONG
{
	Echo = 0,
	EnableProtection = 1,
	DisableProtection = 2,
	EnableUnload = 3,
	DisableUnload = 4,
	EnumerateProbes = 5,
	ConfigureProbe = 6,
	OneShotKill = 7,
	MAXCMDS
} WVU_COMMAND;

//
//  Response Type
//
typedef enum _WVU_RESPONSE : ULONG
{
	NORESPONSE = 0,
	Success = 1,
	Failure = 2,
	NoSuchProbe = 3,
	MAXRESP = 3
} WVU_RESPONSE;

//
// Defines the possible replies between the filter and user prograem
// Reply:  User <- Kernel
//
typedef struct _RESPONSE_MESSAGE
{
	WVU_RESPONSE Response;	// Command Response Type
	NTSTATUS Status;		// returned status from command
	SIZE_T DataSz;			// The Optional Response Packets Data Size
	UCHAR Data[1];			// Optional Response Packet Data
} RESPONSE_MESSAGE, *PRESPONSE_MESSAGE;

//
//  Defines the commands between the user program and the filter
//  Command: User -> Kernel
//
typedef struct _COMMAND_MESSAGE {
	WVU_COMMAND Command;// The Command
	UUID ProbeId;		// The probe id this command is directed towards (0 is all probes)
	SIZE_T DataSz;		// The Optional Command Message Data Size
	UCHAR Data[1];      // Optional Command Message Data
} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;

//
//  Connection type enumeration. It will be primarily used in connection context.
//
typedef enum _WVU_CONNECTION_TYPE {
	WVUConnectNotSecure = 1  // By default we have an unsecure connection between the user app and kernel filter
} WVU_CONNECTION_TYPE, *PWVU_CONNECTION_TYPE;

//
//  Connection context. It will be passed through FilterConnectCommunicationPort(...)
//
typedef struct _WVU_CONNECTION_CONTEXT {
	WVU_CONNECTION_TYPE   Type;  // The connection type
} WVU_CONNECTION_CONTEXT, *PWVU_CONNECTION_CONTEXT;

typedef enum _ProbeType : USHORT
{
	NoProbeIdType = 0x0000,
	/** Loaded Image (.exe,.dll, etc) notificaton type */
	ImageLoad = 0x0001,
	/** Process Creation notificaton type */
	ProcessCreate = 0x0002,
	/** Process Destruction notificaton type */
	ProcessDestroy = 0x0003,
	/** Thread Creation notificaton type */
	ThreadCreate = 0x0004,
	/** Thread Destruction notificaton type */
	ThreadDestroy = 0x0005,
	/** process list validation */
	ProcessListValidation = 0x0006,
	/** Registry Modification */
	RegQueryValueKeyInformation = 0x0007,
	RegCreateKeyInformation = 0x0008,
	RegSetValueKeyInformation = 0x0009,
	RegOpenKeyInformation = 0x000A,
	RegDeleteValueKeyInformation = 0x000B,	
	RegRenameKeyInformation = 0x000C,
	RegQueryMultipleValueKeyInformation = 0x000D,
	RegPreReplaceKey = 0x000E,
	RegPostReplaceKey = 0x000F,
	RegPreLoadKey = 0x0010,
	/** post operations start at 0x1001 */
	RegPostOperationInformation = 0x1001
} ProbeType;

_Struct_size_bytes_(Size + sizeof USHORT)
typedef struct _Atom
{
	_In_ USHORT Size;
	_In_  BYTE Buffer[0];
} Atom, *PAtom;

_Struct_size_bytes_(data_sz)
typedef struct _ProbeDataHeader
{
	_In_ UUID probe_id;
	_In_ ProbeType  probe_type;
	_In_ USHORT data_sz;
	_In_ LARGE_INTEGER current_gmt;
	_In_ LIST_ENTRY  ListEntry;
} PROBE_DATA_HEADER, *PPROBE_DATA_HEADER;

typedef struct _RegPostOperationInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;			// The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;		// The EProcess that is emitting the registry changes
	_In_ PVOID Object;				// A pointer to the registry key object for which the operation has completed.	
	_In_ NTSTATUS Status;			// The NTSTATUS-typed value that the system will return for the registry operation
	_In_ PVOID PreInformation;		// A pointer to the structure that contains preprocessing information for the registry operation that has completed
	_In_ NTSTATUS ReturnStatus;		// A driver-supplied NTSTATUS-typed value.
} RegPostOperationInfo, *PRegPostOperationInfo;

typedef struct _RegLoadKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	    // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;	// The EProcess that is emitting the registry changes
	_In_ PVOID Object;			// registry key object pointer	
	_In_ ACCESS_MASK     DesiredAccess;
	_In_ ULONG EntryCount;		// The number of entries in the ValueEntries array
	_In_ LONG BufferLength;		// variable that contains the length, in bytes, of the ValueBuffer buffer.
	_In_ BYTE ValueBuffer[0];	// A pointer to a buffer that receives the unicode strings KeyName and SourceFile
} RegLoadKeyInfo, *PRegLoadKeyInfo;

typedef struct _RegQueryMultipleValueKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	    // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;	// The EProcess that is emitting the registry changes
	_In_ PVOID Object;			// registry key object pointer	
	_In_ ULONG EntryCount;		// The number of entries in the ValueEntries array
	_In_ LONG BufferLength;		// variable that contains the length, in bytes, of the ValueBuffer buffer.
	_In_ BYTE ValueBuffer[0];	// A pointer to a buffer that receives (from the system) the data for all the value entries specified by the ValueEntries array
} RegQueryMultipleValueKeyInfo, *PRegQueryMultipleValueKeyInfo;

typedef struct _RegDeleteValueKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	      // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;     // The EProcess that is emitting the registry changes
	_In_ PVOID Object;			  // registry key object pointer	
	_In_ USHORT ValueNameLength;  // the value name length	
	_In_ BYTE ValueName[0];		  // key value information
} RegDeleteValueKeyInfo, *PRegDeleteValueKeyInfo;

typedef struct _RegRenameKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	    // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;   // The EProcess that is emitting the registry changes
	_In_ PVOID Object;			// registry key object pointer	
	_In_ USHORT NewNameLength;  // the new name length	
	_In_ BYTE NewName[0];		// new name
} RegRenameKeyInfo, *PRegRenameKeyInfo;

typedef struct _RegCreateKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	      // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;     // The EProcess that is emitting the registry changes
	_In_ PVOID           RootObject;
	_In_ ULONG           Options;
	_In_ PVOID           SecurityDescriptor;
	_In_ PVOID           SecurityQualityOfService;
	_In_ ACCESS_MASK     DesiredAccess;
	_In_ ACCESS_MASK     GrantedAccess;
	_In_ ULONG_PTR       Version;
	_In_ ULONG           Wow64Flags;
	_In_ ULONG           Attributes;
	_In_ KPROCESSOR_MODE CheckAccessMode;
	_In_ USHORT			 CompleteNameSz;
	_In_ BYTE            CompleteName[1];
} RegCreateKeyInfo, *PRegCreateKeyInfo;

typedef enum _RegObjectType : ULONG
{
	RegNone = REG_NONE, // No value type
	RegSz = REG_SZ, // Unicode nul terminated string
	RegExpandSz = REG_EXPAND_SZ, // Unicode nul terminated string
	// (with environment variable references)
	RegBinary = REG_BINARY, // Free form binary
	RegDWord = REG_DWORD, // 32-bit number
	RegDWordLE = REG_DWORD_LITTLE_ENDIAN, // 32-bit number (same as REG_DWORD)
	RegDWordBE = REG_DWORD_BIG_ENDIAN, // 32-bit number
	RegLink = REG_LINK, // Symbolic Link (unicode)
	RegMultiSz = REG_MULTI_SZ, // Multiple Unicode strings
	RegResourceList = REG_RESOURCE_LIST, // Resource list in the resource map
	RegFullResourceDescriptor = REG_FULL_RESOURCE_DESCRIPTOR, // Resource list in the hardware description
	RegResourceRequirementsList = REG_RESOURCE_REQUIREMENTS_LIST,
	RegQWord = REG_QWORD, // 64-bit number
	RegQWordLE = REG_QWORD_LITTLE_ENDIAN // 64-bit number (same as REG_QWORD)
} RegObjectType, *PRegObjectType;

typedef struct _RegSetValueKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	      // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;     // The EProcess that is emitting the registry changes
	_In_ PVOID Object;			  // registry key object pointer	
	_In_ RegObjectType Type;      // the registry object type
	_In_ USHORT ValueNameLength;   // the value name length	
	_In_ BYTE ValueName[0];		  // key value information
} RegSetValueKeyInfo, *PRegSetValueKeyInfo;

typedef struct _RegQueryValueKeyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	// probe data header
	_In_ HANDLE ProcessId;	      // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;     // The EProcess that is emitting the registry changes
	_In_ ULONG Class;  // This registry modification notification class
	_In_ PVOID Object;			  // registry key object pointer
	_In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;   // value that indicates the type of information to be returned by the system
	_In_ USHORT ValueNameLength;   // the value name length	
	_In_ BYTE ValueName[0];		  // key value information
} RegQueryValueKeyInfo, *PRegQueryValueKeyInfo;

typedef struct _ProcessListValidationFailed
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ NTSTATUS Status;	      // the operations status
	_In_ HANDLE ProcessId;	      // the process id that was NOT found in the process list
	_In_ PEPROCESS  EProcess;     // the eprocess that was NOT found in the process list
} ProcessListValidationFailed, *PProcessListValidationFailed;


typedef struct _ThreadCreateInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ HANDLE ProcessId;
	_In_ PEPROCESS  EProcess;
	_In_ BOOLEAN Create;
} ThreadCreateInfo, *PThreadCreateInfo;


typedef struct _LoadedImageInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ HANDLE ProcessId;
	_In_ PEPROCESS  EProcess;
	_In_ PVOID ImageBase;
	_In_ SIZE_T ImageSize;
	_In_ USHORT FullImageNameSz;
	_In_ UCHAR FullImageName[1];
} LoadedImageInfo, *PLoadedImageInfo;

/**
* @note Is it important to also include the imagefilename or is it
* duplicate information from module load?
*/
typedef struct _ProcessCreateInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ HANDLE ParentProcessId;
	_In_ HANDLE ProcessId;
	_In_ PEPROCESS EProcess;
	_In_ CLIENT_ID CreatingThreadId;
	_Inout_ struct _FILE_OBJECT *FileObject;
	_Inout_ NTSTATUS CreationStatus;
	_In_ USHORT CommandLineSz;
	_In_ UCHAR CommandLine[1];
} ProcessCreateInfo, *PProcessCreateInfo;


typedef struct _ProcessDestroyInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ HANDLE ProcessId;
	_In_ PEPROCESS EProcess;
} ProcessDestroyInfo, *PProcessDestroyInfo;

/** The probe status message header */
typedef struct _ProbeStatusHeader
{
	DWORD NumberOfEntries;	
} ProbeStatusHeader, *PProbeStatusHeader;

/** The probe status as recovered by the probe enumeration function */
typedef struct _ProbeStatus
{
	UUID ProbeId;			    /** this probes unique probe number, becomes SensorId or sensor_id in user space */
	LARGE_INTEGER LastRunTime;  /** GMT time this probe last ran */
	LARGE_INTEGER RunInterval;  /** This probes configured run interval - if any */
	LONG OperationCount;        /** The number of completed operations since driver was loaded */
	USHORT Attributes;			/** This probes attributes */
	BOOLEAN Enabled;			/** TRUE if this probe is enabled else FALSE */
	CHAR ProbeName[MAXSENSORNAMESZ];	/** This probes name, becomes SensorName or sensor_name in user space */
} ProbeStatus, *PProbeStatus;