/**
* @file FltCommandQueue.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define WinVirtues Command, Response and Probe Lists and their operations
*/
#include "FltCommandQueue.h"
#define COMMON_POOL_TAG WVU_FLTCOMMSQUEUE_POOL_TAG

CONST INT FltCommandQueue::COMMANDQUEUESZ = 0x01;	// This queue size should be tunable?
CONST INT FltCommandQueue::RESPONSEQUEUESZ = 0x100;	// This queue size should be tunable?

/**
* @brief construct an instance of the command queueu
*/
FltCommandQueue::FltCommandQueue() :
	CommandId(1), Enabled(FALSE), SizeOfDataInQueue(0LL), NumberOfQueueEntries(0LL)
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
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
		PRKSEMAPHORE pSemaphore = (PKSEMAPHORE)ALLOC_POOL(NonPagedPool, sizeof KSEMAPHORE);
		if (NULL == pSemaphore)
		{
			Status = STATUS_MEMORY_NOT_ALLOCATED;
			WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
			goto ErrorExit;
		}
		KeInitializeSemaphore(pSemaphore, 0, COMMANDQUEUESZ);
		this->CmdQEvents[SemEmptyQueue] = pSemaphore;

		// initialize the queue entry count semaphore such that processing halts when there are no
		// more entries in the queue
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
		pSemaphore = (PKSEMAPHORE)ALLOC_POOL(NonPagedPool, sizeof KSEMAPHORE);
		if (NULL == pSemaphore)
		{
			Status = STATUS_MEMORY_NOT_ALLOCATED;
			WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
			goto ErrorExit;
		}
		KeInitializeSemaphore(pSemaphore, 0, RESPONSEQUEUESZ);
		this->RespQEvents[SemEmptyQueue] = pSemaphore;

		// init the Port Connection Event  This coordinate the connections from user space
		// so that the outbound queue processor will start or stop processing depending on
		// current connection state	
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
		PRKEVENT pKEvent = (PRKEVENT)ALLOC_POOL(NonPagedPool, sizeof KEVENT);
		if (NULL == pKEvent)
		{
			Status = STATUS_MEMORY_NOT_ALLOCATED;
			WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "Failed to allocate nonpaged pool memory for the semaphore FAIL=%08x\n", Status);
			goto ErrorExit;
		}
		KeInitializeEvent(pKEvent, EVENT_TYPE::NotificationEvent, FALSE);
		this->CmdQEvents[EvtConnect] = pKEvent;
		this->RespQEvents[EvtConnect] = pKEvent;

		// initialize the spinlock that controls access to the command queue
		KeInitializeSpinLock(&this->CmdQueueSpinLock);

		// initialize the command Queue TWLL
		InitializeListHead(&this->CmdQueue);

		// initialize the spinlock that controls access to the Response queue
		KeInitializeSpinLock(&this->RspQueueSpinLock);

		// initialize the Response Queue TWLL
		InitializeListHead(&this->RespQueue);

		// we build the queue successfully
		this->Enabled = TRUE;
		goto SuccessExit;
	ErrorExit:
		this->Enabled = FALSE;
	SuccessExit:
		return;
}

/**
* @brief destroy this instance of the command queueu
*/
FltCommandQueue::~FltCommandQueue()
{
	PLIST_ENTRY pListEntry = NULL;

	if (NULL != this->CmdQueue.Flink)
	{
		pListEntry = this->CmdQueue.Flink;
		while (NULL != pListEntry && pListEntry != &this->CmdQueue)
		{
			PCOMMAND_MESSAGE pCmdMsg = CONTAINING_RECORD(pListEntry, COMMAND_MESSAGE, ListEntry);
			pListEntry = pListEntry->Flink;
			delete[](PUCHAR)pCmdMsg;
		}
	}

	if (NULL != this->RespQueue.Flink)
	{
		pListEntry = this->RespQueue.Flink;
		while (NULL != pListEntry && pListEntry != &this->RespQueue)
		{
			PRESPONSE_MESSAGE pRespMsg = CONTAINING_RECORD(pListEntry, RESPONSE_MESSAGE, ListEntry);
			pListEntry = pListEntry->Flink;
			delete[](PUCHAR)pRespMsg;
		}
	}

	if (NULL != this->CmdQEvents[EvtConnect])
	{
		FREE_POOL(this->CmdQEvents[EvtConnect]);
		this->CmdQEvents[EvtConnect] = NULL;
	}

	if (NULL != this->CmdQEvents[SemEmptyQueue])
	{
		FREE_POOL(this->CmdQEvents[SemEmptyQueue]);
		this->CmdQEvents[SemEmptyQueue] = NULL;
	}

	this->Enabled = FALSE;
}

/**
* @brief subscribe a probe to this command queue
*/
_Use_decl_annotations_
BOOLEAN 
FltCommandQueue::subscribe(const AbstractVirtueProbe& probe)
{
	UNREFERENCED_PARAMETER(probe);
	return BOOLEAN();
}

/**
* @brief unsubscribe a probe from this command queue
*/
_Use_decl_annotations_
BOOLEAN 
FltCommandQueue::unsubscribe(const AbstractVirtueProbe& probe)
{
	UNREFERENCED_PARAMETER(probe);
	return BOOLEAN();
}
