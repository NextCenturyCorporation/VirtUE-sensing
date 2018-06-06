/**
* @file ProbeDataQueue.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProbeData Queue
*/
#include "ProbeDataQueue.h"
#include "externs.h"
#define COMMON_POOL_TAG WVU_PROBEDATAQUEUE_POOL_TAG

ProbeDataQueue::ProbeDataQueue() : MessageId(1), Enabled(FALSE), SizeOfDataInQueue(0LL), PDQEvents{NULL, NULL}
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	wfso_timeout.QuadPart = 0LL;
	timeout.QuadPart =
#if defined(WVU_DEBUG)
		RELATIVE(SECONDS(300));
#else
		RELATRIVE(SECONDS(1));
#endif

	// initialize the queue entry count semaphore such that processing halts when there are no
	// more entries in the queue
	PRKSEMAPHORE pSemaphore = (PKSEMAPHORE)ALLOC_POOL(NonPagedPool, sizeof KSEMAPHORE);
	if (NULL == pSemaphore)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
		goto ErrorExit;
	}
	KeInitializeSemaphore(pSemaphore, 0, PROBEDATAQUEUESZ);
	this->PDQEvents[ProbeDataSemEmptyQueue] = pSemaphore;

	// init the Port Connection Event  This coordinate the connections from user space
	// so that the outbound queue processor will start or stop processing depending on
	// current connection state	
	PRKEVENT pKEvent = (PRKEVENT)ALLOC_POOL(NonPagedPool, sizeof KEVENT);
	if (NULL == pKEvent)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
		goto ErrorExit;
	}
	KeInitializeEvent(pKEvent, EVENT_TYPE::NotificationEvent, FALSE);
	this->PDQEvents[ProbeDataEvtConnect] = pKEvent;

	// initialize the spinlock that controls access to the probe data queue
	KeInitializeSpinLock(&this->PDQueueSpinLock);

	// initialize the Probe Data Queue TWLL
	InitializeListHead(&this->PDQueue);

	// we build the queue successfully
	this->Enabled = TRUE;
	goto SuccessExit;
ErrorExit:
	this->Enabled = FALSE;
SuccessExit:
	return;
}

_Use_decl_annotations_
int 
ProbeDataQueue::dtor_exc_filter(
	UINT32 code, 
	struct _EXCEPTION_POINTERS *ep)
{
	UNREFERENCED_PARAMETER(ep);
	if (code == STATUS_SEMAPHORE_LIMIT_EXCEEDED)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

ProbeDataQueue::~ProbeDataQueue()
{
	this->Enabled = FALSE;
	Globals.ShuttingDown = TRUE;  // make sure we exit the loop/thread in the queue processor

	// Cause the Multiple Event Water to proceed in the loop and exit
	__try
	{
		// cause the semaphore to move through the processing loop
		KeReleaseSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue], IO_NO_INCREMENT, 1, FALSE);  // Signaled when the Queue is not empty		
	}
	__except (dtor_exc_filter(GetExceptionCode(), GetExceptionInformation()) )
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "Error augmenting semaphore value - ignored!\n")
	}

	KeSetEvent((PRKEVENT)this->PDQEvents[ProbeDataEvtConnect], IO_NO_INCREMENT, FALSE);  // Signaled when Port is Connected

	if (NULL != this->PDQEvents[ProbeDataEvtConnect])
	{
		FREE_POOL(this->PDQEvents[ProbeDataEvtConnect]);
		this->PDQEvents[ProbeDataEvtConnect] = NULL;
	}

	if (NULL != this->PDQEvents[ProbeDataSemEmptyQueue])
	{
		FREE_POOL(this->PDQEvents[ProbeDataSemEmptyQueue]);
		this->PDQEvents[ProbeDataSemEmptyQueue] = NULL;
	}
}

_Use_decl_annotations_
VOID
ProbeDataQueue::SemaphoreRelease()
{
	(VOID)KeReleaseSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue], IO_NO_INCREMENT, 1, FALSE);  // Signaled when the Queue is not empty
	this->NumberOfQueueEntries = this->Count();
}

