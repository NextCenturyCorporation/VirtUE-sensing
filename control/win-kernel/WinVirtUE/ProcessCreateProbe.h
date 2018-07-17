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

public:
#pragma region Process Entry Structure Definition
#define COMMON_POOL_TAG WVU_PROCESSENTRY_POOL_TAG
	class ProcessEntry
	{
	public:
		LIST_ENTRY ListEntry;
		PEPROCESS pEProcess;
		HANDLE ProcessId;

		ProcessEntry() : ListEntry({ nullptr,nullptr }), 
			ProcessId(INVALID_HANDLE_VALUE), pEProcess(nullptr) {}
		~ProcessEntry() = default;

		/**
		* @brief construct an instance of this object utilizing non paged pool memory
		* @return Instance of ProbeInfo Class
		*/
		PVOID operator new(size_t size)
		{
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
			PVOID pVoid = ExAllocatePoolWithTag(NonPagedPool, size, COMMON_POOL_TAG);
			return pVoid;
		}

		/**
		* @brief destroys an instance of this object and releases its memory
		* @param ptr pointer to the object instance to be destroyed
		*/
		VOID CDECL operator delete(PVOID ptr)
		{
			if (!ptr)
			{
				return;
			}
			ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
		}
	};
#undef COMMON_POOL_TAG
#pragma endregion
private:
	/** The process list */
	LIST_ENTRY ProcessList;
	/** ProcessList Spin Lock */
	KSPIN_LOCK ProcessListSpinLock;
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
	NTSTATUS RemoveNotify(_In_ BOOLEAN remove);
	static
	VOID ProcessNotifyCallbackEx(
			_Inout_ PEPROCESS Process,
			_In_ HANDLE ProcessId,
			_Inout_opt_ const PPS_CREATE_NOTIFY_INFO  CreateInfo);	
public:
	ProcessCreateProbe();
	~ProcessCreateProbe();
	_Success_(TRUE == return)
		BOOLEAN Start();
	_Success_(TRUE == return)
		BOOLEAN Stop();
	_Must_inspect_result_
		BOOLEAN IsEnabled();
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
		NTSTATUS Mitigate(
			_In_ UINT32 argc,
			_In_count_(argc) ANSI_STRING argv[]);
	_Must_inspect_result_
	NTSTATUS OnRun();
	_Has_lock_kind_(_Lock_kind_semaphore_)
	_Must_inspect_result_
	_Success_(NULL != return)
	ProcessEntry* FindProcessByEProcess(_In_ PEPROCESS pEPROCESS);
	_Has_lock_kind_(_Lock_kind_semaphore_)
	_Must_inspect_result_
	_Success_(NULL != return)
	ProcessEntry* FindProcessByProcessId(_In_ HANDLE ProcessId);
	_Has_lock_kind_(_Lock_kind_semaphore_)
	_Success_(TRUE == return)	
	BOOLEAN InsertProcessEntry(_In_ PEPROCESS pEProcess, _In_ HANDLE ProcessId);
	_Has_lock_kind_(_Lock_kind_semaphore_)
	_Success_(TRUE == return)
	BOOLEAN RemoveProcessEntry(_In_ ProcessEntry* pProcessEntry);
	KSPIN_LOCK& GetProcessListSpinLock() { return this->ProcessListSpinLock; }
	const LIST_ENTRY& GetProcessList() { return this->ProcessList; }
};

