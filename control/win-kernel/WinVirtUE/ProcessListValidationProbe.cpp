/**
* @file ProcessListValidationProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ProcessListValidationProbe Probe Class definition
*/
#include "ProcessListValidationProbe.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_PROCLISTVALIDPROBE_POOL_TAG

/**
* @brief construct an instance of this probe
*/
ProcessListValidationProbe::ProcessListValidationProbe()
{
	ProbeName = RTL_CONSTANT_STRING(L"ProcessListValidation");
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
	if (NULL != pPDQ)
	{
		pPDQ->Register(*this);
	}
}

/**
* @brief destroy this probes instance *
*/
ProcessListValidationProbe::~ProcessListValidationProbe()
{
	if (NULL != pPDQ)
	{
		pPDQ->DeRegister(*this);
	}
}

/**
* @brief Enable the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Enable()
{
	this->Enabled = TRUE;
	return TRUE;
}

/**
* @brief Disable the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::Disable()
{
	this->Enabled = FALSE;
	return TRUE;
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ProcessListValidationProbe::State()
{
	return this->Enabled;
}

/**
* @brief Mitigate known issues that this probe discovers
* @note Mitigation is not being called as of June 2018
* @param argv array of arguments
* @param argc argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS ProcessListValidationProbe::Mitigate(
	PCHAR argv[],
	UINT32 argc)
{
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(argc);
	return NTSTATUS();
}


/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS
ProcessListValidationProbe::OnRun()
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = AbstractVirtueProbe::OnRun();
	return Status;
}