_Use_decl_annotations_
VOID
ProbeDataQueue::update_counters(
	PLIST_ENTRY pListEntry)
{
	PProbeDataHeader pPDH = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
	InterlockedAdd64(&this->SizeOfDataInQueue, pPDH->DataSz);
	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
		this->SizeOfDataInQueue, this->Count());
}

_Use_decl_annotations_
VOID
ProbeDataQueue::TrimProbeDataQueue()
{
	while (this->Count() >= ::PROBEDATAQUEUESZ)  // remove the oldest entry if we're too big
	{
		const PLIST_ENTRY pDequedEntry = RemoveHeadList(&this->PDQueue);  // cause the WaitForSingleObject to drop the semaphore count
		InterlockedExchange64(&this->NumberOfQueueEntries, KeReadStateSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue]));
		PProbeDataHeader pPDH = CONTAINING_RECORD(pDequedEntry, PROBE_DATA_HEADER, ListEntry);
		InterlockedAdd64(&this->SizeOfDataInQueue, (-(pPDH->DataSz)));
		delete[] pPDH;
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "Trimmed ProbeDataQueue\n");
		this->AcquireQueueSempahore();  // cause the semaphore count to be decremented by one
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
			this->SizeOfDataInQueue, this->Count());
	}
}

_Use_decl_annotations_
BOOLEAN
ProbeDataQueue::Enqueue(
	PLIST_ENTRY pListEntry)
{
	KLOCK_QUEUE_HANDLE LockHandle;
	BOOLEAN success = FALSE;

	if (FALSE == Enabled)
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "Attempting to use an invalid queue!\n");
		goto Error;
	}

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);
	TrimProbeDataQueue();
	InsertTailList(&this->PDQueue, pListEntry);
	update_counters(pListEntry);	
	SemaphoreRelease();
	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "**** Queue Status: Data Size %ld, Entry Count: %ld\n",
		this->SizeOfDataInQueue, this->Count());
	KeReleaseInStackQueuedSpinLock(&LockHandle);
	success = TRUE;
Error:
	return success;
}

_Use_decl_annotations_
BOOLEAN 
ProbeDataQueue::PutBack(
	PLIST_ENTRY pListEntry)
{
	KLOCK_QUEUE_HANDLE LockHandle;
	BOOLEAN success = FALSE;

	if (FALSE == Enabled)
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "Attempting to use an invalid queue!\n");
		goto Error;
	}

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);	  
	TrimProbeDataQueue();
	InsertHeadList(&this->PDQueue, pListEntry);	
	update_counters(pListEntry);
	SemaphoreRelease();
	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
		this->SizeOfDataInQueue, this->Count());
	KeReleaseInStackQueuedSpinLock(&LockHandle);
	success = TRUE;
Error:
	return success;
}

_Use_decl_annotations_
PLIST_ENTRY 
ProbeDataQueue::Dequeue()
{
	KLOCK_QUEUE_HANDLE LockHandle;
	PLIST_ENTRY retval = NULL;

	if (FALSE == Enabled || TRUE == IsListEmpty(&this->PDQueue))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "Attempting to use an invalid or empty queue!\n");
		return NULL;
	}

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);

	PLIST_ENTRY pListEntry = RemoveHeadList(&this->PDQueue);
	if (NULL == pListEntry)
	{
		retval = NULL;
		goto Exit;
	}
	PProbeDataHeader pPDH = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
	InterlockedAdd64(&this->SizeOfDataInQueue, (-(pPDH->DataSz)));
	InterlockedExchange64(&this->NumberOfQueueEntries, KeReadStateSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue]));

Exit:
	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
		this->SizeOfDataInQueue, this->Count());
	KeReleaseInStackQueuedSpinLock(&LockHandle);
	return pListEntry;
}

_Use_decl_annotations_
VOID 
ProbeDataQueue::Dispose(PVOID pBuf)
{
	this->MessageId++;
	delete[](PUCHAR)pBuf;
	return VOID();
}

_Use_decl_annotations_
PVOID 
ProbeDataQueue::operator new(size_t size)
{
	PVOID pVoid = ExAllocatePoolWithTag(NonPagedPool, size, COMMON_POOL_TAG);
	return pVoid;
}

_Use_decl_annotations_
VOID CDECL 
ProbeDataQueue::operator delete(PVOID ptr)
{	
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}