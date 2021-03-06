/**
* @file WVUCommsManager.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Filter Communications Manager Class declaration
*/
#pragma once
#include "common.h"
#include "WVUQueueManager.h"

class WVUCommsManager
{
public:
	/** the number of permitted queue port connections */
	const INT NUMBER_OF_PERMITTED_CONNECTIONS = 1;
	/** WVUQueueManager State Change Method Type */

private:
	WVUCommsManager();

	// port name used to communicate between user and kernel
	static CONST PWSTR PortName;
	/** port name for the commands issued by the user space program */
	static CONST PWSTR CommandName;
	/** The filters data streaming queue port name */
	UNICODE_STRING usPortName;
	/** The filters command port name */
	UNICODE_STRING usCommandName;
	/** Port Object Attributes */
	OBJECT_ATTRIBUTES WVUPortObjAttr;
	/** Port Object Attributes */
	OBJECT_ATTRIBUTES WVUComandObjAttr;
	/** Port security descriptor */
	PSECURITY_DESCRIPTOR pWVUPortSecDsc;
	/** Command Port security descriptor */
	PSECURITY_DESCRIPTOR pWVUCommandSecDsc;
	/** initialization status */
	NTSTATUS InitStatus;

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnProtectionStateChange(
			_In_ CONST PCOMMAND_MESSAGE pCmdMsg);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnUnloadStateChange(
			_In_ CONST PCOMMAND_MESSAGE pCmdMsg);

	_IRQL_requires_(PASSIVE_LEVEL)
	_IRQL_requires_same_
	static
	NTSTATUS OnEnumerateProbes(
		_In_ PCOMMAND_MESSAGE pCmdMsg,
		_Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
		_In_ ULONG OutputBufferLength,
		_Out_ _Notnull_ PULONG ReturnOutputBufferLength);

	_IRQL_requires_(PASSIVE_LEVEL)
	_IRQL_requires_same_
	static
	NTSTATUS OnConfigureProbe(
		_In_ PCOMMAND_MESSAGE pCmdMsg);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnOneShotKill(
			_In_ PCOMMAND_MESSAGE pCmdMsg);
	
	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static 
		VOID CreateStandardResponse(
			_In_ NTSTATUS Status,
			_Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
			_In_ ULONG OutputBufferLength,
			_Out_ _Notnull_ PULONG ReturnOutputBufferLength);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS OnCommandMessage(
			_In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
			_In_ ULONG InputBufferLength,			
			_Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
			_In_ ULONG OutputBufferLength,
			_Out_ _Notnull_ PULONG ReturnOutputBufferLength);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS FLTAPI WVUDataStreamPortConnect(
			_In_ PFLT_PORT WVUProbeDataStreamPort,
			_In_opt_ PVOID ServerPortCookie,
			_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
			_In_ ULONG SizeOfContext,
			_Outptr_result_maybenull_ PVOID *ConnectionPortCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_		
		static
		VOID FLTAPI WVUDataStreamPortDisconnect(
			_In_opt_ PVOID ConnectionCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		NTSTATUS FLTAPI WVUCommandConnect(
			_In_ PFLT_PORT WVUProbeDataStreamPort,
			_In_opt_ PVOID ServerPortCookie,
			_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
			_In_ ULONG SizeOfContext,
			_Outptr_result_maybenull_ PVOID *ConnectionPortCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		VOID FLTAPI WVUCommandDisconnect(
			_In_opt_ PVOID ConnectionCookie);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_
		static
		BOOLEAN ChangeProbeState(
		_In_ WVUQueueManager::ProbeInfo * pProbeInfo, 
		_In_ WVU_COMMAND Command);

	_IRQL_requires_(PASSIVE_LEVEL)
		_IRQL_requires_same_		
		static
		NTSTATUS FLTAPI WVUCommandMessageNotify(
			_In_ PVOID PortCookie,
			_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
			_In_ ULONG InputBufferLength,
			_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
			_In_ ULONG OutputBufferLength,
			_Out_ _Notnull_ PULONG ReturnOutputBufferLength);

public:	

	/**
	* @brief returns the one and only comms manager instance 
	* @returns WVUCommsManager instance singleton
	*/
	static WVUCommsManager& GetInstance()
	{
		static WVUCommsManager instance;
		return instance;
	}

	~WVUCommsManager();
	_Success_(TRUE == return)
		BOOLEAN Start();
	VOID Stop();
	_Must_inspect_impl_
		_Success_(NULL != return)
		PVOID operator new(_In_ size_t size);
	VOID CDECL operator delete(_In_ PVOID ptr);
	/**
	* @brief copy constructor 
	*/
	WVUCommsManager(const WVUCommsManager &t) = delete;
	
	/**
	* @brief assignment operator
	*/
	WVUCommsManager& operator=(const WVUCommsManager &rhs) = delete;	
};

