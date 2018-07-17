/**
* @file ThreadCreateProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief  Process Create Probe declaration
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"
#include "WVUQueueManager.h"

class ThreadCreateProbe :
	public AbstractVirtueProbe
{

private:
	static
	VOID
		ThreadCreateCallback(
			_In_ HANDLE ProcessId,
			_In_ HANDLE ThreadId,
			_In_ BOOLEAN Create);
	
public:
	ThreadCreateProbe();
	~ThreadCreateProbe();
	_Success_(TRUE == return)
		BOOLEAN Start();
	_Success_(TRUE == return)
		BOOLEAN Stop();
	_Must_inspect_result_
		BOOLEAN IsEnabled();
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
		NTSTATUS Mitigate(
			_In_opt_count_(argc) PCHAR argv[],
			_In_ UINT32 argc);
	_Must_inspect_result_
		NTSTATUS OnRun();
};

