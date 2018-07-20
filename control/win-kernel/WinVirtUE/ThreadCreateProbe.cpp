#include "ThreadCreateProbe.h"
#define COMMON_POOL_TAG WVU_IMAGELOADPROBE_POOL_TAG

static ANSI_STRING probe_name = RTL_CONSTANT_STRING("ThreadCreate");

/**
* @brief construct an instance of this probe
*/
ThreadCreateProbe::ThreadCreateProbe() : AbstractVirtueProbe(probe_name)
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
}


/**
* @brief Start the ThreadCreateProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ThreadCreateProbe::Start()
{
	NTSTATUS Status = PsSetCreateThreadNotifyRoutine(ThreadCreateProbe::ThreadCreateCallback);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID, "PsSetCreateThreadNotifyRoutine(ThreadCreateCallback) "
			"Add Failed! Status=%08x\n", Status);	
		this->Enabled = FALSE;
		goto ErrorExit;
	}
	this->Enabled = TRUE;
ErrorExit:
	return this->Enabled;
}

/**
* @brief Stop the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ThreadCreateProbe::Stop()
{
	NTSTATUS Status = PsRemoveCreateThreadNotifyRoutine(ThreadCreateProbe::ThreadCreateCallback);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID, "PsRemoveCreateThreadNotifyRoutine (ThreadCreateCallback) "
			"Remove Failed! Status=%08x\n", Status);
		this->Enabled = FALSE;
		goto ErrorExit;
	}
	
	this->Enabled = TRUE;
ErrorExit:
	return this->Enabled;
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ThreadCreateProbe::IsEnabled()
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
NTSTATUS ThreadCreateProbe::Mitigate(
	UINT32 ArgC,
	ANSI_STRING ArgV[])
{
	UNREFERENCED_PARAMETER(ArgV);
	UNREFERENCED_PARAMETER(ArgC);
	return NTSTATUS();
}


/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS
ThreadCreateProbe::OnRun()
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = AbstractVirtueProbe::OnRun();
	return Status;
}
/**
* @brief The Thread Create Notify Callback Routine
* @param ProcessId  the new threads process id
* @param ThreadId this new threads thread id
* @param Create TRUE if thread was created else FALSE it was terminated
*/
_Use_decl_annotations_
VOID 
ThreadCreateProbe::ThreadCreateCallback(
	HANDLE ProcessId,
	HANDLE ThreadId,
	BOOLEAN Create)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PVOID StartAddress = nullptr;
	PVOID Win32StartAddress = nullptr;
	ThreadBehavior ThdBeh = (ThreadBehavior)0;
	PETHREAD pThread = nullptr;

	UNREFERENCED_PARAMETER(ProcessId);
	
	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	const USHORT bufsz = TRUE == Create ? sizeof(ThreadCreateInfo) : sizeof(ThreadDestroyInfo);
