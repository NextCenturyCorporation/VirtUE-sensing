/**
* @file ProcessListValidationProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessListValidationProbe Probe Class definition
*/
#include "ProcessListValidationProbe.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_PROCLISTVALIDPROBE_POOL_TAG

/**
* @brief construct an instance of this probe
*/
ProcessListValidationProbe::ProcessListValidationProbe()
{
	ProbeName = RTL_CONSTANT_STRING(L"ProcessListValidation");
	Attributes = (ProbeAttributes)(ProbeAttributes::Temporal | ProbeAttributes::EnabledAtStart);
	if (NULL != pPDQ)
	{
		pPDQ->Register(*this);
	}
}

/**
* @brief destroy this probes instance *
*/
ProcessListValidationProbe::~ProcessListValidationProbe()
{
	if (NULL != pPDQ)
	{
		pPDQ->DeRegister(*this);
	}
}

/**
* @brief Enable the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Enable()
{
	this->Enabled = TRUE;
	return TRUE;
}

/**
* @brief Disable the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Disable()
{
	this->Enabled = FALSE;
	return TRUE;
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::State()
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
NTSTATUS ProcessListValidationProbe::Mitigate(
	PCHAR argv[],
	UINT32 argc)
{
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(argc);
	return NTSTATUS();
}


/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS
ProcessListValidationProbe::OnRun()
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS Process = nullptr;
	KLOCK_QUEUE_HANDLE LockHandle;

	if (NULL == pPCP)
	{
		FLT_ASSERTMSG("The ProcessCreateProbe Does Not Exists!", NULL != pPCP);
		Status = STATUS_NOT_CAPABLE;  // For some reason the ProcessCreateProbe is gone!
		goto ErrorExit;
	}

	KSPIN_LOCK& ProcessListSpinLock = pPCP->GetProcessListSpinLock();

	/** lock up the list */
	KeAcquireInStackQueuedSpinLock(&ProcessListSpinLock, &LockHandle);
	__try
	{
		/** for each list entry, Lookup the process by pid, and use that process to retrieve the matching pid */
		LIST_FOR_EACH(pProcessEntry, pPCP->GetProcessList(), ProcessCreateProbe::ProcessEntry)
		{		
			Status = PsLookupProcessByProcessId(pProcessEntry->ProcessId, &Process);
			if (FALSE == NT_SUCCESS(Status))
			{
				WVU_DEBUG_PRINT(LOG_PROCESS, ERROR_LEVEL_ID, "Failed to find EPROCESS %08x - FAIL=%08x\n", Process, Status);
				__leave;
			}
			HANDLE pid = PsGetProcessId(Process);
			if (INVALID_HANDLE_VALUE == pid || pid != pProcessEntry->ProcessId)
			{
				Status = STATUS_NOT_FOUND;   // the process id was not found - something fishy is going on
				WVU_DEBUG_PRINT(LOG_PROCESS, ERROR_LEVEL_ID, "EPROCESS %08x failed to retrieve the matching PID %08x!\n", Process, pProcessEntry->ProcessId);
				__leave;
			}
		}
	}
	__finally
	{
		// the last thing we do is to update the last run time
		Status = AbstractVirtueProbe::OnRun();
		KeReleaseInStackQueuedSpinLock(&LockHandle);
	}

ErrorExit:

	return Status;
}

