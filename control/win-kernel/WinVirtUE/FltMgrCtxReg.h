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
    __in POOL_TYPE PoolType,
    __in SIZE_T Size,
    __in FLT_CONTEXT_TYPE ContextType);

_Success_(NULL != return)
_Must_inspect_result_
PVOID StreamContextAllocate(
    __in POOL_TYPE PoolType,
    __in SIZE_T Size,
    __in FLT_CONTEXT_TYPE ContextType);

_Success_(NULL != return)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_IRQL_raises_(APC_LEVEL)
PWVU_STREAM_CONTEXT
CreateStreamContext(
    _In_ PFLT_CALLBACK_DATA          Data,
    _In_ PFLT_FILE_NAME_INFORMATION  FileNameInformation,
    _In_ StreamFlags                 Flags,
    _In_ LARGE_INTEGER               FileId,
    _Out_ PNTSTATUS                  Status);

VOID
StreamContextCleanup(
    _In_ PVOID pVoid,
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
