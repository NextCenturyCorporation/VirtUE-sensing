/**
* @file ProcessCreateProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessCreateProbe Probe Class definition
*/
#include "ProcessCreateProbe.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_PROCESSCTORDTORPROBE_POOL_TAG

static ANSI_STRING probe_name = RTL_CONSTANT_STRING("ProcessCreate");
const ANSI_STRING one_shot_kill = RTL_CONSTANT_STRING("one-shot-kill");

/**
* @brief construct an instance of this probe 
*/
ProcessCreateProbe::ProcessCreateProbe() : 
	AbstractVirtueProbe(probe_name)
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
	// initialize the spinlock that controls access to the Response queue
	KeInitializeSpinLock(&this->ProcessListSpinLock);

	// initialize the Response Queue TWLL
	InitializeListHead(&this->ProcessList);
}


/**
* @brief destroy an instance of this probe
*/
ProcessCreateProbe::~ProcessCreateProbe()
{	
	while (FALSE == IsListEmpty(&this->ProcessList))
	{
		PLIST_ENTRY pListEntry = RemoveHeadList(&this->ProcessList);
		ProcessEntry* pProcessEntry = CONTAINING_RECORD(pListEntry, ProcessEntry, ListEntry);
		delete pProcessEntry;
	}
}

_Use_decl_annotations_
NTSTATUS
ProcessCreateProbe::RemoveNotify(BOOLEAN remove)
{
	const NTSTATUS Status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateProbe::ProcessNotifyCallbackEx, remove);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, %s) "
			"Failed! Status=%08x\n", TRUE ? "REMOVE" : "ADD", Status);
		goto ErrorExit;
	}

ErrorExit:

	return Status;
}

/**
* @brief Start the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::Start()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	if (TRUE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already enabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}
	if ((Attributes & ProbeAttributes::EnabledAtStart) != ProbeAttributes::EnabledAtStart)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w not enabled at start - probe is registered but not active\n",
			&this->ProbeName);
		goto ErrorExit;
	}
	Status = this->RemoveNotify(FALSE);

ErrorExit:
	this->Enabled = NT_SUCCESS(Status) ? TRUE : FALSE;
	return this->Enabled;
}

/**
* @brief Stop the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::Stop()
{	
	BOOLEAN retval = TRUE;
	if (FALSE == this->Enabled)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already disabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}	
	this->Enabled = FALSE;
	retval = NT_SUCCESS(this->RemoveNotify(TRUE));
ErrorExit:
	return retval;
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::IsEnabled()
{
	return this->Enabled;
}

/**
* @brief Mitigate known issues that this probe discovers
* @note Mitigation is not being called as of June 2018
* @param ArgV array of arguments
* @param ArgC argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS ProcessCreateProbe::Mitigate(
	UINT32 ArgC,
	ANSI_STRING ArgV[])
{	

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING ucvalue = { 0,0, NULL };
	ULONG pid = 0L;
	PEPROCESS EProcess = nullptr;
	ANSI_STRING MitigationCommand = { 0,0,0 };
	ANSI_STRING MitigationData = { 0,0,0 };
	HANDLE ProcessHandle = INVALID_HANDLE_VALUE;
	ACCESS_MASK AccessMask = (DELETE | SYNCHRONIZE | GENERIC_ALL);
	OBJECT_ATTRIBUTES ObjectAttributes = { 0,0,0,0,0,0 };
	CLIENT_ID ClientId = { 0,0 };

	if (ArgC != 2 || 0 != RtlCompareString(&one_shot_kill, &ArgV[0], TRUE)
		|| ArgV[1].Length < 1
		|| ArgV[1].MaximumLength < 1
		|| nullptr == ArgV[1].Buffer)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "Invalid Mitigation Data!\n");
		goto ErrorExit;
	}
	MitigationCommand = ArgV[0];
	MitigationData = { ArgV[1].Length, ArgV[1].MaximumLength, ArgV[1].Buffer };

	Status = RtlAnsiStringToUnicodeString(&ucvalue, &MitigationData, TRUE);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"Failed to allocate memory for ansi to unicode conversion for %w - error: 0x%08x\n", MitigationData, Status);
		goto ErrorExit;
	}

	__try
	{
		Status = RtlUnicodeStringToInteger(&ucvalue, 0, &pid);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
				"Failed to convert from string %wZ to native ulong format - error: 0x%08x\n", ucvalue, Status);
			__leave;
		}
	}
	__finally { RtlFreeUnicodeString(&ucvalue); }			

	Status = PsLookupProcessByProcessId((HANDLE)pid, &EProcess);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "Unable to retrieve EProcess from PID 0x%08x - returned 0x%08x!\n", pid, Status);
		goto ErrorExit;
	}
	UNREFERENCED_PARAMETER(EProcess);

	ClientId = { (HANDLE)pid, (HANDLE)0 };
	InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	Status = ZwOpenProcess(&ProcessHandle, AccessMask, &ObjectAttributes, &ClientId);			
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "Call to ZwOpenProcess failed with Status 0x%08x!\n", Status);
		goto DerefAndExit;
	}

	Status = ZwTerminateProcess(ProcessHandle, STATUS_UNSUCCESSFUL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "Unable ZwTerminate 0x%08x - returned 0x%08x!\n", ProcessHandle, Status);
		goto DerefAndExit;
	}
	Status = ZwClose(ProcessHandle);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "Unable Cloase 0x%08x - returned 0x%08x!\n", ProcessHandle, Status);
	}
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully terminated EProcess 0x%p, PID 0x%08x with Status = STATUS_UNSUCCESSFUL\n", EProcess, pid);			

	Status = STATUS_SUCCESS;

DerefAndExit:

	ObDereferenceObject(EProcess);

ErrorExit:

	return Status;
}

/**
* @brief - Process Notification Callback.
* @note The operating system calls the driver's process-notify routine
* at PASSIVE_LEVEL inside a critical region with normal kernel APCs disabled.
* When a process is created, the process-notify routine runs in the context
* of the thread that created the new process. When a process is deleted, the
* process-notify routine runs in the context of the last thread to exit from
* the process.
* @param Process  - a pointer to the new processes EPROCESS structure
* @param ProcessId  - the new processes process id
* @param CreateInfo  - provides information about a newly created process
*/
_Use_decl_annotations_
VOID 
ProcessCreateProbe::ProcessNotifyCallbackEx(
	PEPROCESS Process, 
	HANDLE ProcessId, 
	const PPS_CREATE_NOTIFY_INFO CreateInfo)
{	
	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	WVUQueueManager::ProbeInfo* pProbeInfo = WVUQueueManager::GetInstance().FindProbeByName(probe_name);
	if (NULL == pProbeInfo)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
			"***** Unable to find probe info on probe %w!\n", &probe_name);
		goto ErrorExit;
	}

	if (CreateInfo)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
			"***** Process Created: Image File Name=%wZ, EPROCESS=%p, ProcessId=%p\n",
			CreateInfo->ImageFileName, Process, ProcessId);
		pPCP->InsertProcessEntry(Process, ProcessId);
		const USHORT bufsz = ROUND_TO_SIZE(sizeof(ProcessCreateInfo) + CreateInfo->CommandLine->Length, 0x10);
