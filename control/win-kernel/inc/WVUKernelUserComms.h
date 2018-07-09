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
	LoadedImage = 0x0001,
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
	/** Registry Modificatin */
	RegistryModification = 0x0007
} ProbeType;

_Struct_size_bytes_(data_sz)
typedef struct _ProbeDataHeader
{
	_In_ UUID probe_id;
	_In_ ProbeType  probe_type;
	_In_ USHORT data_sz;
	_In_ LARGE_INTEGER current_gmt;
	_In_ LIST_ENTRY  ListEntry;
} PROBE_DATA_HEADER, *PPROBE_DATA_HEADER;

typedef struct _RegistryModificationInfo
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;	
	_In_ HANDLE ProcessId;	      // The process that is emitting the registry changes
	_In_ PEPROCESS  EProcess;     // The EProcess that is emitting the registry changes
} RegistryModificationInfo, *PRegistryModificationInfo;

typedef struct _ProcessListValidationFailed
{
	_In_ PROBE_DATA_HEADER ProbeDataHeader;
	_In_ NTSTATUS Status;	      // the operations status
	_In_ HANDLE ProcessId;	      // the process id that was NOT found in the process list
	_In_ PEPROCESS  EProcess;     // the eprocess that was NOT found in the process list
} ProcessListValidationFailed, *PProcessListValidationFailed;

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