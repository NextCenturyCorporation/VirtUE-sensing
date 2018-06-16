/**
* @file FltCommandQueue.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Declares WinVirtues Command, Response and Probe Lists and their operations
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"

class FltCommandQueue
{	
public:
	/**
	* Utilized when managing the FltCommandQueue KEVENT array
	*/
	enum CmdQueueEvtEnum
	{
		SemEmptyQueue,	// The SEMAPHORE that signals when the Command Queue is Empty
		EvtConnect,		// Th KEVENT that signals when the service connects
		MAXENTRIES		// The number of entries
	};

private:
	/** The Command Queue Size */
	static CONST INT COMMANDQUEUESZ;
	/** The Response Queue Size */
	static CONST INT RESPONSEQUEUESZ;
	LARGE_INTEGER wfso_timeout;
	LARGE_INTEGER timeout;
	/** Command Queue serialization for waitformultipleevents */
	PVOID CmdQEvents[CmdQueueEvtEnum::MAXENTRIES];
	/** Command Queue serialization for waitformultipleevents */
	PVOID RespQEvents[CmdQueueEvtEnum::MAXENTRIES];
	/** Command Queue Class Instance is enabled or not */
	BOOLEAN Enabled;
	/** the current command id */
	ULONGLONG CommandId;
	/** for debugging show the size of data in the queue */
	volatile LONGLONG SizeOfDataInQueue;
	/** for debugging ease, show the number of current queue entries */
	volatile LONGLONG NumberOfQueueEntries;
	/** Command Queue Spin Lock */
	KSPIN_LOCK CmdQueueSpinLock;
	/** Command Queue  */
	LIST_ENTRY CmdQueue;
	/** Response Queue Spin Lock */
	KSPIN_LOCK RspQueueSpinLock;
	/** Response Queue  */
	LIST_ENTRY RespQueue;
	/** ProbeList Spin Lock */
	KSPIN_LOCK ProbeListSpinLock;
	/** ProbeList - list of active probes available to do work */
	LIST_ENTRY ProbeList;

public:
	FltCommandQueue();
	~FltCommandQueue();	
	// Wait for the queue to have at least one entry and the port to be connected
	_Success_(NT_SUCCESS(return) == TRUE)
		NTSTATUS AcquireCommandQueueSempahore() {
		return KeWaitForSingleObject((PRKSEMAPHORE)this->CmdQEvents[SemEmptyQueue],
			Executive, KernelMode, FALSE, &wfso_timeout);
	}
	// Wait for the queue to have at least one entry and the port to be connected
	_Success_(NT_SUCCESS(return) == TRUE)
		NTSTATUS AcquireResponseQueueSempahore() {
		return KeWaitForSingleObject((PRKSEMAPHORE)this->RespQEvents[SemEmptyQueue],
			Executive, KernelMode, FALSE, &wfso_timeout);
	}
};

