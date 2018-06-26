/**
* @file types.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief Define simple typedefs, structs, unions or classes
*/
#pragma once
#include "common.h"

// some common typedefs . . 
typedef PUCHAR PBYTE;
typedef UCHAR BYTE;

/**
* @brief cache data
*/
typedef struct _CACHE_DATA
{
    volatile LONG LocalReferenceCount;

    /*
    * Because we like prefast, but somethings just need to be
    * -- prefast doesn't like 0 length arrays
    */
#pragma warning (suppress : 4200)
    UCHAR Data[]; /* rest of the cache */
} CACHE_DATA, *PCACHE_DATA;

/** cache data free routine */
typedef VOID (*FreeEntryDataCallback)(PCACHE_DATA EntryData);

//
// Windows VirtUE Device Extension
//
typedef struct _WVUDeviceExtension
{
    LIST_ENTRY				lePendIrp;				// Used by I/O Cancel-Safe
    IO_CSQ					ioCsq;					// I/O Cancel-Safe object
    KSPIN_LOCK				csqSpinLock;			// Spin lock object for I/O Cancel-Safe
    BOOLEAN createdSymLink;                         // True if symlink created, else false
} WVUDeviceExtension, *PWVUDeviceExtension;

// Instance Flags
typedef enum _Enum_is_bitflag_ _InstanceFlags : LONG
{
    InstanceFlagNetwork         = 1 << 0,   // Indicates instance is a Network Redirector
    InstanceFlagReadOnly        = 1 << 1,   // Indicates instance is a read only device
    InstanceFlagRemovableMedia  = 1 << 2    // Indicates instance is Removable Media
} _Enum_is_bitflag_ InstanceFlags;


/*
** Shadow Object Type 
*/
typedef enum _Enum_is_bitflag_ _NodeTypeCode
{
    StreamContext       = 1 << 0,
    FileObjectContext   = 1 << 1,	
} _Enum_is_bitflag_ NodeTypeCode;


typedef enum _Enum_is_bitflag_ _StreamFlags : LONG
{
    /*
    * No operation
    */
    STREAM_FLAG_NOOP = 0,

    /*
    * STREAM_FLAG_PROTECTED: Specifies that the file is a PFF.
    */
    STREAM_FLAG_PROTECTED = 1 << 0,

    /*
    * STREAM_FLAG_DIRTY: Specifies that the file has been written to.
    */
    STREAM_FLAG_DIRTY = 1 << 1,

    /*
    * STREAM_FLAG_FILE_MAPPED: This stream has been mapped.
    */
    STREAM_FLAG_FILE_MAPPED = 1 << 2,

    /*
    * STREAM_FLAG_DELETE_ON_CLOSE: Flag to indicate that this stream is pending
    * delete. New creates are not allowed.
    */
    STREAM_FLAG_DELETE_ON_CLOSE = 1 << 3,

    /*
    * STREAM_FLAG_READONLY: The underlying file is read only.
    */
    STREAM_FLAG_READONLY = 1 << 4,

} _Enum_is_bitflag_ StreamFlags, *PStreamFlags;


//
// Windows VirtUE Instance Context
//
typedef struct _WVU_INSTANCE_CONTEXT
{
    ULONG SectorSize;
    ULONG AllocationSize;
    UNICODE_STRING VolumeName;
    /* TRUE if volume name was allocated else FALSE */
    BOOLEAN AllocatedVolumeName;
    FLT_FILESYSTEM_TYPE FilesystemType;
    volatile InstanceFlags Flags;
} WVU_INSTANCE_CONTEXT, *PWVU_INSTANCE_CONTEXT;

//
// Windows VirtUE Stream Context
//
typedef struct _WVU_STREAM_CONTEXT
{
    FSRTL_ADVANCED_FCB_HEADER ProbeDataHeader;  // common FCB header also used for synchronization     
    volatile StreamFlags SFlags;
    ERESOURCE Resource;
    ERESOURCE PagingIoResource;
    FAST_MUTEX FastMutex;
    PFLT_FILE_NAME_INFORMATION  FileNameInformation;  // This files name
    LARGE_INTEGER FileId;   // This files file id
	PWVU_INSTANCE_CONTEXT InstanceContext;  // This files Instance Context
    PFILE_OBJECT OriginalRelatedFileObject;  // The original file object
} WVU_STREAM_CONTEXT, *PWVU_STREAM_CONTEXT;

//
// Windows VirtUE Globals
typedef struct _WVUGlobals
{
    LOOKASIDE_LIST_EX StreamContextLookaside;

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //
    PFLT_FILTER FilterHandle;

	//
	// Command/Response Port
	//
	PFLT_PORT WVUCommandPort;

    //
    //  User process that connected to the port
    //
    PEPROCESS CommandUserProcess;

	//
	//  Data streaming port
	//
    PFLT_PORT WVUProbeDataStreamPort;

	//
	//  User process that connected to the port
	//
	PEPROCESS DataStreamUserProcess;

    //
    // Connecton Cookie
    //
    PVOID ConnectionCookie;

    EX_RUNDOWN_REF RunDownRef;
#if defined(WVU_DEBUG)
	_Interlocked_
	volatile LONG StreamContextLookasideAllocateCnt;
#endif
    RTL_OSVERSIONINFOEXW lpVersionInformation;
    KEVENT WVUThreadStartEvent;
	KEVENT poll_wait_evt;	    // polling thread waiter
    BOOLEAN AllowFilterUnload;  // if true, then allow the filter to be unloaded else don't allow
    BOOLEAN ProtectionEnabled;   // if true then the driver is protecting
	BOOLEAN CommandConnected;   // TRUE if the Command Port is connected else FALSE
	PDRIVER_OBJECT DriverObject;	
	BOOLEAN ShuttingDown;
	HANDLE MainThreadHandle;
} WVUGlobals, *PWVUGlobals;


/**
* Savior Command Enumeration 
*/
typedef enum _SaviorCommand : UINT16
{
	ECHO = 0
} SaviorCommand;

/** 
* Savior Command Packet
*/
typedef struct _SaviorCommandPkt : FILTER_MESSAGE_HEADER
{
	SaviorCommand Cmd;	// The savior command to send to the listening user space service
	UINT16 CmdMsgSize;	// The command message size
	UCHAR CmdMsg[1];    // The command message
} *PSaviorCommandPkt, SaviorCommandPkt;


typedef enum _ProbeIdType : USHORT
{
	NoProbeIdType = 0x0000,
	/** Loaded Image (.exe,.dll, etc) notificaton type */
	LoadedImage    = 0x0001,
	/** Process Creation notificaton type */
	ProcessCreate  = 0x0002,
	/** Process Destruction notificaton type */
	ProcessDestroy = 0x0003,
	/** Thread Creation notificaton type */
	ThreadCreate   = 0x0004,
	/** Thread Destruction notificaton type */
	ThreadDestroy  = 0x0005,
	/** process list validation */
	ProcessListValidation = 0x0006
} ProbeIdType;

_Struct_size_bytes_(DataSz)
typedef struct _ProbeDataHeader 
{
	_In_ ULONG ReplyLength;
	_In_ ULONGLONG MessageId;
	_In_ ProbeIdType  ProbeId;
	_In_ USHORT DataSz;
	_In_ LARGE_INTEGER CurrentGMT;
	_In_ LIST_ENTRY  ListEntry;
} PROBE_DATA_HEADER, *PPROBE_DATA_HEADER;


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
