/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief filter registeration module
*/
#include "FltMgrReg.h"
#include "WVUProbeManager.h"

#define COMMON_POOL_TAG WVU_FLT_REG_POOL_TAG

#pragma region Filter Registration
//
// The data below is all discardable and pageable.
//

#pragma data_seg( "INIT" )
#pragma const_seg( "INIT" )
//
//
//  This defines what we want to filter with FltMgr
//

#pragma warning(suppress: 26485)  // false positive, cannot decay if it's not an array
CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),         //  Size
    FLT_REGISTRATION_VERSION,         //  Version
#if defined(WVU_DEBUG)
    NULL,                             //  Flags - changes filter behavior
#else
    FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,  // Flags - do NOT allow a service stop when not compiled with WVU_DEBUG
#endif

    ContextRegistrationData,            //  ContextRegistration
	OperationCallbacks,                 //  Operation callbacks

    (PFLT_FILTER_UNLOAD_CALLBACK)WVUUnload,                              //  MiniFilterUnload
    (PFLT_INSTANCE_SETUP_CALLBACK)WVUInstanceSetup,                      //  InstanceSetup
    (PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK)WVUInstanceQueryTeardown,     //  InstanceQueryTeardown
    (PFLT_INSTANCE_TEARDOWN_CALLBACK)WVUInstanceTeardownStart,           //  InstanceTeardownStart
    (PFLT_INSTANCE_TEARDOWN_CALLBACK)WVUInstanceTeardownComplete,        //  InstanceTeardownComplete

    (PFLT_GENERATE_FILE_NAME)WVUGenerateFileNameCallback,                //  GenerateFileName    
    NULL,                                                                //  NormalizeNameComponent
    NULL,                                                                //  NormalizeContextCleanupCallback;
    NULL,                                                                //  TransactionNotificationCallback;
    (PFLT_NORMALIZE_NAME_COMPONENT_EX)WVUNormalizeNameComponentExCallback  //  NormalizeNameComponentExCallback;
};

#pragma data_seg()
#pragma const_seg()
#pragma endregion

#pragma region Module Scoped Locals
/**
* @brief Queries a volume for name information
* @note APC or lower IRQL required
* @param VolumeName  A pointer to a UNICODE_STRING that has been passed to QueryVolumeInformation.
*/
static
VOID
FreeVolumeName(
    _In_ PUNICODE_STRING VolumeName)
{
    FLT_ASSERTMSG("VolumeName is NULL!", NULL != VolumeName);
    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");
    // Back up the point to the beginning of the block.
    ExFreePoolWithTag(
        CONTAINING_RECORD(
            VolumeName->Buffer,
            FILE_FS_VOLUME_INFORMATION,
            VolumeLabel),
		WVU_QUERY_VOLUME_NAME_POOL_TAG);
}

/**
* @brief Queries a volume for name information
* @note PASSIVE IRQL required
* @param FltInstance The filter instance for the volume
* @param InstanceContext A pointer to the InstanceContext
* @return STATUS_SUCCESS or an appropriate NTSTATUS value.
*/
_Must_inspect_result_
_Success_(TRUE == NT_SUCCESS(return))
static
NTSTATUS
QueryVolumeName(
    _In_ PFLT_INSTANCE FltInstance,
    _Inout_ PWVU_INSTANCE_CONTEXT InstanceContext)
{
    IO_STATUS_BLOCK IoStatus = { 0, 0 };
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_FS_VOLUME_INFORMATION VolumeInformation = NULL;
    static CONST INT AllocSize = sizeof(FILE_FS_VOLUME_INFORMATION) + VOLUME_NAME_MAX_BYTES;

    FLT_ASSERTMSG("FltInstance is NULL!", FltInstance != NULL);
    FLT_ASSERTMSG("InstanceContext is NULL!", InstanceContext != NULL);

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    // Allocate the volume info struct.
    VolumeInformation = (PFILE_FS_VOLUME_INFORMATION)ExAllocatePoolWithTag(
        PagedPool,
        AllocSize,
		WVU_QUERY_VOLUME_NAME_POOL_TAG);
    if (NULL == VolumeInformation)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
            "ExAllocatePoolWithTag failed: status = 0x%x\n",
            Status);
        goto Done;
    }

    // Grab the volume name.
    Status = FltQueryVolumeInformation(
        FltInstance,
        &IoStatus,
        VolumeInformation,
        AllocSize,
        FileFsVolumeInformation);
    // This will fail on networked drives, etc.
    if (NT_SUCCESS(Status))
    {
        InstanceContext->VolumeName.Length = (USHORT)VolumeInformation->VolumeLabelLength;
        InstanceContext->VolumeName.MaximumLength = InstanceContext->VolumeName.Length;
        InstanceContext->VolumeName.Buffer = &VolumeInformation->VolumeLabel[0];
        InstanceContext->AllocatedVolumeName = TRUE;
    }
    else
    {
        ExFreePoolWithTag(
            VolumeInformation,
            WVU_QUERY_VOLUME_NAME_POOL_TAG);
        InstanceContext->AllocatedVolumeName = FALSE;
    }

