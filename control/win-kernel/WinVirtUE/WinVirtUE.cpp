/**
* @file WinVirtUe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup
*/
#include "WinVirtUE.h"

#define COMMON_POOL_TAG WVU_OBSIDIANWAVE_POOL_TAG

static const UNICODE_STRING WinVirtUEAltitude = RTL_CONSTANT_STRING(L"360000");
static LARGE_INTEGER Cookie;

/**
* @brief Main initialization thread.
* @note this thread remains active through out the lifetime of the driver
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUMainThreadStart(PVOID StartContext)
{
	PKEVENT WVUMainThreadStartEvt = (PKEVENT)StartContext;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LONG Signaled = 0;

	FLT_ASSERTMSG("WVUMainThreadStart must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);	

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Acquired runndown protection . . .\n");

	Status = PsSetLoadImageNotifyRoutine(ImageLoadNotificationRoutine);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "PsSetLoadImageNotifyRoutine(ImageLoadNotificationRoutine) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	Status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, FALSE);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, FALSE) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	Status = PsSetCreateThreadNotifyRoutine(ThreadCreateCallback);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateThreadNotifyRoutine(ThreadCreateCallback) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}
	
	Cookie.QuadPart = (LONGLONG)Globals.DriverObject;
	Status = CmRegisterCallbackEx(RegistryModificationCB, &WinVirtUEAltitude, Globals.DriverObject, NULL, &Cookie, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "CmRegisterCallbackEx(...) failed with Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE) . . .\n");
#pragma warning(suppress: 28160) // stupid warning about the wait arg TRUE . . . sheesh
	Signaled = KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE);
	do
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeWaitForSingleObject(WVUMainThreadStart, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		Status = KeWaitForSingleObject(WVUMainThreadStartEvt, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Failed! Status=%08x\n", Status);
#pragma warning(suppress: 26438)
			goto ErrorExit;
		}
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Returned from KeWaitForSingleObject(WVUMainThreadStart, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		switch (Status)
		{
		case STATUS_SUCCESS:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Returned SUCCESS - Exiting!\n");
			break;
		case STATUS_ALERTED:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Was Just Alerted - Waiting Again!\n");
			break;
		case STATUS_USER_APC:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Had An APC Delievered - Waiting Again!\n");
			break;
		case STATUS_TIMEOUT:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Timed Out - Exiting!\n");
			break;
		default:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Received Status=0x%08x - Exiting!\n", Status);
			break;
		}
	} while (Status == STATUS_ALERTED || Status == STATUS_USER_APC);  // don't bail if we get alerted or APC'd

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Exiting Thread w/Status=0x%08x!\n", Status);
	return;
}

/**
* @brief probe communcations thread.  this thread drains the sensor queue by0j
* sending probe data to the user space service.
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUSensorThread(PVOID StartContext)
{
	UNREFERENCED_PARAMETER(StartContext);
	const ULONG REPLYLEN = 128;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER timeout = { 0LL };
	timeout.QuadPart = -1000 * 1000 * 10 * 5;  // five second timeout
	ULONG SenderBufferLen = sizeof(SaviorCommandPkt);
	ULONG ReplyBufferLen = REPLYLEN;
	PUCHAR ReplyBuffer = NULL;
	PVOID SenderBuffer = NULL;

	FLT_ASSERTMSG("WVUSensorThread must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);

	WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "Acquired rundown protection . . .\n");
	
	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	ReplyBuffer = (PUCHAR)ALLOC_POOL(NonPagedPool, REPLYLEN);
	if (NULL == ReplyBuffer)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "Unable ato allocate from NonPagedPool! Status=%08x\n", Status);
		goto ErrorExit;
	}

	do
	{		
		Status = KeWaitForMultipleObjects(NUMBER_OF(Globals.ProbeDataEvents), (PVOID*)&Globals.ProbeDataEvents[0], WaitAll, Executive, KernelMode, FALSE, (PLARGE_INTEGER)0, NULL);
		WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "Probe Data Queue Semaphore Read State = %ld\n", KeReadStateSemaphore((PRKSEMAPHORE)Globals.ProbeDataEvents[ProbeDataSemEmptyQueue]));
		if (FALSE == NT_SUCCESS(Status) && FALSE == Globals.ShuttingDown)
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "KeWaitForMultipleObjects(Globals.ProbeDataEvents,...) Failed! Status=%08x\n", Status);
			goto ErrorExit;
		}

		// If the list is empty and we are shutting down then break out
		if (TRUE == IsListEmpty(&Globals.ProbeDataQueue) && TRUE == Globals.ShuttingDown)
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, WARNING_LEVEL_ID, "IsListEmpty(Globals.ProbeDataQueue,...) returned an empty queue?!\n");
			goto ErrorExit;
		}

		// quickly remove a queued entry
		PLIST_ENTRY pListEntry = ExInterlockedRemoveHeadList(&Globals.ProbeDataQueue, &Globals.ProbeDataQueueSpinLock);
		
		// all of these machinations below are required because the python side requires that FILTER_MESSAGE_HEADER
		// be the first type in the structure defintion.  We first get the address of ProbeDataHeader, add the offset
		// equal to the size filter message header and then dereference the pointer from there.  Sheesh.
		PProbeDataHeader pPDH = CONTAINING_RECORD(pListEntry, ProbeDataHeader, ListEntry);
		PFILTER_MESSAGE_HEADER pfmh = (PFILTER_MESSAGE_HEADER)(Add2Ptr(pPDH,sizeof(FILTER_MESSAGE_HEADER)));
		pfmh->MessageId = Globals.MessageId;
		pfmh->ReplyLength = ReplyBufferLen;
		SenderBufferLen = pPDH->DataSz;
		SenderBuffer = (PVOID)pPDH;

		Status = FltSendMessage(Globals.FilterHandle, &Globals.ClientPort,
			SenderBuffer, SenderBufferLen, ReplyBuffer, &ReplyBufferLen, &timeout);

		if (FALSE == NT_SUCCESS(Status) || STATUS_TIMEOUT == Status) 
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "FltSendMessage "
				"(...) Message Send Failed - enqueue and wait! Status=%08x - waiting\n", Status);
			(VOID)ExInterlockedInsertHeadList(&Globals.ProbeDataQueue, pListEntry, &Globals.ProbeDataQueueSpinLock);
			KeReleaseSemaphore((PRKSEMAPHORE)Globals.ProbeDataEvents[ProbeDataSemEmptyQueue], IO_NO_INCREMENT, 1, FALSE);  // Signaled when the Queue is not empty
			KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
		else if (TRUE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "FltSendMessage(...) Succeeded w/ReplyBufferLen = %d Messagse=%s!\n", ReplyBufferLen, ReplyBuffer);
			Globals.MessageId++;
			delete[] (PUCHAR)pPDH;
			pListEntry = NULL;
		}
		ReplyBufferLen = REPLYLEN;
		RtlSecureZeroMemory(ReplyBuffer, REPLYLEN);
	} while (FALSE == Globals.ShuttingDown);


ErrorExit:
	
	if(NULL != ReplyBuffer)
	{
		FREE_POOL(ReplyBuffer);
	}

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "Exiting WVUSensorThread Thread w/Status=0x%08x!\n", Status);
	
	return;
}
