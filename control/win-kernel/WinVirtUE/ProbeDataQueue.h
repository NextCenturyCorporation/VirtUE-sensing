/**
* @file ProbeDataQueue.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief create and manipulate the probe data queue
*/
#pragma once
#include "common.h"

class ProbeDataQueue
{
private:
	const int MAXQUEUESIZE = 0x2000;
	KSPIN_LOCK PDQueueSpinLock;
	LIST_ENTRY PDQueue;
	ULONGLONG MessageId;
	BOOLEAN IsQueueBuilt;	
	PVOID PDQEvents[2];
	static _Must_inspect_result_ int dtor_exc_filter(_In_ UINT32 code, _In_ _EXCEPTION_POINTERS * ep);

public:
	ProbeDataQueue();
	~ProbeDataQueue();
	BOOLEAN Enqueue(_Inout_ PLIST_ENTRY pListEntry);
	BOOLEAN PutBack(_Inout_ PLIST_ENTRY pListEntry);
	_Must_inspect_result_
	PLIST_ENTRY Dequeue();
	_Must_inspect_result_
	ULONGLONG& GetMessageId() { return this->MessageId;  }
	VOID Dispose(_In_ PVOID pBuf);
	_Must_inspect_result_
	LONG Count() { return KeReadStateSemaphore((PRKSEMAPHORE)this->PDQEvents[ProbeDataSemEmptyQueue]); }																								 
	// cause the outbund queue processor to start processing
	VOID OnConnect() { KeSetEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect], IO_NO_INCREMENT, FALSE); }
	// cause the outbound queue processor to stop on disconnect
	VOID OnDisconnect() { (VOID)KeResetEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect]); }
	_Must_inspect_result_
	BOOLEAN IsConnected() { return 0L != KeReadStateEvent((PRKEVENT)PDQEvents[ProbeDataEvtConnect]); }
	// Wait for the queue to have at least one entry and the port to be connected
	_Success_(NT_SUCCESS(return) == TRUE)
	NTSTATUS WaitForQueueAndPortConnect() {
		return KeWaitForMultipleObjects(NUMBER_OF(PDQEvents),
			(PVOID*)&PDQEvents[0], WaitAll, Executive, KernelMode, FALSE, (PLARGE_INTEGER)0, NULL); }
	_Must_inspect_impl_
	PVOID operator new(_In_ size_t size);
	VOID CDECL operator delete(_In_ PVOID ptr);
};