#pragma warning(suppress: 6014)  // we allocate memory, put stuff into, enqueue it and return we do leak!
		const auto buf = new UCHAR[bufsz];
		if (NULL == buf)
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for ProcessCreateInfo Data!\n");
			goto ErrorExit;
		}
		RtlSecureZeroMemory(buf, bufsz);
		const PProcessCreateInfo pPCI = (PProcessCreateInfo)buf;
		KeQuerySystemTimePrecise(&pPCI->ProbeDataHeader.current_gmt);
		pPCI->ProbeDataHeader.probe_type = ProbeType::ProcessCreate;
		pPCI->ProbeDataHeader.probe_id = pProbeInfo->Probe->GetProbeId();
		pPCI->ProbeDataHeader.data_sz = bufsz;
		pPCI->EProcess = Process;
		pPCI->ProcessId = ProcessId;
		pPCI->ParentProcessId = CreateInfo->ParentProcessId;
		pPCI->CreatingThreadId.UniqueProcess = CreateInfo->CreatingThreadId.UniqueProcess;
		pPCI->CreatingThreadId.UniqueThread = CreateInfo->CreatingThreadId.UniqueThread;
		pPCI->FileObject = CreateInfo->FileObject;
		pPCI->CreationStatus = CreateInfo->CreationStatus;
		pPCI->CommandLineSz = CreateInfo->CommandLine->Length;
		RtlMoveMemory(&pPCI->CommandLine[0], CreateInfo->CommandLine->Buffer, pPCI->CommandLineSz);
		
		if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pPCI->ProbeDataHeader.ListEntry))
		{
#pragma warning(suppress: 26407)
			delete[] buf;
			WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
				"***** Process Created Enqueue Operation Failed: Image File Name=%wZ, EPROCESS=%p, ProcessId=%p\n",
				CreateInfo->ImageFileName, Process, ProcessId);
		}
	}
	else
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
			"***** Process Destroyed: EPROCESS %p, ProcessId %p, \n",
			Process, ProcessId);
		ProcessEntry* pProcEntry = pPCP->FindProcessByEProcess(Process);
		if (NULL != pProcEntry)
		{
			pPCP->RemoveProcessEntry(pProcEntry);
		}
		const USHORT bufsz = ROUND_TO_SIZE(sizeof(ProcessDestroyInfo), 0x10);
		auto const buf = new UCHAR[bufsz];
		if (NULL == buf)
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for ProcessDestroyInfo Data!\n");
			goto ErrorExit;
		}
		RtlSecureZeroMemory(buf, bufsz);
		const PProcessDestroyInfo pPDI= (PProcessDestroyInfo)buf;
		KeQuerySystemTimePrecise(&pPDI->ProbeDataHeader.current_gmt);
		pPDI->ProbeDataHeader.probe_type = ProbeType::ProcessDestroy;
		pPDI->ProbeDataHeader.probe_id = pProbeInfo->Probe->GetProbeId();
		pPDI->ProbeDataHeader.data_sz = bufsz;
		pPDI->EProcess = Process;
		pPDI->ProcessId = ProcessId;	
		
		if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pPDI->ProbeDataHeader.ListEntry))
		{
#pragma warning(suppress: 26407)
			delete[] buf;
			WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
				"***** Process Destroyed Enqueue Operation Failed: EPROCESS %p, ProcessId %p, \n",
				Process, ProcessId);
		}
	}

	if (NULL != pProbeInfo && NULL != pProbeInfo->Probe)
	{
		pProbeInfo->Probe->IncrementOperationCount();
		KeQuerySystemTimePrecise(&pProbeInfo->Probe->GetLastProbeRunTime()); 
	}

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);

	return VOID();
}

