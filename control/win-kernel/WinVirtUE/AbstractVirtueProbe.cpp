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

/**
* @brief base class constructing an instance of a probe 
*/
AbstractVirtueProbe::AbstractVirtueProbe(const ANSI_STRING& ProbeName) :
	Attributes(ProbeAttributes::NoAttributes), Enabled(FALSE), LastProbeRunTime({ 0LL })
{
	RunInterval.QuadPart = RELATIVE(SECONDS(30));
	this->ProbeName = ProbeName;
	if (NULL != pPDQ)
	{
		pPDQ->Register(*this);
	}
}

/**
* @brief base class destructor
*/
AbstractVirtueProbe::~AbstractVirtueProbe()
{
	if (NULL != pPDQ)
	{
		pPDQ->Unregister(*this);
	}
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
BOOLEAN
AbstractVirtueProbe::OnPoll()
{
	BOOLEAN success = FALSE;
	LARGE_INTEGER CurrentGMT = { 0LL };

	if ((Attributes & ProbeAttributes::Temporal) != ProbeAttributes::Temporal)
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
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS 
AbstractVirtueProbe::OnRun() 
{
	CONST NTSTATUS Status = STATUS_SUCCESS;
	KeQuerySystemTimePrecise(&this->LastProbeRunTime);  // always call superclasses probe function
	return Status;
}

/**
* @brief called to configure the probe 
* @param NameValuePairs newline terminated with assign operator name value 
* pair configuration information
*/
_Use_decl_annotations_
BOOLEAN AbstractVirtueProbe::Configure(const ANSI_STRING & NameValuePairs)
{
	UNREFERENCED_PARAMETER(NameValuePairs);
	return BOOLEAN();
}