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

_Use_decl_annotations_
VOID
WVUMainThreadStart(PVOID  StartContext)
{
	PKEVENT WVUMainThreadStartEvt = (PKEVENT)StartContext;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LONG Signaled = 0;

	FLT_ASSERTMSG("WVUMainThreadStart must run at IRQL <= PASSIVE!", KeGetCurrentIrql() <= PASSIVE_LEVEL);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "Acquired runndown protection . . .\n");

	Status = PsSetLoadImageNotifyRoutine(ImageLoadNotificationRoutine);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, ERROR_LEVEL_ID, "PsSetLoadImageNotifyRoutine(ImageLoadNotificationRoutine) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	Status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, FALSE);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, FALSE) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	Status = PsSetCreateThreadNotifyRoutine(ThreadCreateCallback);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, ERROR_LEVEL_ID, "PsSetCreateThreadNotifyRoutine(ThreadCreateCallback) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}
	
	Cookie.QuadPart = (LONGLONG)Globals.DriverObject;
	Status = CmRegisterCallbackEx(RegistryModificationCB, &WinVirtUEAltitude, Globals.DriverObject, NULL, &Cookie, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, ERROR_LEVEL_ID, "CmRegisterCallbackEx(...) failed with Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE) . . .\n");
#pragma warning(suppress: 28160) // stupid warning about the wait arg TRUE . . . sheesh
	Signaled = KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE);
	do
	{
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeWaitForSingleObject(WVUMainThreadStart, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		Status = KeWaitForSingleObject(WVUMainThreadStartEvt, KWAIT_REASON::Executive, KernelMode, FALSE, (PLARGE_INTEGER)0);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Failed! Status=%08x\n", Status);
			goto ErrorExit;
		}
		WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "Returned from KeWaitForSingleObject(WVUMainThreadStart, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		switch (Status)
		{
		case STATUS_SUCCESS:
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Returned SUCCESS - Exiting!\n");
			break;
		case STATUS_ALERTED:
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Was Just Alerted - Waiting Again!\n");
			break;
		case STATUS_USER_APC:
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Had An APC Delievered - Waiting Again!\n");
			break;
		case STATUS_TIMEOUT:
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Timed Out - Exiting!\n");
			break;
		default:
			WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Received Status=0x%08x - Exiting!\n", Status);
			break;
		}
	} while (Status == STATUS_ALERTED || Status == STATUS_USER_APC);  // don't bail if we get alerted or APC'd

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_WVU_MAINTHREAD, TRACE_LEVEL_ID, "Exiting Thread w/Status=0x%08x!\n", Status);
	return;
}