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
		Status = KeWaitForSingleObject(WVUMainThreadStartEvt, KWAIT_REASON::Executive, KernelMode, FALSE, (PLARGE_INTEGER)0);
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
* @brief probe communcations thread.  this thread drains the sensor queue by
* sending probe data to the user space service.
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUSensorThread(PVOID StartContext)
{
	UNREFERENCED_PARAMETER(StartContext);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PSaviorCommandPkt pSavCmdPkt = NULL;
	LARGE_INTEGER timeout = { 0LL };
	timeout.QuadPart = -1000 * 1000 * 10 * 30;  // ten second timeout
	ULONG SenderBufferLen = sizeof(SaviorCommandPkt);
	ULONG ReplyBufferLen = 64;
	PUCHAR ReplyBuffer = NULL;

	FLT_ASSERTMSG("WVUSensorThread must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);

	__debugbreak();

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	pSavCmdPkt = (PSaviorCommandPkt)ALLOC_POOL(PagedPool, SenderBufferLen);
	if (NULL == pSavCmdPkt)
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "ALLOC_POOL(SaviorCommandPkt) "
			"Memory Allocation Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}
	pSavCmdPkt->Cmd = SaviorCommand::ECHO;
	pSavCmdPkt->MessageId = 0;
	pSavCmdPkt->ReplyLength = ReplyBufferLen;
	pSavCmdPkt->CmdMsgSize = 1;
	pSavCmdPkt->CmdMsg[0] = 'X';
	ReplyBuffer = (PUCHAR)ALLOC_POOL(PagedPool, ReplyBufferLen);
	if (NULL == ReplyBuffer)
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "ALLOC_POOL(ReplyBuffer) "
			"Memory Allocation Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}


	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Acquired rundown protection . . .\n");

	do
	{
		Status = FltSendMessage(Globals.FilterHandle, &Globals.ClientPort,
			pSavCmdPkt, SenderBufferLen, ReplyBuffer, &ReplyBufferLen, &timeout);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "FltSendMessage "
				"(...) Message Send Failed! Status=%08x\n", Status);
		}
		else
		{
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "FltSendMessage(...) Succeeded! . . .\n");
			pSavCmdPkt->MessageId++;
		}
	} while (FALSE == Globals.ShuttingDown);


ErrorExit:
	if (NULL != pSavCmdPkt)
	{
		FREE_POOL(pSavCmdPkt);
	}

	if(NULL != ReplyBuffer)
	{
		FREE_POOL(ReplyBuffer);
	}

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Exiting WVUSensorThread Thread w/Status=0x%08x!\n", Status);
	return;
}
