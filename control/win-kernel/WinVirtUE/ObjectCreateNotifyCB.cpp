/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Object creation/destruction callback handlers
*/
#include "ObjectCreateNotifyCB.h"
#define COMMON_POOL_TAG WVU_OBJECT_CREATE_POOL_TAG

/**
* @brief - Process Notification Callback.
* @note currently this is just a 'filler' function - more to follow!
* @param Process  - a pointer to the new processes EPROCESS structure
* @param ProcessId  - the new processes process id
* @param CreateInfo  - provides information about a newly created process
*/
VOID
ProcessNotifyCallbackEx(
	_Inout_ PEPROCESS  Process,
	_In_ HANDLE  ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO  CreateInfo)
{

    // Take a rundown reference 
    (VOID)ExAcquireRundownProtection(&Globals.RunDownRef);
    
    if (CreateInfo) 
    {    
        WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, TRACE_LEVEL_ID,
            "***** Process Created: Image File Name=%wZ, EPROCESS=%p, ProcessId=%p\n",
            CreateInfo->ImageFileName, Process, ProcessId);        
    }
    else
    {
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, TRACE_LEVEL_ID,
            "***** Process Destroyed: EPROCESS %p, ProcessId %p, \n",
            Process, ProcessId);
    }

    // Drop a rundown reference 
    ExReleaseRundownProtection(&Globals.RunDownRef);

    return VOID();
}


/**
* @brief The Image Load Notify Callback Routine
* @param FullImageName This images full image name including directory
* @param ProcessId This Images Process ID that's attaching to.  Zero if its a driver load
* @param ImageInfo Pointer to the IMAGE_INFO data structure that describes this image
*/
VOID
ImageLoadNotificationRoutine(
	_In_ PUNICODE_STRING FullImageName,
	_In_ HANDLE ProcessId,
	_In_ PIMAGE_INFO pImageInfo)
{
	PEPROCESS  pProcess = NULL;
	UNREFERENCED_PARAMETER(pImageInfo);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	NTSTATUS Status = PsLookupProcessByProcessId(ProcessId, &pProcess);
	if (FALSE == NT_SUCCESS(Status))
	{		
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, WARNING_LEVEL_ID, "***** Failed to retreve a PEPROCESS for Process Id %p!\n", ProcessId);
		goto Done;
	}
	
	WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, TRACE_LEVEL_ID, "FullImageName=%wZ,"
		"ProcessId=%p,ImageBase=%p,ImageSize=%p,ImageSectionNumber=%ul\n",
		FullImageName, ProcessId, pImageInfo->ImageBase, (PVOID)pImageInfo->ImageSize,
		pImageInfo->ImageSectionNumber);

Done:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
}

/**
* @brief The Thread Create Notify Callback Routine
* @param ProcessId  the new threads process id
* @param ThreadId this new threads thread id
* @param Create TRUE if thread was created else FALSE it was terminated
*/
VOID ThreadCreateCallback(
	HANDLE ProcessId, 
	HANDLE ThreadId, 
	BOOLEAN Create)
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, TRACE_LEVEL_ID,
		"Thread %p within ProcessId %p was %s\n",
		ThreadId, ProcessId, Create ? "Created" : "Terminated");	
}
