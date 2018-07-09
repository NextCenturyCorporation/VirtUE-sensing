/**
* @file ProcessListValidationProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief  Process List Validation
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"
#include "ProcessCreateProbe.h"
class ProcessListValidationProbe :
	public AbstractVirtueProbe
{
public:
	ProcessListValidationProbe();
	~ProcessListValidationProbe() = default;
	_Must_inspect_result_
		BOOLEAN Configure(_In_ const ANSI_STRING& config_data);
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
	_Has_lock_kind_(_Lock_kind_semaphore_)
	_Must_inspect_result_
		NTSTATUS OnRun();
};

