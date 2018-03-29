/**
* @file FltMgrCallbacks.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define the filter drivers callback entry points
*/
#pragma once
#include "common.h"


/**
** Operation Callbacks
*/

FLT_PREOP_CALLBACK_STATUS
WinVirtUEPreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

VOID
WinVirtUEOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);

FLT_POSTOP_CALLBACK_STATUS
WinVirtUEPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
WinVirtUEPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

BOOLEAN
WinVirtUEDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

