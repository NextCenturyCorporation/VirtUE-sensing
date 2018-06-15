/**
* @file ProcessCreateProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief  Process Create Probe declaration
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"

class ProcessCreateProbe :
	public AbstractVirtueProbe
{
private:
	_Must_inspect_result_
	BOOLEAN RemoveNotify(_In_ BOOLEAN remove);
	static
	VOID ProcessNotifyCallbackEx(
			_Inout_ PEPROCESS  Process,
			_In_ HANDLE  ProcessId,
			_Inout_opt_ const PPS_CREATE_NOTIFY_INFO  CreateInfo);	
public:
	ProcessCreateProbe();
	~ProcessCreateProbe();
	_Success_(TRUE == return)
		BOOLEAN Enable();
	_Success_(TRUE == return)
		BOOLEAN Disable();
	_Must_inspect_result_
		BOOLEAN State();
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
		NTSTATUS Mitigate(
			_In_opt_count_(argc) PCHAR argv[],
			_In_ UINT32 argc);
	_Must_inspect_result_
	NTSTATUS OnRun();
};

