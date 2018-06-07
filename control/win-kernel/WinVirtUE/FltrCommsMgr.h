/**
* @file FltrCommsMgr.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Filter Communications Manager Class declaration
*/
#pragma once
#include "common.h"
#include "ProbeDataQueue.h"
#include "externs.h"
class FltrCommsMgr
{
public:
	/** the number of permitted queue port connections */
	const INT NUMBER_OF_PERMITTED_CONNECTIONS = 1;
	/** ProbeDataQueue State Change Method Type */

private:
	/** The filters queue port name */
	UNICODE_STRING usPortName;
	/** Port Object Attributes */
	OBJECT_ATTRIBUTES WVUPortObjAttr;
	/** Port security descriptor */
	PSECURITY_DESCRIPTOR pWVUPortSecDsc;
	/** initialization status */
	NTSTATUS InitStatus;

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnProtectionStateChange(
			_In_ WVU_COMMAND command);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnUnloadStateChange(
			_In_ WVU_COMMAND command);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnCommandMessage(
			_In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
			_In_ ULONG InputBufferLength);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS FLTAPI WVUPortConnect(
			_In_ PFLT_PORT ClientPort,
			_In_opt_ PVOID ServerPortCookie,
			_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
			_In_ ULONG SizeOfContext,
			_Outptr_result_maybenull_ PVOID *ConnectionPortCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_		
		static
		VOID FLTAPI WVUPortDisconnect(
			_In_opt_ PVOID ConnectionCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_		
		static
		NTSTATUS FLTAPI WVUMessageNotify(
			_In_ PVOID PortCookie,
			_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
			_In_ ULONG InputBufferLength,
			_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
			_In_ ULONG OutputBufferLength,
			_Out_ _Notnull_ PULONG ReturnOutputBufferLength);
	
public:	
	FltrCommsMgr();
	~FltrCommsMgr();
	_Success_(TRUE == return)
		BOOLEAN Enable();
	VOID Disable();
	_Must_inspect_impl_
		_Success_(NULL != return)
		PVOID operator new(_In_ size_t size);
	VOID CDECL operator delete(_In_ PVOID ptr);
};

