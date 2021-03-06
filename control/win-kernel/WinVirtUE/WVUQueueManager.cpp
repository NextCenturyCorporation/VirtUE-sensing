/**
* @file WVUQueueManager.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProbeData Queue
*/
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_PROBEDATAQUEUE_POOL_TAG

CONST INT WVUQueueManager::PROBEDATAQUEUESZ = 0x40000; // 256K max entries

/**
* @brief construct an instance of the WVUQueueManager.  The WVUQueueManager constructs a 
* simple producer/consumer lockable queue that moves data to a connected user space program.
*/
#pragma warning(suppress: 26439)
#pragma warning(suppress: 26495)
WVUQueueManager::WVUQueueManager() : MessageId(1), Enabled(FALSE), SizeOfDataInQueue(0LL), 
NumberOfQueueEntries(0LL), NumberOfRegisteredProbes(0L)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	wfso_timeout.QuadPart = 0LL;
	timeout.QuadPart = RELATIVE(SECONDS(220752000));  // yup, wait a year . . .

	// initialize the queue entry count semaphore such that processing halts when there are no
	// more entries in the queue
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
	PRKSEMAPHORE pSemaphore = (PKSEMAPHORE)ALLOC_POOL(NonPagedPool, sizeof KSEMAPHORE);
	if (NULL == pSemaphore)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
		goto ErrorExit;
	}
	KeInitializeSemaphore(pSemaphore, 0, PROBEDATAQUEUESZ);
	this->PDQEvents[ProbeDataSemEmptyQueue] = pSemaphore;

	// init the Port Connection Event  This coordinate the connections from user space
	// so that the outbound queue processor will start or stop processing depending on
	// current connection state	
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
	PRKEVENT pKEvent = (PRKEVENT)ALLOC_POOL(NonPagedPool, sizeof KEVENT);
	if (NULL == pKEvent)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
		goto ErrorExit;
	}
	KeInitializeEvent(pKEvent, EVENT_TYPE::NotificationEvent, FALSE);
	this->PDQEvents[ProbeDataEvtConnect] = pKEvent;

	// initialize the spinlock that controls access to the probe data queue
	KeInitializeSpinLock(&this->PDQueueSpinLock);

	// initialize the Probe Data Queue TWLL
	InitializeListHead(&this->PDQueue);

	// initialize the spinlock that controls access to the probe list
	KeInitializeSpinLock(&this->ProbeListSpinLock);

	// initialize the probe list
	InitializeListHead(&this->ProbeList);

	// we build the queue successfully
	this->Enabled = TRUE;
	goto SuccessExit;
ErrorExit:
	this->Enabled = FALSE;
SuccessExit:
	return;
}

/**
* @brief filter exceptions for the destructor
*/
_Use_decl_annotations_
int 
WVUQueueManager::dtor_exc_filter(
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

/**
* @brief destroy this instance of the WVUQueueManager
*/
WVUQueueManager::~WVUQueueManager()
{	
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);
	__try
	{
		while (FALSE == IsListEmpty(&this->ProbeList))
		{
			PLIST_ENTRY pListEntry = RemoveHeadList(&this->ProbeList);
			ProbeInfo* pProbeInfo = CONTAINING_RECORD(pListEntry, ProbeInfo, ListEntry);
			delete pProbeInfo;
		}

		while (FALSE == IsListEmpty(&this->PDQueue))
		{
			PLIST_ENTRY pListEntry = RemoveHeadList(&this->PDQueue);
			PPROBE_DATA_HEADER pProbeInfo = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
			delete[](PBYTE)pProbeInfo;
		}
	}
	__finally
	{
		KeReleaseInStackQueuedSpinLock(&LockHandle);
	}
	
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

	this->Enabled = FALSE;
}

/**
* @brief cause the data queue to exit
* @note if the queue is full, then exception will be emitted. We will catch that exception
* and move on.
*/
VOID
WVUQueueManager::TerminateLoop()
{
    WVU_DEBUG_PRINT(LOG_QUEUE_MGR, INFO_LEVEL_ID, "Shutting down.\n");
	Globals.ShuttingDown = TRUE;  // make sure we exit the loop/thread in the queue processor
	// the next two instructions will cause the consumer loop to terminate
	this->SemaphoreRelease();  // artificially release the semaphore to move things along
	this->OnConnect();  // artificially simulate a connect to get things moving
}

/**
* @brief cause the sempahore count to be incremented. Ignore exceptions
*/
VOID
WVUQueueManager::SemaphoreRelease()
{
	__try
	{
		// cause the semaphore to move through the processing loop
		KeReleaseSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue], IO_NO_INCREMENT, 1, FALSE);  // Signaled when the Queue is not empty		
		this->NumberOfQueueEntries = this->Count();
	}
	__except (dtor_exc_filter(GetExceptionCode(), GetExceptionInformation()))
	{
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Error augmenting semaphore value - ignored!\n");
	}
}

