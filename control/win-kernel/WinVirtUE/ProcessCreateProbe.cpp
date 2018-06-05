/**
* @file ProcessCreateProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessCreateProbe Probe Class definition
*/
#include "ProcessCreateProbe.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_PROCESSCTORDTORPROBE_POOL_TAG

ProcessCreateProbe::ProcessCreateProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Constructed The Process Create Sensor\n");
}

ProcessCreateProbe::~ProcessCreateProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Destroyed The Process Create Sensor\n");
}

_Use_decl_annotations_
BOOLEAN 
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

	return NT_SUCCESS(Status);
}
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::Enable()
{
	return this->RemoveNotify(FALSE);
}
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::Disable()
{
	return this->RemoveNotify(TRUE);
}
_Use_decl_annotations_
BOOLEAN ProcessCreateProbe::State()
{
	return this->Enabled;
}

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

		const USHORT bufsz = ROUND_TO_SIZE(sizeof(ProcessCreateInfo) + CreateInfo->CommandLine->Length, 0x10);
		const PUCHAR buf = new UCHAR[bufsz];
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
		pPCI->ProbeDataHeader.Type = DataType::ProcessCreate;
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
		if (FALSE == pPDQ->Enqueue(&pPCI->ProbeDataHeader.ListEntry))
		{
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
		const USHORT bufsz = ROUND_TO_SIZE(sizeof(ProcessDestroyInfo), 0x10);
		const PUCHAR buf = new UCHAR[bufsz];
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
		pPDI->ProbeDataHeader.Type = DataType::ProcessDestroy;
		pPDI->ProbeDataHeader.DataSz = bufsz;
		pPDI->EProcess = Process;
		pPDI->ProcessId = ProcessId;		
		if (FALSE == pPDQ->Enqueue(&pPDI->ProbeDataHeader.ListEntry))
		{
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

