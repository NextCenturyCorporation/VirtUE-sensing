/**
* @file AbstractVirtueProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Abstract Base Class for Windows VirtUE Probes Definition
*/
#include "AbstractVirtueProbe.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_ABSTRACTPROBE_POOL_TAG

#pragma warning(suppress: 26439)

volatile LONG AbstractVirtueProbe::ProbeCount = 0L;

/**
* @brief base class constructing an instance of a probe 
*/
AbstractVirtueProbe::AbstractVirtueProbe(const ANSI_STRING& ProbeName) :
	Attributes(ProbeAttributes::NoAttributes), Enabled(FALSE), 
	Registered(FALSE), LastProbeRunTime({ 0LL }), OperationCount(0L)
{
	RunInterval.QuadPart = RELATIVE(SECONDS(30));
	this->ProbeName = ProbeName;	
	WVUQueueManager::GetInstance().Register(*this);
	this->Registered = TRUE;

	if (!NT_SUCCESS(ExUuidCreate(&this->ProbeId)))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
				"ExUuidCreate() failed\n");
		NT_ASSERT(!"Failed to create UUID");
	}

	InterlockedIncrement(&AbstractVirtueProbe::ProbeCount);
}

/**
* @brief base class destructor
*/
AbstractVirtueProbe::~AbstractVirtueProbe()
{
	WVUQueueManager::GetInstance().Unregister(*this);
	this->Registered = FALSE;
	InterlockedDecrement(&AbstractVirtueProbe::ProbeCount);
}

/**
* @brief construct an instance of this object utilizing non paged pool memory
* @return pListEntry an item that was on the probe data queue for further processing
*/
_Use_decl_annotations_
PVOID
AbstractVirtueProbe::operator new(size_t size)
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
AbstractVirtueProbe::operator delete(PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}

/**
* @brief called by system polling thread to check if elapsed time has expired
* @return TRUE if time has expired else FALSE
*/
_Use_decl_annotations_
BOOLEAN
AbstractVirtueProbe::OnPoll()
{
	BOOLEAN success = FALSE;
	LARGE_INTEGER CurrentGMT = { 0LL };

	/** leave if we are not enabled and were not temporal */
	if (FALSE == this->Enabled
		|| (Attributes & ProbeAttributes::Temporal) != ProbeAttributes::Temporal)
	{
		success = FALSE;
		goto Exit;
	}
	KeQuerySystemTimePrecise(&CurrentGMT);
	if (CurrentGMT.QuadPart - LastProbeRunTime.QuadPart >= ABS(RunInterval.QuadPart))
	{
		success = TRUE;
	}
	else
	{
		success = FALSE;
	}
Exit:
	return success;
}


/**
* @brief Start the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN AbstractVirtueProbe::Start()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (TRUE == this->Enabled)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already enabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	if ((Attributes & ProbeAttributes::EnabledAtStart) != ProbeAttributes::EnabledAtStart)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w not enabled at start - probe is registered but not active\n",
			&this->ProbeName);
		goto ErrorExit;
	}

	// we can proceed with the start iff we are not already enabled and we 
	// are to be enabled at start
	Status = STATUS_SUCCESS;

ErrorExit:

	return NT_SUCCESS(Status);
}

/**
* @brief Stop the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN AbstractVirtueProbe::Stop()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (FALSE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already disabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	Status = STATUS_SUCCESS;

ErrorExit:

	return NT_SUCCESS(Status);
}
/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS 
AbstractVirtueProbe::OnRun() 
{
	CONST NTSTATUS Status = STATUS_SUCCESS;
	KeQuerySystemTimePrecise(&this->LastProbeRunTime);  // always call superclasses probe function
	InterlockedIncrement(&this->OperationCount);
	return Status;
}

/**
* @brief called to configure the probe 
* @note Do not create threads that will actively configure this probe, or
* defer execution during during which the probe is configured else
* unpredictable and bizzare results could occur.
* @param config_data newline terminated with assign operator name value 
* pair configuration information
*/
_Use_decl_annotations_
BOOLEAN AbstractVirtueProbe::Configure(const ANSI_STRING & config_data)
{
	UNREFERENCED_PARAMETER(config_data);
	return FALSE;
}