Done:

    return Status;
}

/**
* @brief Queries a volume for name information
* @note PASSIVE IRQL required
* @param FltInstance The filter instance for the volume
* @param InstanceContext A pointer to the InstanceContext
* @return STATUS_SUCCESS or an appropriate NTSTATUS value.
*/
_Must_inspect_result_
_Success_(TRUE == NT_SUCCESS(return))
static
NTSTATUS
QueryVolumeInformation(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ PWVU_INSTANCE_CONTEXT InstanceContext)
{
    FILE_FS_ATTRIBUTE_INFORMATION AttributeInformation = { 0,0,0,{ 0 } };
    FILE_FS_DEVICE_INFORMATION DeviceInformation = { 0,0 };
    IO_STATUS_BLOCK IoStatus = { 0,0 };
    FILE_FS_SIZE_INFORMATION SizeInformation = { 0L,0L,0,0 };
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_FS_VOLUME_INFORMATION VolumeInformation = NULL;
	UCHAR VolumeInfoStack[sizeof(FILE_FS_VOLUME_INFORMATION) + 128] = {};

    UNREFERENCED_PARAMETER(VolumeDeviceType); // release

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    VolumeInformation = (PFILE_FS_VOLUME_INFORMATION)&VolumeInfoStack[0];

    Status = FltQueryVolumeInformation(
        FltObjects->Instance,
        &IoStatus,
        &SizeInformation,
        sizeof(SizeInformation),
        FileFsSizeInformation);

    // FltQueryVolumeInformation will fail on a network volume. 
    if (!NT_SUCCESS(Status))
    {
        FLT_ASSERTMSG("Volume Device Type must be network file system if failed!",
            VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);
        InstanceContext->SectorSize = ENCRYPT_PAGE_SIZE;
        InstanceContext->AllocationSize = ENCRYPT_PAGE_SIZE;
        Status = STATUS_SUCCESS;
    }
    else
    {
        InstanceContext->SectorSize = SizeInformation.BytesPerSector;
        InstanceContext->AllocationSize =
            SizeInformation.SectorsPerAllocationUnit *
            InstanceContext->SectorSize;
    }

    RtlSecureZeroMemory(&IoStatus, sizeof(IoStatus));
    RtlSecureZeroMemory(&AttributeInformation, sizeof(AttributeInformation));

    // Query read only status. Handle standard buffer overflow since we don't 
    // care about the FS name.
    Status = FltQueryVolumeInformation(
        FltObjects->Instance,
        &IoStatus,
        &AttributeInformation,
        sizeof(AttributeInformation),
        FileFsAttributeInformation);

    if (!NT_SUCCESS(Status)
        && STATUS_BUFFER_OVERFLOW == Status
#pragma warning(suppress: 6102)  // False Warning
        && IoStatus.Information == sizeof(AttributeInformation))
    {
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
#pragma warning(suppress: 6102)  // False Warning
        if (FlagOn(AttributeInformation.FileSystemAttributes, FILE_READ_ONLY_VOLUME))
        {
            InterlockedOr((PLONG)&InstanceContext->Flags, InstanceFlags::InstanceFlagReadOnly);
        }
    }
    else
    {
        FLT_ASSERTMSG("Volume Device Type must be network file system if failed!",
            VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);
        Status = STATUS_SUCCESS;
    }

    // Query device characteristics.
    Status = FltQueryVolumeInformation(
        FltObjects->Instance,
        &IoStatus,
        &DeviceInformation,
        sizeof(DeviceInformation),
        FileFsDeviceInformation);

    if (NT_SUCCESS(Status))
    {
        if (FlagOn(DeviceInformation.Characteristics, FILE_REMOVABLE_MEDIA))
        {
            InterlockedOr(
                (PLONG)&InstanceContext->Flags,
                InstanceFlags::InstanceFlagRemovableMedia);
        }
    }
    else
    {
        FLT_ASSERTMSG("Volume Device Type must be network file system if failed!",
            VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

#pragma endregion

/**
* @brief This is the unload routine for this miniFilter driver. This is called
* when the minifilter is about to be unloaded. We can fail this unload
* request if this is not a mandatory unload indicated by the Flags parameter.
* @note This IRP will originate from the Tcpip driver
* @param Flags Indicating if this is a mandatory unload
* @retval this operations callback status
*/
_Use_decl_annotations_
NTSTATUS
WVUUnload(
    FLT_FILTER_UNLOAD_FLAGS Flags)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PKTHREAD pThread = NULL;

    UNREFERENCED_PARAMETER(Flags);

    if (FLTFL_FILTER_UNLOAD_MANDATORY == (FLTFL_FILTER_UNLOAD_MANDATORY & Flags))
    {
        WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "---===> *Mandatory* Windows VirtUE File System Filter Unload In Progress!!!\n");
    }
    else if (FALSE == Globals.AllowFilterUnload)       
    {
        WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Windows VirtUE File System Filter Unload Aborted Because AllowFilterUnload is FALSE!\n");
        Status = STATUS_FLT_DO_NOT_DETACH;
		goto Error;
        
    }

	WVUDebugBreakPoint();

	WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Windows VirtUE Filter Unload Proceeding . . .\n");

	// cause the WVU Thread to proceed with object destruction
	KeSetEvent(&Globals.WVUThreadStartEvent, IO_NO_INCREMENT, FALSE);

	Status = ObReferenceObjectByHandle(Globals.MainThreadHandle, (SYNCHRONIZE | DELETE), NULL, KernelMode, (PVOID*)&pThread, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "ObReferenceObjectByHandle() Failed! - FAIL=%08x\n", Status);
	}
	else
	{
		Status = KeWaitForSingleObject((PVOID)pThread, Executive, KernelMode, FALSE, (PLARGE_INTEGER)0);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(WVUMainInitThread,...) Failed waiting for the Sensor Thread! Status=%08x\n", Status);
		}
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, TRACE_LEVEL_ID, "Main Thread Exited w/Status=0x%08x!\n", PsGetThreadExitStatus(pThread));
		(VOID)ObDereferenceObject(pThread);
	}

	// wait for all of that to end
	ExWaitForRundownProtectionRelease(&Globals.RunDownRef);
	if (NULL != pWVUMgr)
	{
		delete pWVUMgr;
	}
	// we've made, all is well
	Status = STATUS_SUCCESS;

