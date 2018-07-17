
/**
* @file ImageLoadProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ImageLoad Probe Class definition
*/
#include "ImageLoadProbe.h"
#include "WVUQueueManager.h"
#define COMMON_POOL_TAG WVU_IMAGELOADPROBE_POOL_TAG

static ANSI_STRING probe_name = RTL_CONSTANT_STRING("ImageLoad");

/**
* @brief construct an instance of this probe
*/
ImageLoadProbe::ImageLoadProbe() : AbstractVirtueProbe(probe_name) 
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
}

/**
* @brief Start the ImageLoadProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ImageLoadProbe::Start()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (TRUE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID, 
			"Probe %w already enabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	if ((Attributes & ProbeAttributes::EnabledAtStart) != ProbeAttributes::EnabledAtStart)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w not enabled at start - probe is registered but not active\n", 
			&this->ProbeName);			
		goto ErrorExit;
	}

	Status = PsSetLoadImageNotifyRoutine(ImageLoadProbe::ImageLoadNotificationRoutine);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "PsSetLoadImageNotifyRoutine(ImageLoadNotificationRoutine) "
			"Add Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "PsSetLoadImageNotifyRoutine():  Successfully Enabled Image Load Sensor\n");
	this->Enabled = TRUE;

ErrorExit:

	return NT_SUCCESS(Status);
}

/**
* @brief Stop the ImageLoadProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN ImageLoadProbe::Stop()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (FALSE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already disabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	Status = PsRemoveLoadImageNotifyRoutine(ImageLoadProbe::ImageLoadNotificationRoutine);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "PsRemoveLoadImageNotifyRoutine(ImageLoadNotificationRoutine) "
			"Remove Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "PsRemoveLoadImageNotifyRoutine(): Successfully Disabled Image Load Sensor\n");
	this->Enabled = FALSE;
ErrorExit:

	return NT_SUCCESS(Status);
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN ImageLoadProbe::IsEnabled()
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
NTSTATUS ImageLoadProbe::Mitigate(
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
ImageLoadProbe::OnRun()
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = AbstractVirtueProbe::OnRun();
	return Status;
}

/**
* @brief The Image Load Notify Callback Routine
* @param FullImageName This images full image name including directory
* @param ProcessId This Images Process ID that's attaching to.  Zero if its a driver load
* @param ImageInfo Pointer to the IMAGE_INFO data structure that describes this image
*/
_Use_decl_annotations_
VOID
ImageLoadProbe::ImageLoadNotificationRoutine(
	PUNICODE_STRING FullImageName,
	HANDLE ProcessId,
	PIMAGE_INFO pImageInfo)
{
	PEPROCESS  pProcess = NULL;

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);

	const NTSTATUS Status = PsLookupProcessByProcessId(ProcessId, &pProcess);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID, "***** Failed to retreve a PEPROCESS for Process Id %p!\n", ProcessId);
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "FullImageName=%wZ,"
		"ProcessId=%p,ImageBase=%p,ImageSize=%p,ImageSectionNumber=%ul\n",
		FullImageName, ProcessId, pImageInfo->ImageBase, (PVOID)pImageInfo->ImageSize,
		pImageInfo->ImageSectionNumber);

	const USHORT bufsz = ROUND_TO_SIZE(sizeof(LoadedImageInfo) + FullImageName->Length, 0x10);
#pragma warning(suppress: 6014)  // we allocate memory, put stuff into, enqueue it and return we do leak!
	const auto buf = new UCHAR[bufsz];
	if (NULL == buf)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for LoadedImageInfo Data!\n");
		goto ErrorExit;
	}

	WVUQueueManager::ProbeInfo* pProbeInfo = WVUQueueManager::GetInstance().FindProbeByName(probe_name);
	if (NULL == pProbeInfo)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_PROCESS, ERROR_LEVEL_ID,
			"***** Unable to find probe info on probe %w!\n", &probe_name);
		goto ErrorExit;
	}
	const PLoadedImageInfo pLoadedImageInfo = (PLoadedImageInfo)buf;
	RtlSecureZeroMemory(buf, bufsz);
	pLoadedImageInfo->ProbeDataHeader.probe_type = ProbeType::ImageLoad;
	pLoadedImageInfo->ProbeDataHeader.data_sz = bufsz;
	pLoadedImageInfo->ProbeDataHeader.probe_id = pProbeInfo->Probe->GetProbeId();
	KeQuerySystemTimePrecise(&pLoadedImageInfo->ProbeDataHeader.current_gmt);
	pLoadedImageInfo->EProcess = pProcess;
	pLoadedImageInfo->ProcessId = ProcessId;
	pLoadedImageInfo->ImageBase = pImageInfo->ImageBase;
	pLoadedImageInfo->ImageSize = pImageInfo->ImageSize;
	pLoadedImageInfo->FullImageNameSz = FullImageName->Length;
	RtlMoveMemory(&pLoadedImageInfo->FullImageName[0], FullImageName->Buffer, pLoadedImageInfo->FullImageNameSz);

	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pLoadedImageInfo->ProbeDataHeader.ListEntry))
	{
#pragma warning(suppress: 26407)
		delete[] buf;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Load Module Enqueue Operation Failed: FullImageName=%wZ,"
			"ProcessId=%p,ImageBase=%p,ImageSize=%p,ImageSectionNumber=%ul\n",
			FullImageName, ProcessId, pImageInfo->ImageBase, (PVOID)pImageInfo->ImageSize,
			pImageInfo->ImageSectionNumber);
	}	

	if (NULL != pProbeInfo && NULL != pProbeInfo->Probe)
	{
		pProbeInfo->Probe->IncrementOperationCount();
		KeQuerySystemTimePrecise(&pProbeInfo->Probe->GetLastProbeRunTime());
	}
ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
}