/**
* @brief adjust non-critical counter data
*/
_Use_decl_annotations_
VOID
WVUQueueManager::update_counters(
	PLIST_ENTRY pListEntry)
{
	PPROBE_DATA_HEADER pPDH = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
	InterlockedAdd64(&this->SizeOfDataInQueue, pPDH->data_sz);
	WVU_DEBUG_PRINT(LOG_QUEUE_MGR, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
		this->SizeOfDataInQueue, this->Count());
}

/**
* @brief cause the queue to be trimmed a level that is at or below the max queue size
*/
VOID
WVUQueueManager::TrimProbeDataQueue()
{
	while (this->Count() >= WVUQueueManager::PROBEDATAQUEUESZ)  // remove the oldest entry if we're too big
	{
		const PLIST_ENTRY pDequedEntry = RemoveHeadList(&this->PDQueue);  // cause the WaitForSingleObject to drop the semaphore count
		this->NumberOfQueueEntries = this->Count();
		PPROBE_DATA_HEADER pPDH = CONTAINING_RECORD(pDequedEntry, PROBE_DATA_HEADER, ListEntry);
		InterlockedAdd64(&this->SizeOfDataInQueue, (-(pPDH->data_sz)));
		delete[] pPDH;
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Trimmed WVUQueueManager\n");
		this->AcquireQueueSempahore();  // cause the semaphore count to be decremented by one
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
			this->SizeOfDataInQueue, this->Count());
	}
}

/**
* @brief enqueue a probe data item to be transmitted to the listening userspace program
* @param pListEntry The item to be enqueued
* @return TRUE if item was successfully enqueued else FALSE if queue is disabled
*/
_Use_decl_annotations_
BOOLEAN
WVUQueueManager::Enqueue(
	PLIST_ENTRY pListEntry)
{
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	BOOLEAN success = FALSE;

	if (FALSE == Enabled)
	{
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Attempting to use an invalid queue!\n");
		goto Error;
	}

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);
	__try
	{
		TrimProbeDataQueue();		
		InsertTailList(&this->PDQueue, pListEntry);
		update_counters(pListEntry);
		SemaphoreRelease();
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
			this->SizeOfDataInQueue, this->Count());
	}
	__finally
	{
		KeReleaseInStackQueuedSpinLock(&LockHandle);
	}
	success = TRUE;
Error:
	return success;
}

/**
* @brief return a probe data item that cannot be transmitted to the listening userspace program
* @note If the queue is full at the time the putback is attempted, this data item will be discarded
* because it will be the oldest data in the queue.
* @param pListEntry The item to be enqueued
* @return TRUE if item was successfully enqueued else FALSE if queue is disabled
*/
_Use_decl_annotations_
BOOLEAN 
WVUQueueManager::PutBack(
	PLIST_ENTRY pListEntry)
{
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	BOOLEAN success = FALSE;

	if (FALSE == Enabled)
	{
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Attempting to use an invalid queue!\n");
		goto Error;
	}

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);
	__try
	{
		TrimProbeDataQueue();
		InsertHeadList(&this->PDQueue, pListEntry);
		update_counters(pListEntry);
		SemaphoreRelease();
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
			this->SizeOfDataInQueue, this->Count());
	}
	__finally
	{
		KeReleaseInStackQueuedSpinLock(&LockHandle);
	}
	success = TRUE;
Error:
	return success;
}

/**
* @brief dequeue and return a pointer an item to be processed
* @return NULL if memory allocation not successfull else pointer to 
* the new object instance
*/
_Use_decl_annotations_
PLIST_ENTRY 
WVUQueueManager::Dequeue()
{
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	PLIST_ENTRY pListEntry = NULL;

	KeAcquireInStackQueuedSpinLock(&this->PDQueueSpinLock, &LockHandle);
	__try
	{
		if (FALSE == Enabled || TRUE == IsListEmpty(&this->PDQueue))
		{
			WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Attempting to use an invalid or empty queue!\n");
			pListEntry = NULL;
			__leave;
		}
		pListEntry = RemoveHeadList(&this->PDQueue);
		if (NULL == pListEntry)
		{
			WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Unable to remove a queue entry!\n");
			pListEntry = NULL;
			__leave;
		}
		PPROBE_DATA_HEADER pPDH = CONTAINING_RECORD(pListEntry, PROBE_DATA_HEADER, ListEntry);
		InterlockedAdd64(&this->SizeOfDataInQueue, (-(pPDH->data_sz)));
		this->NumberOfQueueEntries = this->Count();
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, TRACE_LEVEL_ID, "**** Queue Status: Data Size %lld, Entry Count: %ld\n",
			this->SizeOfDataInQueue, this->Count());
	}
	__finally
	{
		KeReleaseInStackQueuedSpinLock(&LockHandle);
	}
	return pListEntry;
}

