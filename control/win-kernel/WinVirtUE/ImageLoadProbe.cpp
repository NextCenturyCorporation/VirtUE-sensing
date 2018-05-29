
/**
* @file ImageLoadProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ImageLoad Probe Class definiopm
*/
#include "ImageLoadProbe.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_IMAGELOADPROBE_POOL_TAG


ImageLoadProbe::ImageLoadProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "PsCreateSystemThread():  Successfully Constructed The Image Load Sensor\n");
}


ImageLoadProbe::~ImageLoadProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "PsCreateSystemThread():  Successfully Destroyed The Image Load Sensor\n");
}

BOOLEAN ImageLoadProbe::Enable()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

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

BOOLEAN ImageLoadProbe::Disable()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	Status = PsRemoveLoadImageNotifyRoutine(ImageLoadProbe::ImageLoadNotificationRoutine);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "PsRemoveLoadImageNotifyRoutine(ImageLoadNotificationRoutine) "
			"Remove Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "PsRemoveLoadImageNotifyRoutine(): Successfully Disabled Image Load Sensor\n");

ErrorExit:

	return NT_SUCCESS(Status);
}

BOOLEAN ImageLoadProbe::State()
{
	return this->Enabled;
}

BOOLEAN ImageLoadProbe::Mitigate()
{
	return FALSE;
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
	UNREFERENCED_PARAMETER(FullImageName);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(pImageInfo);

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
	const PUCHAR buf = new UCHAR[bufsz];
	if (NULL == buf)
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Unable to allocate memory via new() for LoadedImageInfo Data!\n");
		goto ErrorExit;
	}

	const PLoadedImageInfo pLoadedImageInfo = (PLoadedImageInfo)buf;
	pLoadedImageInfo->Header.Type = DataType::LoadedImage;
	pLoadedImageInfo->Header.DataSz = bufsz;
	KeQuerySystemTime(&pLoadedImageInfo->Header.CurrentGMT);
	pLoadedImageInfo->EProcess = pProcess;
	pLoadedImageInfo->ProcessId = ProcessId;
	pLoadedImageInfo->ImageBase = pImageInfo->ImageBase;
	pLoadedImageInfo->ImageSize = pImageInfo->ImageSize;
	pLoadedImageInfo->FullImageNameSz = FullImageName->Length;
	RtlMoveMemory(&pLoadedImageInfo->FullImageName[0], FullImageName->Buffer, pLoadedImageInfo->FullImageNameSz);

	pPDQ->Enqueue(&pLoadedImageInfo->Header.ListEntry);

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
}
