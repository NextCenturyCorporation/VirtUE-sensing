/**
* @file WinVirtUe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup
*/
#include "WVURuntimeThreads.h"
#include "WVUCommsManager.h"
#include "WVUProbeManager.h"

#define COMMON_POOL_TAG WVU_THREAD_POOL_TAG

static LARGE_INTEGER Cookie;
class WVUProbeManager *pWVUMgr;
#pragma region Main Initialization Thread
/**
* @brief Main initialization thread.
* @note this thread remains active through out the lifetime of the driver and is also used
* during unload to ensure all allocate C++ objects and OS resources are released as required.
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUMainInitThread(PVOID StartContext)
{
	PKEVENT WVUMainThreadStartEvt = (PKEVENT)StartContext;
	OBJECT_ATTRIBUTES SensorThdObjAttr = { 0,0,0,0,0,0 };
	OBJECT_ATTRIBUTES PollThdObjAttr = { 0,0,0,0,0,0 };	
	HANDLE SensorThreadHandle = (HANDLE)-1;
	HANDLE PollThreadHandle = (HANDLE)-1;
	CLIENT_ID SensorClientId = { (HANDLE)-1,(HANDLE)-1 };
	CLIENT_ID PollClientId = { (HANDLE)-1,(HANDLE)-1 };
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LONG Signaled = 0;
	PKTHREAD pSensorThread = NULL;
	PKTHREAD pPollThread = NULL;	

	FLT_ASSERTMSG("WVUMainInitThread must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);	
	
	/** configure the event so that automatically goes to non-signaled and start at non-signaled */
	KeInitializeEvent(&Globals.poll_wait_evt, EVENT_TYPE::SynchronizationEvent, FALSE);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);	
	
	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "WVUMainInitThread Acquired runndown protection . . .\n");

	WVUProbeManager& instance = WVUProbeManager::GetInstance();  // touch off the probe manager
	UNREFERENCED_PARAMETER(instance);

	InitializeObjectAttributes(&SensorThdObjAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	// create sensor thread
	Status = PsCreateSystemThread(&SensorThreadHandle, GENERIC_ALL, &SensorThdObjAttr, NULL, &SensorClientId, WVUProbeCommsThread, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "PsCreateSystemThread() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	Status = ObReferenceObjectByHandle(SensorThreadHandle, (SYNCHRONIZE | DELETE), NULL, KernelMode, (PVOID*)&pSensorThread, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "ObReferenceObjectByHandle() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "PsCreateSystemThread():  Successfully created Sensor thread %p process %p thread id %p\n",
		SensorThreadHandle, SensorClientId.UniqueProcess, SensorClientId.UniqueThread);

	InitializeObjectAttributes(&PollThdObjAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	// create poll thread
	Status = PsCreateSystemThread(&PollThreadHandle, GENERIC_ALL, &PollThdObjAttr, NULL, &PollClientId, WVUTemporalProbeThread, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "PsCreateSystemThread() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	Status = ObReferenceObjectByHandle(PollThreadHandle, (SYNCHRONIZE | DELETE), NULL, KernelMode, (PVOID*)&pPollThread, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "ObReferenceObjectByHandle() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "PsCreateSystemThread():  Successfully created Poll thread %p process %p thread id %p\n",
		PollThreadHandle, PollClientId.UniqueProcess, PollClientId.UniqueThread);

	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE) . . .\n");
#pragma warning(suppress: 28160) // stupid warning about the wait arg TRUE . . . sheesh
	Signaled = KeSetEvent(WVUMainThreadStartEvt, IO_NO_INCREMENT, TRUE);
	do
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Calling KeWaitForSingleObject(WVUMainInitThread, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		Status = KeWaitForSingleObject(WVUMainThreadStartEvt, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Failed! Status=%08x\n", Status);
#pragma warning(suppress: 26438)
			goto ErrorExit;
		}
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Returned from KeWaitForSingleObject(WVUMainInitThread, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		switch (Status)
		{
		case STATUS_SUCCESS:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Thread Returned SUCCESS - Exiting!\n");
			break;
		case STATUS_ALERTED:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Thread Was Just Alerted - Waiting Again!\n");
			break;
		case STATUS_USER_APC:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Thread Had An APC Delievered - Waiting Again!\n");
			break;
		case STATUS_TIMEOUT:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Thread Has Just Timed Out - Exiting!\n");
			break;
		default:
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Thread Has Just Received Status=0x%08x - Exiting!\n", Status);
			break;
		}
	} while (Status == STATUS_ALERTED || Status == STATUS_USER_APC);  // don't bail if we get alerted or APC'd

	WVUQueueManager::GetInstance().TerminateLoop();
	PVOID thd_ary[] = { pSensorThread, pPollThread };
	KeSetEvent(&Globals.poll_wait_evt, IO_NO_INCREMENT, FALSE);
	Status = KeWaitForMultipleObjects(NUMBER_OF(thd_ary), thd_ary, WaitAll, Executive, KernelMode, FALSE, (PLARGE_INTEGER)0, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForMultipleObjects(NUMBER_OF(thd_ary), thd_ary,,...) Failed waiting for the Sensor and Poll Thread! Status=%08x\n", Status);		
	}	
	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Sensor Thread Exited w/Status=0x%08x!\n", PsGetThreadExitStatus(pSensorThread));
	WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Poll Thread Exited w/Status=0x%08x!\n", PsGetThreadExitStatus(pPollThread));
	(VOID)ObDereferenceObject(pSensorThread);
	(VOID)ObDereferenceObject(pPollThread);
	ZwClose(SensorThreadHandle);
	ZwClose(PollThreadHandle);
	
ErrorExit:
	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	PsTerminateSystemThread(Status);
}
#pragma endregion

#pragma region Probe Communications Thread
/**
* @brief probe communcations thread.  this thread drains the sensor queue by0j
* sending probe data to the user space service.
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUProbeCommsThread(PVOID StartContext)
{
	UNREFERENCED_PARAMETER(StartContext);
	const ULONG REPLYLEN = 128;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER send_timeout = { 0LL };
	send_timeout.QuadPart = RELATIVE(SECONDS(20));
	ULONG SenderBufferLen = 0L;
	ULONG ReplyBufferLen = REPLYLEN;
	PUCHAR ReplyBuffer = NULL;
	PVOID SenderBuffer = NULL;

	FLT_ASSERTMSG("WVUProbeCommsThread must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);

	WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "WVUProbeCommsThread Acquired rundown protection . . .\n");

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
	ReplyBuffer = (PUCHAR)ALLOC_POOL(NonPagedPool, REPLYLEN);
	if (NULL == ReplyBuffer)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "Unable to allocate from NonPagedPool! Status=%08x\n", Status);
		goto ErrorExit;
	}
	
	do
	{
		Status = WVUQueueManager::GetInstance().WaitForQueueAndPortConnect();
		WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "Probe Data Queue Semaphore Read State = %ld\n", WVUQueueManager::GetInstance().Count());
		if (FALSE == NT_SUCCESS(Status) || TRUE == Globals.ShuttingDown)
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "KeWaitForMultipleObjects(Globals.ProbeDataEvents,...) Failed! Status=%08x\n", Status);
			goto ErrorExit;
		}

		// quickly remove a queued entry
		PLIST_ENTRY pListEntry = WVUQueueManager::GetInstance().Dequeue();
		if (NULL == pListEntry)
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, WARNING_LEVEL_ID, "Dequeued an emtpy list entry - continuing!\n");
			continue;
		}

		// all of these machinations below are required because the python side requires that FILTER_MESSAGE_HEADER data
		// be the first type in the structure defintion.  We first get the address of PROBE_DATA_HEADER, add the offset
		// equal to the size filter message header and then dereference the pointer from there.  Sheesh.	
		PPROBE_DATA_HEADER pPDH = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
		SenderBuffer = (PVOID)pPDH;
#pragma warning(suppress: 28193)  // message id will be inspected in the user space service
		SenderBufferLen = pPDH->data_sz;

		Status = FltSendMessage(Globals.FilterHandle, &Globals.WVUProbeDataStreamPort,
			SenderBuffer, SenderBufferLen, ReplyBuffer, &ReplyBufferLen, &send_timeout);

		if (FALSE == NT_SUCCESS(Status) || STATUS_TIMEOUT == Status)
		{
			WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, ERROR_LEVEL_ID, "FltSendMessage "
				"(...) Message Send Failed - putting it back into the queue and waiting!! Status=%08x\n", Status);
			if (FALSE == WVUQueueManager::GetInstance().PutBack(&pPDH->ListEntry)) // put the dequeued entry back
			{
				WVUQueueManager::GetInstance().Dispose(SenderBuffer);  // failed to re-enqueue, free and move on
				WVUQueueManager::GetInstance().IncrementDiscardsCount();
				pListEntry = NULL;
			}
		}
		else if (TRUE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT_BUFFER(LOG_SENSOR_THREAD, ReplyBuffer, ReplyBufferLen);
			WVUQueueManager::GetInstance().Dispose(SenderBuffer);
			pListEntry = NULL;
		}
		ReplyBufferLen = REPLYLEN;
		RtlSecureZeroMemory(ReplyBuffer, REPLYLEN);
	} while (FALSE == Globals.ShuttingDown);

ErrorExit:

	if (NULL != ReplyBuffer)
	{
		FREE_POOL(ReplyBuffer);
	}

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_SENSOR_THREAD, TRACE_LEVEL_ID, "Exiting WVUProbeCommsThread Thread w/Status=0x%08x!\n", Status);

	PsTerminateSystemThread(Status);
}
#pragma endregion

#pragma region Temporal Probe Polling Thread Definition

/**
* @brief Temporal Probe Polling Thread This thread polls active probes and
* runs them when their polling interval is expired
* @param StartContext this threads context as passed during creation
*/
_Use_decl_annotations_
VOID
WVUTemporalProbeThread(PVOID StartContext)
{
	UNREFERENCED_PARAMETER(StartContext);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER poll_timeout = { 0LL };
	poll_timeout.QuadPart = RELATIVE(MILLISECONDS(100));
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	TIME_FIELDS time_fields = {};

	FLT_ASSERTMSG("WVUPollThread must run at IRQL == PASSIVE!", KeGetCurrentIrql() == PASSIVE_LEVEL);

	WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "WVUPollThread Acquired rundown protection . . .\n");
	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	do
	{
		Status = KeWaitForSingleObject(&Globals.poll_wait_evt, Executive, KernelMode, FALSE, &poll_timeout);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_POLLTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(poll_wait_evt,...) Failed! Status=%08x\n", Status);
#pragma warning(suppress: 26438)
			goto ErrorExit;
		}
		WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "Returned from KeWaitForSingleObject(poll_wait_evt, KWAIT_REASON::Executive, KernelMode, TRUE, (PLARGE_INTEGER)0) . . .\n");
		switch (Status)
		{
		case STATUS_SUCCESS:
			WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(poll_wait_evt,...) Thread Returned SUCCESS - Continuing!\n");
			break;
		case STATUS_TIMEOUT:
			WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(poll_wait_evt,...) Thread Has Just Timed Out - Continuing!\n");
			break;
		default:
			WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "KeWaitForSingleObject(poll_wait_evt,...) Thread Has Just Received Status=0x%08x - Continuing!\n", Status);
			break;
		}

		/** let's not modify the list while we are iterating */
		KeAcquireInStackQueuedSpinLock(&WVUQueueManager::GetInstance().GetProbeListSpinLock(), &LockHandle);
		__try
		{
			LIST_FOR_EACH(pProbeInfo, WVUQueueManager::GetInstance().GetProbeList(), WVUQueueManager::ProbeInfo)
			{
				AbstractVirtueProbe* avp = pProbeInfo->Probe;
				if (NULL == avp)
				{
					continue;
				}

				WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "Polling Probe %w for work to be done!\n", &avp->GetProbeName());
				// poll the probe for any required actions on our part
				if (FALSE == avp->OnPoll())
				{
					continue;
				}
				Status = avp->OnRun();
				if (FALSE == NT_SUCCESS(Status))
				{
					WVU_DEBUG_PRINT(LOG_POLLTHREAD, WARNING_LEVEL_ID, "Probe %wZ Failed running avp->OnRun() - Status=0x%08x - Continuing!\n", &avp->GetProbeName(), Status);
				}

				LARGE_INTEGER& probe_last_runtime = (LARGE_INTEGER&)avp->GetLastProbeRunTime();
				LARGE_INTEGER local_time;
				ExSystemTimeToLocalTime(&probe_last_runtime, &local_time);
				RtlTimeToTimeFields(&local_time, &time_fields);
				WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "Probe %w successfully finished its work at %d/%d/%d %d:%d:%d.%d!\n",
					avp->GetProbeName(), time_fields.Month, time_fields.Day, time_fields.Year, time_fields.Hour, time_fields.Minute, time_fields.Second, time_fields.Milliseconds);
			}
		}
		__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }

	} while (FALSE == Globals.ShuttingDown);

ErrorExit:
	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
	WVU_DEBUG_PRINT(LOG_POLLTHREAD, TRACE_LEVEL_ID, "Exiting WVUPollThread Thread w/Status=0x%08x!\n", Status);
	PsTerminateSystemThread(Status);
}
#pragma endregion