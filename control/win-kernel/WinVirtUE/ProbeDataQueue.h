/**
* @file ProbeDataQueue.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief create and manipulate the probe data queue
*/
#pragma once
#include "common.h"
#include <new.h>
#include <cstddef>

class ProbeDataQueue
{
public:

	/**
	* Utilized when managing the ProbeDataEvents KEVENT array
	*/
	enum ProbeDataEvtEnum
	{
		ProbeDataSemEmptyQueue, // The SEMAPHORE that signals when the Probe Data Queue is Empty
		ProbeDataEvtConnect,	// The KEVENT that signals when the service connects
		MAXENTRIES				// The number of entries
	};

private:
	LARGE_INTEGER wfso_timeout;
	LARGE_INTEGER timeout;	
	KSPIN_LOCK PDQueueSpinLock;
	LIST_ENTRY PDQueue;
	ULONGLONG MessageId;
	BOOLEAN Enabled;	
	PVOID PDQEvents[ProbeDataEvtEnum::MAXENTRIES];

	/** The number of elements in the probe data queue */
	static CONST INT PROBEDATAQUEUESZ;
	VOID TrimProbeDataQueue();
	static _Must_inspect_result_ int dtor_exc_filter(_In_ UINT32 code, _In_ _EXCEPTION_POINTERS * ep);
	VOID update_counters(_Inout_ PLIST_ENTRY pListEntry);
	volatile LONGLONG SizeOfDataInQueue;
	/** for debugging ease, show the number of current queue entries */
	volatile LONGLONG NumberOfQueueEntries;
	// Wait for the queue to have at least one entry and the port to be connected
	_Success_(NT_SUCCESS(return) == TRUE)
		NTSTATUS AcquireQueueSempahore() {
		return KeWaitForSingleObject((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue],
			Executive, KernelMode, FALSE, &wfso_timeout);
	}

	_Must_inspect_result_
		BOOLEAN IsConnected() { return 0L != KeReadStateEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect]); }
public:
	ProbeDataQueue();
	~ProbeDataQueue();
	VOID SemaphoreRelease();
	VOID TerminateLoop();
	_Success_(TRUE == return)
	BOOLEAN Enqueue(_Inout_ PLIST_ENTRY pListEntry);
	_Success_(TRUE == return)
	BOOLEAN PutBack(_Inout_ PLIST_ENTRY pListEntry);
	_Must_inspect_result_
	PLIST_ENTRY Dequeue();
	_Must_inspect_result_
	ULONGLONG& GetMessageId() { return this->MessageId;  }
	VOID Dispose(_In_ PVOID pBuf);																							 
	// cause the outbund queue processor to start processing
	VOID OnConnect() { KeSetEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect], IO_NO_INCREMENT, FALSE); }
	// cause the outbound queue processor to stop on disconnect
	VOID OnDisconnect() { (VOID)KeResetEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect]); }
	/** The count function utilizes the semaphore state to show the number of queued entries */
	_Must_inspect_result_
	LONG Count() { return KeReadStateSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue]); }
	// Wait for the queue to have at least one entry and the port to be connected
	_Success_(NT_SUCCESS(return) == TRUE)
	NTSTATUS WaitForQueueAndPortConnect() {
		return KeWaitForMultipleObjects(NUMBER_OF(PDQEvents),
			(PVOID*)&PDQEvents[0], WaitAll, Executive, KernelMode, FALSE, &timeout, NULL); }
	_Must_inspect_impl_
		_Success_(NULL != return)
		PVOID operator new(_In_ size_t size);
	VOID CDECL operator delete(_In_ PVOID ptr);
};
