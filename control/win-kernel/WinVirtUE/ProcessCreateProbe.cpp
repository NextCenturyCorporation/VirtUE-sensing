/**
* @file ProcessCreateProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessCreateProbe Probe Class definition
*/
#include "ProcessCreateProbe.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_PROCESSCTORDTORPROBE_POOL_TAG

/**
* @brief construct an instance of this probe 
*/
ProcessCreateProbe::ProcessCreateProbe() : 
	AbstractVirtueProbe(RTL_CONSTANT_STRING("ProcessCreate"))
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);

	// initialize the spinlock that controls access to the Response queue
	KeInitializeSpinLock(&this->ProcessListSpinLock);

	// initialize the Response Queue TWLL
	InitializeListHead(&this->ProcessList);

	// we build the queue successfully
	this->Enabled = TRUE;
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

/**
* @brief called to configure the probe
* @note currently not implmented, placeholder code
* @param NameValuePairs newline terminated with assign operator name value
* pair configuration information
*/
_Use_decl_annotations_
BOOLEAN 
ProcessCreateProbe::Configure(_In_ const ANSI_STRING& NameValuePairs)
{
	UNREFERENCED_PARAMETER(NameValuePairs);
	return BOOLEAN();
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
	if ((Attributes & ProbeAttributes::EnabledAtStart) != ProbeAttributes::EnabledAtStart)
	{
		Status = STATUS_NOT_SUPPORTED;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %Z not enabled at start - probe is registered but not active\n",
			&this->ProbeName);
		goto ErrorExit;
	}
	Status = this->RemoveNotify(FALSE);
ErrorExit:
	this->Enabled = NT_SUCCESS(Status) ? TRUE : FALSE;
	return NT_SUCCESS(Status) ? TRUE : FALSE;
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
		goto ErrorExit;
	}	
	retval = NT_SUCCESS(this->RemoveNotify(TRUE));
	this->Enabled = !retval;
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
* @param argv array of arguments
* @param argc argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS ProcessCreateProbe::Mitigate(
	PCHAR argv[], 
	UINT32 argc)
{
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(argc);
	return NTSTATUS();
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
		pPCI->ProbeDataHeader.MessageId = 0LL;
		pPCI->ProbeDataHeader.ReplyLength = 0L;
		KeQuerySystemTimePrecise(&pPCI->ProbeDataHeader.CurrentGMT);
		pPCI->ProbeDataHeader.ProbeId = ProbeIdType::ProcessCreate;
		pPCI->ProbeDataHeader.DataSz = bufsz;
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
		KeQuerySystemTimePrecise(&pPDI->ProbeDataHeader.CurrentGMT);
		pPDI->ProbeDataHeader.MessageId = 0LL;
		pPDI->ProbeDataHeader.ReplyLength = 0L;
		pPDI->ProbeDataHeader.ProbeId = ProbeIdType::ProcessDestroy;
		pPDI->ProbeDataHeader.DataSz = bufsz;
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