Error:

    return Status;
}

/**
* @brief InstanceSetup Callback for this filter
* @param FltObjects to an FLT_RELATED_OBJECTS structure that contains opaque pointers for the objects related to the current operation
* @param Flags Bitmask of flags that indicate why the instance is being attached. One or more of the following
* @param VolumeDeviceType Device type of the file system volume.
* @param VolumeFilesystemType File system type of the volume
* @return returns STATUS_SUCCESS or STATUS_FLT_DO_NOT_ATTACH
*/
_Use_decl_annotations_
NTSTATUS
WVUInstanceSetup(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_SETUP_FLAGS Flags,
    DEVICE_TYPE VolumeDeviceType,
    FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
    PWVU_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING defaultVolumeName = RTL_CONSTANT_STRING(L"Unknown");

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    /*
    * For now we only support NTFS. FAT may
    * or may not work. For now, this is fine,
    * but it may need to be changed as we support
    * other file systems (network?)
    */
    if (FLT_FSTYPE_NTFS != VolumeFilesystemType)
    {
        Status = STATUS_FLT_DO_NOT_ATTACH;
        goto Done;
    }

    /*
    * Create an instance context so we can get the sector size for this
    * volume. 
    */
    Status = FltAllocateContext(
        Globals.FilterHandle,
        FLT_INSTANCE_CONTEXT,
        sizeof(WVU_INSTANCE_CONTEXT),
        NonPagedPool,
        (PFLT_CONTEXT*)&InstanceContext);
    FLT_ASSERTMSG("Unable to allocate Instance Context", NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        InstanceContext = NULL;
        WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID, "FltAllocateContext Failed: Status = 0x%x\n", Status);
        goto Done;
    }

    // Set the context.
    Status = FltSetInstanceContext(
        FltObjects->Instance,
        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
        InstanceContext,
        NULL);
    FLT_ASSERTMSG("Unable to set Instance Context", NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID, "FltSetInstanceContext Failed: Status = 0x%x\n", Status);
        goto Done;
    }

    // At this point we should have valid sector and allocation sizes.
    InstanceContext->FilesystemType = VolumeFilesystemType;

    // Query sector/allocation size.
    Status = QueryVolumeInformation(
        FltObjects,
        VolumeDeviceType,
        InstanceContext);
    FLT_ASSERTMSG("QueryVolumeInformation Failed", NT_SUCCESS(Status));

    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
    {
        InterlockedOr((PLONG)&InstanceContext->Flags, InstanceFlags::InstanceFlagNetwork);
    }

    FLT_ASSERTMSG("SectorSize must be greated than zero!", 
        InstanceContext->SectorSize > 0);
    FLT_ASSERTMSG("SectorSize must be greated than zero!", 
        InstanceContext->AllocationSize > 0);
    FLT_ASSERTMSG("SectorSize must be modulo zero ENCRYPT_PAGE_SIZE", 
        InstanceContext->SectorSize % ENCRYPT_PAGE_SIZE == 0);

    // See if we can get volume information. We don't care if this fails. It's 
    // just for diagnostic purposes.
    Status = QueryVolumeName(FltObjects->Instance, InstanceContext);

    PUNICODE_STRING VolumeName = NT_SUCCESS(Status) ? &InstanceContext->VolumeName : &defaultVolumeName;
    Status = STATUS_SUCCESS;

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID,
        "Attaching to device %wZ (DevType %u, FsType %d, RO %u, Removable %u)\n",
        VolumeName,
        VolumeDeviceType,
        VolumeFilesystemType,
        BooleanFlagOn(InstanceContext->Flags, InstanceFlags::InstanceFlagReadOnly),
        BooleanFlagOn(InstanceContext->Flags, InstanceFlags::InstanceFlagRemovableMedia));

