/**
* @file ProcessListValidationProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessListValidationProbe Probe Class definition
*/
#include "ProcessListValidationProbe.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_PROCLISTVALIDPROBE_POOL_TAG

/**
* @brief construct an instance of this probe
*/
ProcessListValidationProbe::ProcessListValidationProbe() :
	AbstractVirtueProbe(RTL_CONSTANT_STRING("ProcessListValidation"))
{
	Attributes = (ProbeAttributes)(ProbeAttributes::Temporal | ProbeAttributes::EnabledAtStart);
}

/**
* @brief called to configure the probe
* @note currently not implmented, placeholder code
* @param NameValuePairs newline terminated with assign operator name value
* pair configuration information
*/
_Use_decl_annotations_
BOOLEAN 
ProcessListValidationProbe::Configure(_In_ const ANSI_STRING & NameValuePairs)
{
	UNREFERENCED_PARAMETER(NameValuePairs);
	return BOOLEAN();
}

/**
* @brief Start the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Start()
{
	this->Enabled = TRUE;
	return TRUE;
}

/**
* @brief Stop the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Stop()
{
	this->Enabled = FALSE;
	return TRUE;
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::IsEnabled()
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
	NTSTATUS ReportStatus = STATUS_SUCCESS;
	PEPROCESS Process = nullptr;
	HANDLE ProcessId = INVALID_HANDLE_VALUE;
	KLOCK_QUEUE_HANDLE LockHandle = { {NULL,NULL},0 };

	if (FALSE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %Z already disabled - not running polled operation!\n", &this->ProbeName);
		goto ErrorExit;
	}

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
			Process = pProcessEntry->pEProcess;  // in case we need to generate a report 
			ProcessId = PsGetProcessId(pProcessEntry->pEProcess);
			if (INVALID_HANDLE_VALUE == ProcessId || ProcessId != pProcessEntry->ProcessId)
			{
				RemoveEntryList(&pProcessEntry->ListEntry);    // we know its invalid, remove it and notify!
				ReportStatus = STATUS_NOT_FOUND;   // the process id was not found - something fishy is going on
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

	if (FALSE == NT_SUCCESS(ReportStatus))
	{
		// MFS - create the WVUProbeManager class that is charge of init/fini of probes and receives notifictions
		// Notify the WVUProbeManager that the Sensor is in an Alarm State.  User Space Program MUST acknowledge.
		// Normal, UnAcknowledged Alarm State, Alarm State, 
		PProcessListValidationFailed pPLVF = (PProcessListValidationFailed)new BYTE[sizeof ProcessListValidationFailed];
		if (NULL == pPLVF)
		{
			Status = STATUS_MEMORY_NOT_ALLOCATED;   // the process id was not found - something fishy is going on
			WVU_DEBUG_PRINT(LOG_PROCESS, ERROR_LEVEL_ID, "Unable to allocate memory for ProcessListValidationFailed object - FAILED: 0x%08x \n", Status);
			goto ErrorExit;
		}
		pPLVF->Status = ReportStatus;	    // tell the user space program what happened
		pPLVF->EProcess = Process;			// suspect data, this process does not exist in the OS process list
		pPLVF->ProcessId = ProcessId;		// suspect data, this pid does not exist in the OS process list
		pPLVF->ProbeDataHeader.ProbeId = ProbeIdType::ProcessListValidation;  // this is as temporal probe report		
		pPLVF->ProbeDataHeader.DataSz = sizeof(ProcessListValidationFailed);
		KeQuerySystemTimePrecise(&pPLVF->ProbeDataHeader.CurrentGMT);
		
		if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pPLVF->ProbeDataHeader.ListEntry))
		{
#pragma warning(suppress: 26407)
			delete[] pPLVF;
			WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
				"***** Temporal Probe Report Failed to Notify that EPROCESS=%p, ProcessId=%p do NOT exist in OS Process List!\n",
				Process, ProcessId);
		}
	}

ErrorExit:

	return Status;
}