#pragma warning(suppress: 6014)  // we allocate memory, put stuff into, enqueue it and return we do leak!
	const auto buf = new UCHAR[bufsz];
	if (NULL == buf)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for Thread State Data!\n");
		goto ErrorExit;
	}
	RtlSecureZeroMemory(buf, bufsz);

	WVUQueueManager::ProbeInfo* pProbeInfo = WVUQueueManager::GetInstance().FindProbeByName(probe_name);
	if (NULL == pProbeInfo)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
			"***** Unable to find probe info on probe %w!\n", &probe_name);
		goto ErrorExit;
	}
	
	if (TRUE == Create)
	{
		Status = PsLookupThreadByThreadId(ThreadId, &pThread);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID,
				"Failed to grab thread reference for ProcessId 0x%08 ThreadId 0x%08 - Status=0x%08x\n", Status);
			goto ErrorExit;
		}
		__try
		{
			switch (Globals.lpVersionInformation.dwBuildNumber)
			{
			case 14393:
				StartAddress = (PVOID)*((PULONG_PTR)Add2Ptr(pThread, 0x608));
				Win32StartAddress = (PVOID)*((PULONG_PTR)Add2Ptr(pThread, 0x688));
				ThdBeh = (ThreadBehavior)*((PBYTE)Add2Ptr(pThread, 0x6c8));
				break;
			case 16299:
				StartAddress = (PVOID)*((PULONG_PTR)Add2Ptr(pThread, 0x610));
				Win32StartAddress = (PVOID)*((PULONG_PTR)Add2Ptr(pThread, 0x690));
				ThdBeh = (ThreadBehavior)*((PBYTE)Add2Ptr(pThread, 0x6d8));
				break;
			default:
				StartAddress = nullptr;
				Win32StartAddress = nullptr;
				ThdBeh = (ThreadBehavior)0;
				WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID,
					"Failed to retrieve Thread Data for Version 0x%08 - Status=0x%08x\n",
					Globals.lpVersionInformation.dwBuildNumber, Status);
				goto ErrorExit;
			}
		}
		__finally
		{
			if (FALSE == AbnormalTermination())
			{
				WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, TRACE_LEVEL_ID,
					"Process Id 0x%08 Thread Id 0x%08 at EProcess 0x%p was %s to run at start address 0x%p\n",
					pThread, ProcessId, Create ? "Created" : "Terminated", StartAddress);
			}
			ObDereferenceObject(pThread);
		}
		const PThreadCreateInfo pThreadCreateInfo = (PThreadCreateInfo)buf;
		pThreadCreateInfo->ProbeDataHeader.probe_type = ProbeType::ThreadCreate;
		pThreadCreateInfo->ProbeDataHeader.data_sz = bufsz;
		pThreadCreateInfo->ProbeDataHeader.probe_id = pProbeInfo->Probe->GetProbeId();
		KeQuerySystemTimePrecise(&pThreadCreateInfo->ProbeDataHeader.current_gmt);
		pThreadCreateInfo->ProcessId = ProcessId;
		pThreadCreateInfo->ThreadId = ThreadId;
		pThreadCreateInfo->StartAddress = StartAddress;
		pThreadCreateInfo->Win32StartAddress = Win32StartAddress;
		pThreadCreateInfo->IsStartAddressValid =
			((ThdBeh & ThreadBehavior::StartAddressInvalid) == ThreadBehavior::StartAddressInvalid)
			? TRUE
			: FALSE;

		if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pThreadCreateInfo->ProbeDataHeader.ListEntry))
		{
#pragma warning(suppress: 26407)
			delete[] buf;
			WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID,
				"***** Thread Create Enqueue Operation Failed: ProcessId 0x%08 ThreadId 0x%08 was Created "
				"to execute on StartAddress 0x%p, Win32StartAddress = %p w/Valid Start Address = %s\n",
				ProcessId, ThreadId, pThreadCreateInfo->StartAddress, pThreadCreateInfo->Win32StartAddress,
				pThreadCreateInfo->IsStartAddressValid ? "TRUE" : "FALSE");
		}
	}
	else
	{
		const PThreadDestroyInfo pThreadDestroyInfo = (PThreadDestroyInfo)buf;
		pThreadDestroyInfo->ProbeDataHeader.probe_type = ProbeType::ThreadCreate;
		pThreadDestroyInfo->ProbeDataHeader.data_sz = bufsz;
		pThreadDestroyInfo->ProbeDataHeader.probe_id = pProbeInfo->Probe->GetProbeId();
		KeQuerySystemTimePrecise(&pThreadDestroyInfo->ProbeDataHeader.current_gmt);
		pThreadDestroyInfo->ProcessId = ProcessId;
		pThreadDestroyInfo->ThreadId = ThreadId;

		if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pThreadDestroyInfo->ProbeDataHeader.ListEntry))
		{
#pragma warning(suppress: 26407)
			delete[] buf;
			WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID,
				"***** Thread Destroy Enqueue Operation Failed: ProcessId 0x%08 ThreadId 0x%08\n",
				ProcessId, ThreadId);
		}
	}

	if (NULL != pProbeInfo && NULL != pProbeInfo->Probe)
	{
		pProbeInfo->Probe->IncrementOperationCount();
		KeQuerySystemTimePrecise(&pProbeInfo->Probe->GetLastProbeRunTime());
	}
	Status = STATUS_SUCCESS;

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);

	return;
}