/**
* @file FltMgrCallbacks.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define the filter drivers registration and instance setup / teardown entry points
*/
#pragma once
#include "common.h"
#include "externs.h"


/**
** Instance setup, teardown, filter unload and etc
*/
_Success_(TRUE == NT_SUCCESS(return))
NTSTATUS
WVUInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
WVUInstanceTeardownStart(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
WVUInstanceTeardownComplete(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

_Success_(TRUE == NT_SUCCESS(return))
NTSTATUS
WVUUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

_Success_(TRUE == NT_SUCCESS(return))
NTSTATUS
WVUInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

_Success_(TRUE == NT_SUCCESS(return))
NTSTATUS 
WVUNormalizeNameComponentExCallback(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT VolumeNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext);

_Success_(TRUE == NT_SUCCESS(return))
NTSTATUS 
WVUGenerateFileNameCallback(
    _In_   PFLT_INSTANCE Instance,
    _In_   PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA CallbackData,
    _In_   FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_   PBOOLEAN CacheFileNameInformation,
    _Out_   PFLT_NAME_CONTROL FileName);

