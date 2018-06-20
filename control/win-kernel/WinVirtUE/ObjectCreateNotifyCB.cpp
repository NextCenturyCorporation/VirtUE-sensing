/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Object creation/destruction callback handlers
*/
#include "ObjectCreateNotifyCB.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_OBJECT_CREATE_POOL_TAG


/**
* @brief The Thread Create Notify Callback Routine
* @param ProcessId  the new threads process id
* @param ThreadId this new threads thread id
* @param Create TRUE if thread was created else FALSE it was terminated
*/
VOID ThreadCreateCallback(
	_In_ HANDLE ProcessId, 
	_In_ HANDLE ThreadId,
	_In_ BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ThreadId);
	UNREFERENCED_PARAMETER(Create);

	WVU_DEBUG_PRINT(LOG_NOTIFY_THREAD, TRACE_LEVEL_ID,
		"Thread %p within ProcessId %p was %s\n",
		ThreadId, ProcessId, Create ? "Created" : "Terminated");	
}
