/**
* @file PortMsgTrans.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define the drivers entry points, major function handlers and etc
*/
#pragma once
#include "common.h"
#include "WVUKernelUserComms.h"
#include "externs.h"

#define DEVICE_SDDL             L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"   // limit access to system and admins only

/*************************************************************************
** Prototypes
**************************************************************************/

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnProtectionStateChange(
	_In_ WVU_COMMAND command);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnUnloadStateChange(
	_In_ WVU_COMMAND command);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnCommandMessage(
	_In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
FLTAPI WVUPortConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ _Notnull_ PVOID ServerPortCookie,
	_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID *ConnectionPortCookie);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
FLTAPI WVUPortDisconnect(
	_In_opt_ PVOID ConnectionCookie);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
FLTAPI WVUMessageNotify(
	_In_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ _Notnull_ PULONG ReturnOutputBufferLength);