#include "ProcessCreateProbe.h"


ProcessCreateProbe::ProcessCreateProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Constructed The Process Create Sensor\n");
}


ProcessCreateProbe::~ProcessCreateProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Destroyed The Process Create Sensor\n");
}

BOOLEAN 
ProcessCreateProbe::RemoveNotify(BOOLEAN remove)
{
	NTSTATUS Status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateProbe::ProcessNotifyCallbackEx, remove);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, %s) "
			"Failed! Status=%08x\n", TRUE ? "REMOVE" : "ADD", Status);
		goto ErrorExit;
	}

ErrorExit:

	return NT_SUCCESS(Status);
}

BOOLEAN ProcessCreateProbe::Enable()
{
	return this->RemoveNotify(FALSE);	
}
	
BOOLEAN ProcessCreateProbe::Disable()
{
	return this->RemoveNotify(TRUE);
}

BOOLEAN ProcessCreateProbe::State()
{
	return this->Enabled;
}

BOOLEAN ProcessCreateProbe::Mitigate()
{
	return BOOLEAN();
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
	PEPROCESS  Process,
	HANDLE  ProcessId,
	const PPS_CREATE_NOTIFY_INFO  CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	if (CreateInfo)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
			"***** Process Created: Image File Name=%wZ, EPROCESS=%p, ProcessId=%p\n",
			CreateInfo->ImageFileName, Process, ProcessId);

		const USHORT bufsz = ROUND_TO_SIZE(sizeof(ProcessCreateInfo->Length) + CreateInfo->CommandLine->Length, 0x10);
		const PUCHAR buf = new UCHAR[bufsz];
		if (NULL == buf)
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for LoadedImageInfo Data!\n");
			goto ErrorExit;
		}

	}
	else
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
			"***** Process Destroyed: EPROCESS %p, ProcessId %p, \n",
			Process, ProcessId);
	}



	// PProcessCreateInfo pPCI = new
ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);

	return VOID();
}