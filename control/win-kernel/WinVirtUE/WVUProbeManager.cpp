/**
* @file WinVirtUe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Windows Virtue Runtime Manager
*/

#include "WVUProbeManager.h"

#define COMMON_POOL_TAG WVU_WVUMGR_POOL_TAG

// managers
class WVUQueueManager *pPDQ = nullptr;

// Probes
class ImageLoadProbe *pILP = nullptr;
class ProcessCreateProbe *pPCP = nullptr;
class ProcessListValidationProbe *pPLVP = nullptr;
class RegistryModificationProbe *pRMP = nullptr;
class ThreadCreateProbe *pTCP = nullptr;

/**
* @brief Construct an instance of the Windows Virtue Manager
* @note All Static Probes *MUST* be constructed here
*/
WVUProbeManager::WVUProbeManager() : Status(STATUS_SUCCESS)
{ 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	WVUCommsManager& commsmgr = WVUCommsManager::GetInstance();
	// Start the filter comms manager
	NT_ASSERTMSG("Failed to enable the Filter Communications Manager!", TRUE == commsmgr.Start());

	// Make ready the image load probe
	pILP = new ImageLoadProbe();
	if (NULL == pILP)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_PROBE_MGR, ERROR_LEVEL_ID,
			"ImageLoadProbe not constructed - Status=%08x\n", Status);
		goto ErrorExit;
	}

	if (TRUE == pILP->IsEnabledAtStart())
	{
		// Start the image load probe
		NT_ASSERTMSG("Failed to start the image load probe!", TRUE == pILP->Start());
	}

	// Make ready the process create probe
	pPCP = new ProcessCreateProbe();
	if (NULL == pPCP)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_PROBE_MGR, ERROR_LEVEL_ID,
			"ProcessCreateProbe not constructed - Status=%08x\n", Status);
		goto ErrorExit;
	}

	if (TRUE == pPCP->IsEnabledAtStart())
	{
		// Start the process create probe
		NT_ASSERTMSG("Failed to start the process create probe!", TRUE == pPCP->Start());
	}

	// Make ready the process list validation probe
	pPLVP = new ProcessListValidationProbe();
	if (NULL == pPLVP)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_PROBE_MGR, ERROR_LEVEL_ID,
			"ProcessListValidationProbe not constructed - Status=%08x\n", Status);
		goto ErrorExit;
	}

	if (TRUE == pPLVP->IsEnabledAtStart())
	{
		// Start the process create probe
		NT_ASSERTMSG("Failed to start the process list validation probe!", TRUE == pPLVP->Start());
	}

	// Make ready the registry modification probe
	pRMP = new RegistryModificationProbe();
	if (NULL == pRMP)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_PROBE_MGR, ERROR_LEVEL_ID,
			"RegistryModificationProbe not constructed - Status=%08x\n", Status);
		goto ErrorExit;
	}

	if (TRUE == pRMP->IsEnabledAtStart())
	{
		// Start the process create probe
		NT_ASSERTMSG("Failed to start the Registry Modification Probe!", TRUE == pRMP->Start());
	}

	// Make ready the thread creation probe
	pTCP = new ThreadCreateProbe();
	if (NULL == pTCP)
	{
		Status = STATUS_MEMORY_NOT_ALLOCATED;
		WVU_DEBUG_PRINT(LOG_PROBE_MGR, ERROR_LEVEL_ID,
			"ThreadCreateProbe not constructed - Status=%08x\n", Status);
		goto ErrorExit;
	}
	if (TRUE == pTCP->IsEnabledAtStart())
	{
		// Start the process create probe
		NT_ASSERTMSG("Failed to start the Thread Create Probe!", TRUE == pTCP->Start());
	}
	
ErrorExit:
	return;
}

/**
* @brief desroy the instance of the Windows Virtue Manager
* @note All Static Probes *MUST* be destroyed here
*/
WVUProbeManager::~WVUProbeManager()
{
	if (NULL != pPLVP)
	{
		NT_ASSERTMSG("Failed to stop the process list validation probe!", TRUE == pPLVP->Stop());
		delete pPLVP;
	}

	if (NULL != pPCP)
	{
		NT_ASSERTMSG("Failed to stop the process create probe!", TRUE == pPCP->Stop());
		delete pPCP;
	}

	if (NULL != pILP)
	{
		NT_ASSERTMSG("Failed to stop the image load probe!", TRUE == pILP->Stop());
		delete pILP;
	}

	if (NULL != pTCP)
	{
		NT_ASSERTMSG("Failed to stop the Thread Create probe!", TRUE == pTCP->Stop());
		delete pTCP;
	}

	if (NULL != pRMP)
	{
		NT_ASSERTMSG("Failed to stop the registry modification probe!", TRUE == pRMP->Stop());
		delete pRMP;
	}

	WVUCommsManager::GetInstance().Stop();

	if (NULL != pPDQ)
	{
		delete pPDQ;
	}

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
}

/**
* @brief construct an instance of this object utilizing non paged pool memory
* @return an instance of this class
*/
_Use_decl_annotations_
PVOID
WVUProbeManager::operator new(size_t size)
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
WVUProbeManager::operator delete(PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}


