/**
* @file AbstractVirtueProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Abstract Base Class for Windows VirtUE Probes Definition
*/
#include "AbstractVirtueProbe.h"
#define COMMON_POOL_TAG WVU_ABSTRACTPROBE_POOL_TAG

#pragma warning(suppress: 26439)

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
* @brief subscribe to the Filter Command Queue
* @param rhs reference to the command queue
* @return reference to the command queue that we subscribed to
*/
_Use_decl_annotations_
const AbstractVirtueProbe&
AbstractVirtueProbe::operator+=(const FltCommandQueue& rhs)
{	
	FLT_ASSERTMSG("Failed to subscribe to the Command Queue!", ((FltCommandQueue&)rhs).subscribe(*this));
	return *this;
}


/**
* @brief unsubscribe from the Filter Command Queue
* @param rhs reference to the command queue
* @return reference to the command queue that we unsubscribed from
*/
_Use_decl_annotations_
const AbstractVirtueProbe&
AbstractVirtueProbe::operator-=(const FltCommandQueue& rhs)
{
	FLT_ASSERTMSG("Failed to unsubscribe from the Command Queue!", ((FltCommandQueue&)rhs).unsubscribe(*this));
	return *this;
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
	KeQuerySystemTimePrecise(&this->LastProbeRunTime);  // always call superclasses probe function
	return STATUS_SUCCESS;
}