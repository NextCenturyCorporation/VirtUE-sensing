/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Object creation/destruction callback handlers
*/
#include "ObjectCreateNotifyCB.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_OBJECT_CREATE_POOL_TAG

/**
* @brief - Process Notification Callback.
* @note The operating system calls the driver's process-notify routine 
* at PASSIVE_LEVEL inside a critical region with normal kernel APCs disabled.
* When a process is created, the process-notify routine runs in the context 
* of the thread that created the new process. When a process is deleted, the 
* process-notify routine runs in the context of the last thread to exit from 
* the process.
* @param Process  - a pointer to the new processes EPROCESS structure
* @param ProcessId  - the new processes process id
* @param CreateInfo  - provides information about a newly created process
*/
_Use_decl_annotations_
VOID
ProcessNotifyCallbackEx(
	PEPROCESS  Process,
	HANDLE  ProcessId,
	const PPS_CREATE_NOTIFY_INFO  CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);

    // Take a rundown reference 
    (VOID)ExAcquireRundownProtection(&Globals.RunDownRef);
    
    if (CreateInfo) 
    {    
        WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
            "***** Process Created: Image File Name=%wZ, EPROCESS=%p, ProcessId=%p\n",
            CreateInfo->ImageFileName, Process, ProcessId);        
    }
    else
    {
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, TRACE_LEVEL_ID,
            "***** Process Destroyed: EPROCESS %p, ProcessId %p, \n",
            Process, ProcessId);
    }

    // Drop a rundown reference 
    ExReleaseRundownProtection(&Globals.RunDownRef);

    return VOID();
}


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
