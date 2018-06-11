/**
* @file FltMgrCtxReg.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define the filter drivers context Registration 
*/
#pragma once
#include "common.h"
#include "externs.h"
_Success_(NULL != return)
_Must_inspect_result_
PVOID
InstanceContextAllocate(
    _In_ POOL_TYPE PoolType,
	_In_ SIZE_T Size,
	_In_ FLT_CONTEXT_TYPE ContextType);

_Success_(NULL != return)
_Must_inspect_result_
PVOID StreamContextAllocate(
	_In_ POOL_TYPE PoolType,
	_In_ SIZE_T Size,
	_In_ FLT_CONTEXT_TYPE ContextType);

_Success_(NULL != return)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_not_held_(pStreamContext->ProbeDataHeader.Resource)
_Acquires_exclusive_lock_(pStreamContext->ProbeDataHeader.Resource)
PWVU_STREAM_CONTEXT
CreateStreamContext(
    _In_ PFLT_CALLBACK_DATA          Data,
    _In_ PFLT_FILE_NAME_INFORMATION  FileNameInformation,
    _In_ StreamFlags                 Flags,
    _In_ LARGE_INTEGER               FileId,
    _Out_ PNTSTATUS                  Status);

_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(pStreamContext->ProbeDataHeader.Resource)
_Releases_lock_(pStreamContext->ProbeDataHeader.Resource)
VOID
StreamContextCleanup(
    _In_ PVOID pContext,
    _In_ FLT_CONTEXT_TYPE ContextType);

VOID
StreamContextFree(
    _In_ PVOID pVoid,
    _In_ FLT_CONTEXT_TYPE ContextType);

VOID
InstanceContextCleanup(
    _In_ PVOID pVoid,
    _In_ FLT_CONTEXT_TYPE ContextType);

VOID
InstanceContextFree(
    _In_ PVOID pVoid,
    _In_ FLT_CONTEXT_TYPE ContextType);