/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS 
ProcessCreateProbe::OnRun() 
{	
	NTSTATUS Status = STATUS_SUCCESS;
	Status = AbstractVirtueProbe::OnRun();
	return Status;
}


/**
* @brief finds a process by eprocess
* @param eprocess  The eprocess to find
* @return PEPROCESS if found else NULL
*/
_Use_decl_annotations_
ProcessCreateProbe::ProcessEntry*
ProcessCreateProbe::FindProcessByEProcess(PEPROCESS pEProcess)
{
	ProcessCreateProbe::ProcessEntry* retVal = nullptr;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };

	KeAcquireInStackQueuedSpinLock(&this->ProcessListSpinLock, &LockHandle);
	__try
	{
		LIST_FOR_EACH(pProcessEntry, this->ProcessList, ProcessEntry)
		{
			if (pProcessEntry->pEProcess == pEProcess)
			{
				retVal = pProcessEntry;
				__leave;
			}
		}
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }
	return retVal;
}

/**
* @brief finds a process by process id
* @param ProcessId  The Process Id to find
* @return PEPROCESS if found else NULL
*/
_Use_decl_annotations_
ProcessCreateProbe::ProcessEntry*
ProcessCreateProbe::FindProcessByProcessId(HANDLE ProcessId)
{
	ProcessCreateProbe::ProcessEntry* retVal = nullptr;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	
	KeAcquireInStackQueuedSpinLock(&this->ProcessListSpinLock, &LockHandle);
	__try
	{
		LIST_FOR_EACH(pProcessEntry, this->ProcessList, ProcessEntry)
		{
			if (pProcessEntry->ProcessId == ProcessId)
			{
				retVal = pProcessEntry;
				__leave;
			}
		}
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }
	return retVal;
}

/**
* @brief finds a process by process id
* @param ProcessId  The Process Id to find
* @return PEPROCESS if found else NULL
*/
_Use_decl_annotations_
BOOLEAN
ProcessCreateProbe::InsertProcessEntry(PEPROCESS pEProcess, HANDLE ProcessId)
{
	BOOLEAN success = FALSE;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	ProcessCreateProbe::ProcessEntry* pProcEntry =
		(ProcessCreateProbe::ProcessEntry*)new BYTE[sizeof ProcessCreateProbe::ProcessEntry];
	if (NULL == pProcEntry)
	{
		success = FALSE;
		goto ErrorExit;
	}

	pProcEntry->pEProcess = pEProcess;
	pProcEntry->ProcessId = ProcessId;

	KeAcquireInStackQueuedSpinLock(&this->ProcessListSpinLock, &LockHandle);
	__try
	{
		InsertTailList(&this->ProcessList, &pProcEntry->ListEntry);
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }

	success = TRUE;

ErrorExit:

	return success;
}

/**
* @brief finds a process by process id
* @param ProcessId  The Process Id to find
* @return PEPROCESS if found else NULL
*/
_Use_decl_annotations_
BOOLEAN
ProcessCreateProbe::RemoveProcessEntry(ProcessCreateProbe::ProcessEntry* pProcessEntry)
{
	BOOLEAN success = FALSE;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };

	KeAcquireInStackQueuedSpinLock(&this->ProcessListSpinLock, &LockHandle);
	__try
	{
		LIST_FOR_EACH(pLstProcEntry, this->ProcessList, ProcessEntry)
		{
			if (pLstProcEntry->ProcessId == pProcessEntry->ProcessId)
			{
				RemoveEntryList(&pLstProcEntry->ListEntry);
				delete[] (PBYTE)pLstProcEntry;
				success = TRUE;
				__leave;
			}
		}		
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }	

	return success;
}