/**
* @brief disposes a successfully transmitted probe data item
* @note bumps the message id count
*/
_Use_decl_annotations_
VOID 
WVUQueueManager::Dispose(PVOID pBuf)
{
	this->MessageId++;
	delete[](PUCHAR)pBuf;
	return VOID();
}


/**
* @brief registers a Virtue Probe
* @note returns FALSE if probe already registered
* @param probe The probe to register
* @return TRUE if probe succesfully registered else FALSE
*/
_Use_decl_annotations_
BOOLEAN 
WVUQueueManager::Register(AbstractVirtueProbe& probe)
{
	BOOLEAN success = FALSE;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	KeAcquireInStackQueuedSpinLock(&this->ProbeListSpinLock, &LockHandle);
	__try
	{
		if (NULL == FindProbeByName(probe.GetProbeName()))
		{
			ProbeInfo* pProbeInfo = new ProbeInfo;
			pProbeInfo->Probe = &probe;
			InsertTailList(&this->ProbeList, &pProbeInfo->ListEntry);
			InterlockedIncrement(&this->NumberOfRegisteredProbes);
			success = TRUE;
		}
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }
	return success;
}

/**
* @brief unregisters a Virtue Probe
* @param probe The probe to unregister
* @return TRUE if the registration list is empty else FALSE
*/
_Use_decl_annotations_
BOOLEAN 
WVUQueueManager::Unregister(AbstractVirtueProbe& probe)
{
	BOOLEAN is_empty = TRUE;
	KLOCK_QUEUE_HANDLE LockHandle = { { NULL,NULL },0 };
	KeAcquireInStackQueuedSpinLock(&this->ProbeListSpinLock, &LockHandle);
	__try
	{
		ProbeInfo* pProbeInfo = FindProbeByName(probe.GetProbeName());
		if (NULL != pProbeInfo)
		{
			is_empty = RemoveEntryList(&pProbeInfo->ListEntry);
			delete pProbeInfo;
			InterlockedDecrement(&this->NumberOfRegisteredProbes);
		}
	}
	__finally { KeReleaseInStackQueuedSpinLock(&LockHandle); }
	return is_empty;
}

/**
* @brief Finds a probe by name ignoring case
* @note the case lowering shenanigans are because we operate under IRQL restrictions
* @param probe The probe to unregister
* @return TRUE if the registration list is empty else FALSE
*/
_Use_decl_annotations_
WVUQueueManager::ProbeInfo* 
WVUQueueManager::FindProbeById(const UUID & probeid_to_be_found)
{
	ProbeInfo* pProbeInfo = nullptr;
	if (TRUE == IsEqualGUID(probeid_to_be_found, ZEROGUID))
	{
		WVU_DEBUG_PRINT(LOG_QUEUE_MGR, WARNING_LEVEL_ID, "Invalid Zero GUID Value!\n");
		goto ErrorExit;
	}

	LIST_FOR_EACH(probe, this->ProbeList, ProbeInfo)
	{		
		if (TRUE == IsEqualGUID(probeid_to_be_found, probe->Probe->GetProbeId()))
		{
			pProbeInfo = probe;
			break;
		}
	}

ErrorExit:

	return pProbeInfo;
}

/**
* @brief Finds a probe by name ignoring case
* @note the case lowering shenanigans are because we operate under IRQL restrictions
* @param probe The probe to unregister
* @param IgnoreCase If TRUE then case ignored else not ignored
* @return ProbeInfo instance else nullptr if not found
*/
_Use_decl_annotations_
WVUQueueManager::ProbeInfo *
WVUQueueManager::FindProbeByName(const ANSI_STRING& probe_to_be_found, BOOLEAN IgnoreCase)
{
	ProbeInfo* pProbeInfo = nullptr;

	LIST_FOR_EACH(probe, this->ProbeList, ProbeInfo)
	{
		if (NULL == probe
			|| NULL == probe->Probe
			|| NULL == probe->Probe->GetProbeName().Buffer)
		{
			FLT_ASSERTMSG("Something is dramatically wrong with the probe data!", FALSE);
			continue;  // nope, we don't have a name?
		}

		if (0 == CompareAnsiString(probe->Probe->GetProbeName(), probe_to_be_found, IgnoreCase))
		{
			pProbeInfo = probe;
			break;
		}
	}

	return pProbeInfo;
}

/**
* @brief construct an instance of this object utilizing non paged pool memory
* @return pListEntry an item that was on the probe data queue for further processing
*/
_Use_decl_annotations_
PVOID 
WVUQueueManager::operator new(size_t size)
{
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
	PVOID pVoid = ExAllocatePoolWithTag(NonPagedPool, size, COMMON_POOL_TAG);
	return pVoid;
}

/**
* @brief destroys an instance of this object and releases its memory
* @param ptr pointer to the object instance to be destroyed
*/
_Use_decl_annotations_
VOID CDECL 
WVUQueueManager::operator delete(PVOID ptr)
{	
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}
