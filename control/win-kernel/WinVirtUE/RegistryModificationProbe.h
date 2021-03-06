/**
* @file RegistryModificationProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Registry Modification Probe class declaration
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"
#include "WVUQueueManager.h"

class RegistryModificationProbe :
	public AbstractVirtueProbe
{
private:
	static EX_CALLBACK_FUNCTION RegistryModificationCB;
	static EX_CALLBACK_FUNCTION DefaultExCallback;
	static EX_CALLBACK_FUNCTION RegNtPreSetValueKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPreDeleteValueKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPreRenameKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPreQueryValueKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPreQueryMultipleValueKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPreCreateKeyExCallback;
	static EX_CALLBACK_FUNCTION RegNtPreOpenKeyExCallback;
	static EX_CALLBACK_FUNCTION RegNtPreLoadKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtReplaceKeyCallback;
	static EX_CALLBACK_FUNCTION RegNtPostOperationCallback;
			
	LARGE_INTEGER Cookie;
public:
	RegistryModificationProbe();
	~RegistryModificationProbe() = default;
	_Success_(TRUE == return)
		BOOLEAN Start();
	_Success_(TRUE == return)
		BOOLEAN Stop();
	_Must_inspect_result_
		BOOLEAN IsEnabled();
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
		NTSTATUS Mitigate(
			_In_ UINT32 ArgC,
			_In_count_(ArgC) ANSI_STRING ArgV[]);
	_Must_inspect_result_
		NTSTATUS OnRun();
	_Must_inspect_result_
		virtual const LARGE_INTEGER& GetCookie() const { return this->Cookie; }
	_Success_(NULL != return)
		CONST ANSI_STRING&
		GetNotifyClassString(
			_In_ USHORT NotifyClass);
};