Done:

    if (InstanceContext != NULL)
    {
        FltReleaseContext(InstanceContext);
    }
    return Status;
}

/**
* @brief  This is called when an instance is being manually deleted by a
* call to FltDetachVolume or FilterDetach thereby giving us a
* chance to fail that detach request.
* @note If this routine is not defined in the registration structure, explicit
* detach requests via FltDetachVolume or FilterDetach will always be failed.
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance and its associated volume
* @param Flags Indicating where this detach request came from
* @return status of this operation
*/
_Use_decl_annotations_
NTSTATUS
WVUInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    return STATUS_SUCCESS;
}

/**
* @brief  This routine is called at the start of instance teardown
* chance to fail that detach request.
* @note If this routine is not defined in the registration structure, explicit
* detach requests via FltDetachVolume or FilterDetach will always be failed.
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance and its associated volume
* @param Flags Reason why this instance is being deleted.
*/
_Use_decl_annotations_
VOID
WVUInstanceTeardownStart(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PWVU_INSTANCE_CONTEXT InstanceContext = NULL;
    UNREFERENCED_PARAMETER(Flags);

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    Status = FltGetInstanceContext(
        FltObjects->Instance,
        (PFLT_CONTEXT*)&InstanceContext);
    if (!NT_SUCCESS(Status))
    {
        WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID, "FltGetInstanceContext Failed: status = 0x%x\n", Status);
        InstanceContext = NULL;
        goto Done;
    }

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID,
        "Starting teardown to context %p (device %wZ)\n",
        InstanceContext, &InstanceContext->VolumeName);

    if (InstanceContext->AllocatedVolumeName)
    {
        FreeVolumeName(&InstanceContext->VolumeName);
    }

    if (InstanceContext != NULL)
    {
        FltReleaseContext(InstanceContext);
    }

