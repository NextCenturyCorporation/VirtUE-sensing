
/**
* @file ImageLoadProbe.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief ImageLoad Probe Class definition
*/
#include "ImageLoadProbe.h"
#include "ProbeDataQueue.h"
#define COMMON_POOL_TAG WVU_IMAGELOADPROBE_POOL_TAG


ImageLoadProbe::ImageLoadProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Constructed The Image Load Sensor\n");
}


ImageLoadProbe::~ImageLoadProbe()
{
	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "Successfully Destroyed The Image Load Sensor\n");
}

_Use_decl_annotations_
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

_Use_decl_annotations_
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

_Use_decl_annotations_
BOOLEAN ImageLoadProbe::State()
{
	return this->Enabled;
}

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
	RtlSecureZeroMemory(buf, bufsz);
	pLoadedImageInfo->ProbeDataHeader.MessageId = 0LL;
	pLoadedImageInfo->ProbeDataHeader.ReplyLength = 0L;
	pLoadedImageInfo->ProbeDataHeader.Type = DataType::LoadedImage;
	pLoadedImageInfo->ProbeDataHeader.DataSz = bufsz;
	KeQuerySystemTimePrecise(&pLoadedImageInfo->ProbeDataHeader.CurrentGMT);
	pLoadedImageInfo->EProcess = pProcess;
	pLoadedImageInfo->ProcessId = ProcessId;
	pLoadedImageInfo->ImageBase = pImageInfo->ImageBase;
	pLoadedImageInfo->ImageSize = pImageInfo->ImageSize;
	pLoadedImageInfo->FullImageNameSz = FullImageName->Length;
	RtlMoveMemory(&pLoadedImageInfo->FullImageName[0], FullImageName->Buffer, pLoadedImageInfo->FullImageNameSz);

	if (FALSE == pPDQ->Enqueue(&pLoadedImageInfo->ProbeDataHeader.ListEntry))
	{
		delete[] buf;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "***** Load Module Enqueue Operation Failed: FullImageName=%wZ,"
			"ProcessId=%p,ImageBase=%p,ImageSize=%p,ImageSectionNumber=%ul\n",
			FullImageName, ProcessId, pImageInfo->ImageBase, (PVOID)pImageInfo->ImageSize,
			pImageInfo->ImageSectionNumber);
	}

ErrorExit:

	// Drop a rundown reference 
	ExReleaseRundownProtection(&Globals.RunDownRef);
}
