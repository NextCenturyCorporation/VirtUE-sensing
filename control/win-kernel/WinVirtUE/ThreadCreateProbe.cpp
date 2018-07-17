#include "ThreadCreateProbe.h"
#define COMMON_POOL_TAG WVU_IMAGELOADPROBE_POOL_TAG

static ANSI_STRING probe_name = RTL_CONSTANT_STRING("ThreadCreate");

ThreadCreateProbe::ThreadCreateProbe() : AbstractVirtueProbe(probe_name)
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
}


ThreadCreateProbe::~ThreadCreateProbe()
{
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
* @param argv array of arguments
* @param argc argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS ThreadCreateProbe::Mitigate(
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
	PETHREAD pThread = nullptr;

	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ThreadId);
	UNREFERENCED_PARAMETER(Create);

	Status = PsLookupThreadByThreadId(ThreadId, &pThread);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, ERROR_LEVEL_ID,
			"Failed to grab thread reference, Status=0x%08x\n", Status);
		goto ErrorExit;
	}	
	__try
	{
		/** TODO: always retrieve the correct offset per version: Version 1807 */
		StartAddress = Add2Ptr(pThread, 0x610);
	}
	__finally
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, TRACE_LEVEL_ID,
			"Process Id 0x%08 Thread Id 0x%08 at EProcess 0x%p was %s to run at start address 0x%p\n",
			pThread, ProcessId, Create ? "Created" : "Terminated", StartAddress);
		ObDereferenceObject(pThread);
	}

ErrorExit:

	return;
}