Done:

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID,
        "Finished teardown to context %p\n",
        InstanceContext);

    return;
}

/**
* @brief  This routine is called at the end of instance teardown
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance and its associated volume
* @param Flags Reason why this instance is being deleted.
*/
_Use_decl_annotations_
VOID
WVUInstanceTeardownComplete(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PWVU_INSTANCE_CONTEXT InstanceContext = NULL;

    UNREFERENCED_PARAMETER(Flags);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    Status = FltGetInstanceContext(
        FltObjects->Instance,
        (PFLT_CONTEXT*)&InstanceContext);
    if (!NT_SUCCESS(Status))
    {
        InstanceContext = NULL;
        WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
            "FltGetInstanceContext failed to retrieve context for, Object Instance is %p\n",
             FltObjects->Instance);
        ASSERT(!"FltGetInstanceContext");
        goto Done;
    }

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID,
        "Completed tear down on Instance %p, Object Instance is %p\n",
        InstanceContext, FltObjects->Instance);
    if (InstanceContext != NULL)
    {
        FltReleaseContext(InstanceContext);
    }

Done:
    FltDeleteInstanceContext(FltObjects->Instance, NULL);
    return;
}

/**
* @brief  A minifilter driver that provides file names for the filter manager's name cache 
* @param Instance Opaque instance pointer for the minifilter driver instance that this callback routine is registered for
* @param FileObject A pointer to a file object for the file whose name is being requested
* @param CallbackData A pointer to the callback data structure for the operation during which this name is being requested. This parameter is NULL when FltGetFileNameInformationUnsafe is called to retrieve the name of the file
* @param NameOptions FLT_FILE_NAME_OPTIONS value that specifies the name format, query method, and flags for this file name information query
* @param CacheFileNameInformation A pointer to a Boolean value specifying whether this name can be cached. Set to TRUE on output if the name can be cached; set to FALSE otherwise
* @param FileName A pointer to a filter manager-allocated FLT_NAME_CONTROL structure to receive the file name on output
* @returns returns STATUS_SUCCESS or an appropriate NTSTATUS value
*/
_Use_decl_annotations_
NTSTATUS 
WVUGenerateFileNameCallback(
      PFLT_INSTANCE Instance,
      PFILE_OBJECT FileObject,
      PFLT_CALLBACK_DATA CallbackData,
      FLT_FILE_NAME_OPTIONS NameOptions,
      PBOOLEAN CacheFileNameInformation,
      PFLT_NAME_CONTROL FileName)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PFLT_FILE_NAME_INFORMATION belowFileName = NULL;    

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    __try 
    {

        //
        //  We expect to only get requests for opened and short names.
        //  If we get something else, fail. Please note that it is in
        //  fact possible that if we get a normalized name request the
        //  code would work because it's not really doing anything other 
        //  than calling FltGetFileNameInformation which would handle the
        //  normalized name request just fine. However, in a real name 
        //  provider this might require a different implementation. 
        //

        if (!FlagOn(NameOptions, FLT_FILE_NAME_OPENED) &&
            !FlagOn(NameOptions, FLT_FILE_NAME_SHORT)) 
        {

            FLT_ASSERTMSG("Received a request for an unknown format - Investigate!", FALSE);
            status =  STATUS_NOT_SUPPORTED;
            __leave;
        }

        //  
        // First we need to get the file name. We're going to call   
        // FltGetFileNameInformation below us to get the file name from FltMgr.   
        // However, it is possible that we're called by our own minifilter for   
        // the name so in order to avoid an infinite loop we must make sure to   
        // remove the flag that tells FltMgr to query this same minifilter.   
        //  
        ClearFlag(NameOptions, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER);

        //  
        // this will be called for FltGetFileNameInformationUnsafe as well and  
        // in that case we don't have a CallbackData, which changes how we call   
        // into FltMgr.  
        //  
        if (NULL == CallbackData)
        {
            //  
            // This must be a call from FltGetFileNameInformationUnsafe.  
            // However, in order to call FltGetFileNameInformationUnsafe the   
            // caller MUST have an open file (assert).  
            //  
            ASSERT(FileObject->FsContext != NULL);
            status = FltGetFileNameInformationUnsafe(FileObject,
                Instance,
                NameOptions,
                &belowFileName);
            if (!NT_SUCCESS(status)) 
            {
                belowFileName = NULL;
                WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
                    "FltGetFileNameInformationUnsafe failed to retrieve filename for Instance %p\n", 
                    Instance);
                __leave;
            }
        }
        else 
        {
            //  
            // We have a callback data, we can just call FltMgr.  
            //  
            status = FltGetFileNameInformation(CallbackData,
                NameOptions,
                &belowFileName);
            if (!NT_SUCCESS(status)) 
            {
                WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
                    "FltGetFileNameInformation failed to retrieve filename for CallbackData %p\n",
                    CallbackData);
                belowFileName = NULL;
                __leave;
            }
        }

        if (NULL == FileName)
        {
            status = STATUS_NOT_SUPPORTED;
            __leave;
        }
        //  
        // At this point we have a name for the file (the opened name) that   
        // we'd like to return to the caller. We must make sure we have enough   
        // buffer to return the name or we must grow the buffer. This is easy   
        // when using the right FltMgr API.  
        //  
#pragma warning(suppress: 6001)
        status = FltCheckAndGrowNameControl(FileName, belowFileName->Name.Length);
        if (!NT_SUCCESS(status)) 
        {
            WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
                "FltCheckAndGrowNameControl failed: Status = 0x%x\n",
                status);
            __leave;
        }
        //  
        // There is enough buffer, copy the name from our local variable into  
        // the caller provided buffer.  
        //  
        RtlCopyUnicodeString(&FileName->Name, &belowFileName->Name);
        //  
        // And finally tell the user they can cache this name.  
        //  
        *CacheFileNameInformation = TRUE;
    }
    __finally 
    {
        if (belowFileName != NULL) 
        {
            FltReleaseFileNameInformation(belowFileName);
        }
    }
    return status;
}

/**
* @brief Provides file names for the filter manager's name cache
* @param Instance Opaque instance pointer for the minifilter driver instance that this callback routine is registered for
* @param FileObject Pointer to the file object for the file whose name is being requested or the file that is the target of the IRP_MJ_SET_INFORMATION operation if the FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME flag is set. See the Flags parameter below for more information.
* @param ParentDirectory Pointer to a UNICODE_STRING structure that contains the name of the parent directory for this name component
* @param VolumeNameLength Length, in bytes, of the parent directory name that is stored in the structure that the ParentDirectory parameter points to
* @param CacheFileNameInformation A pointer to a Boolean value specifying whether this name can be cached. Set to TRUE on output if the name can be cached; set to FALSE otherwise
* @param Component Pointer to a UNICODE_STRING structure that contains the name component to be expanded
* @param ExpandComponentName Pointer to a FILE_NAMES_INFORMATION structure that receives the expanded (normalized) file name information for the name component. 
* @param ExpandComponentNameLength Length, in bytes, of the buffer that the ExpandComponentName parameter points to
* @param Flags Name normalization flags
* @param NormalizationContextPointer to minifilter driver-provided context information to be passed in any subsequent calls to this callback routine that are made to normalize the remaining components in the same file name path
* @returns returns STATUS_SUCCESS or an appropriate NTSTATUS value
*/
_Use_decl_annotations_
NTSTATUS
WVUNormalizeNameComponentExCallback(
    PFLT_INSTANCE Instance,
    PFILE_OBJECT FileObject,
    PCUNICODE_STRING ParentDirectory,
    USHORT VolumeNameLength,
    PCUNICODE_STRING Component,
    PFILE_NAMES_INFORMATION ExpandComponentName,
    ULONG ExpandComponentNameLength,
    FLT_NORMALIZE_NAME_FLAGS Flags,
    PVOID *NormalizationContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE parentDirHandle = NULL;
    OBJECT_ATTRIBUTES parentDirAttributes{ 0,0,0,0,0,0 };
    BOOLEAN isDestinationFile = FALSE;
    BOOLEAN isCaseSensitive = FALSE;
    IO_STATUS_BLOCK ioStatus = { 0,0 };
    IO_DRIVER_CREATE_CONTEXT driverContext = { 0,0,0,0 };
    PTXN_PARAMETER_BLOCK txnParameter = NULL;

    UNREFERENCED_PARAMETER(VolumeNameLength);
    UNREFERENCED_PARAMETER(NormalizationContext);    

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Entered\n");

    __try 
    {
        //  
        // Initialize the boolean variables. we only use the case sensitivity  
        // one but we initialize both just to point out that you can tell   
        // whether Component is a "destination" (target of a rename or hardlink  
        // creation operation).  
        //  
        isCaseSensitive = BooleanFlagOn(Flags,
            FLTFL_NORMALIZE_NAME_CASE_SENSITIVE);
        isDestinationFile = BooleanFlagOn(Flags,
            FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME);
        //  
        // Open the parent directory for the component we're trying to   
        // normalize. It might need to be a case sensitive operation so we   
        // set that flag if necessary.  
        //  
        InitializeObjectAttributes(&parentDirAttributes,
            (PUNICODE_STRING)ParentDirectory,
            (ULONG)(OBJ_KERNEL_HANDLE | (isCaseSensitive ? OBJ_CASE_INSENSITIVE : 0)),
            NULL,
            NULL);
  
        //  
        // In Vista and newer this must be done in the context of the same  
        // transaction the FileObject belongs to.      
        //  
        IoInitializeDriverCreateContext(&driverContext);
        if (NULL == FileObject)
        {
            __leave;
        }
        txnParameter = IoGetTransactionParameterBlock(FileObject);
        driverContext.TxnParameters = txnParameter;
        status = FltCreateFileEx2(Globals.FilterHandle,
            Instance,
            &parentDirHandle,
            NULL,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            &parentDirAttributes,
            &ioStatus,
            0,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK,
            &driverContext);

        if (!NT_SUCCESS(status)) 
        {
            WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
                "FltCreateFileEx2 failed to retrieve Driver Context : status = 0x%x\n",
                status);
            parentDirHandle = NULL;
            __leave;
        }
        //  
        // Now that we have a handle to the parent directory of Component, we  
        // need to query its long name from the file system. We're going to use  
        // ZwQueryDirectoryFile because the handle we have for the directory   
        // was opened with FltCreateFile and so targeting should work just fine.  
        //  
        status = ZwQueryDirectoryFile(parentDirHandle,
            NULL,
            NULL,
            NULL,
            &ioStatus,
            ExpandComponentName,
            ExpandComponentNameLength,
            FileNamesInformation,
            TRUE,
            (PUNICODE_STRING)Component,
            TRUE);
        if (!NT_SUCCESS(status))
        {
            WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID,
                "ZwQueryDirectoryFile failed to retreive components long name : status = 0x%x\n",
                status);        
        }
    }
    __finally 
    {
        if (parentDirHandle != NULL) 
        {
            FltClose(parentDirHandle);
        }
    }
    return status;
}
