/**
* @file FltMgrCallbacks.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief main module of the OW miniFilter driver.
*/
#include "FltMgrCallbacks.h"
#define COMMON_POOL_TAG FLTMGR_OP_CALLBACK_POOL_TAG

static ULONG_PTR OperationStatusCtx = 1;

#pragma region Operation Registration Data

//
// The data below is all discardable and pageable.
//

#pragma data_seg( "INIT" )
#pragma const_seg( "INIT" )
 
//
//  operation registration
//
CONST FLT_OPERATION_REGISTRATION OperationCallbacks[] = {
	{ 
		IRP_MJ_CREATE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{
		IRP_MJ_CREATE_NAMED_PIPE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation
	},

	{
		IRP_MJ_CLOSE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation
	},

	{
		IRP_MJ_READ,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation
	},

	{ 
		IRP_MJ_WRITE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_QUERY_INFORMATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_SET_INFORMATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_QUERY_EA,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_SET_EA,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_FLUSH_BUFFERS,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_QUERY_VOLUME_INFORMATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
	
		IRP_MJ_SET_VOLUME_INFORMATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_DIRECTORY_CONTROL,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_FILE_SYSTEM_CONTROL,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_DEVICE_CONTROL,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_INTERNAL_DEVICE_CONTROL,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_SHUTDOWN,
		0,
		WinVirtUEShutdownPreOp,
		NULL //post operations not supported
	},                               

	{ 
		IRP_MJ_LOCK_CONTROL,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_CLEANUP,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_CREATE_MAILSLOT,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_QUERY_SECURITY,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_SET_SECURITY,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation
	},

	{ 
		IRP_MJ_QUERY_QUOTA,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_SET_QUOTA,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_PNP,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_RELEASE_FOR_MOD_WRITE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_RELEASE_FOR_CC_FLUSH,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_NETWORK_QUERY_OPEN,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_MDL_READ,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_MDL_READ_COMPLETE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_PREPARE_MDL_WRITE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_MDL_WRITE_COMPLETE,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_VOLUME_MOUNT,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

	{ 
		IRP_MJ_VOLUME_DISMOUNT,
		0,
		WinVirtUEPreOperation,
		WinVirtUEPostOperation 
	},

    { IRP_MJ_OPERATION_END }
};

#pragma data_seg()
#pragma const_seg()

#pragma endregion

/*************************************************************************
MiniFilter callback routines.
*************************************************************************/

/**
* @brief This routine is a pre-operation dispatch routine for this miniFilter.
* This is non-pageable because it could be called on the paging path

* @param Data Pointer to the filter callbackData that is passed to us
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance, its associated volume and
* file object.
* @param CompletionContext The context for the completion routine for this
* operation.
* @return Operations Callback Status
*/
_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS
WinVirtUEPreOperation(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID *CompletionContext)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
		"WinVirtUE!WinVirtUEPreOperation: Entered\n");

	//
	//  See if this is an operation we would like the operation status
	//  for.  If so request it.
	//
	//  NOTE: most filters do NOT need to do this.  You only need to make
	//        this call if, for example, you need to know if the oplock was
	//        actually granted.
	//

	if (WinVirtUEDoRequestOperationStatus(Data)) {

		status = FltRequestOperationStatusCallback(Data,
			WinVirtUEOperationStatusCallback,
			(PVOID)(++OperationStatusCtx));
		if (!NT_SUCCESS(status)) {
			WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
				"WinVirtUE!WinVirtUEPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
					status);
		}
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/**
* @brief This routine is called when the given operation returns from the call
* to IoCallDriver.  This is useful for operations where STATUS_PENDING
* means the operation was successfully queued.  This is useful for OpLocks
* and directory change notification operations.
*
* This callback is called in the context of the originating thread and will
* never be called at DPC level.  The file object has been correctly
* referenced so that you can access it.  It will be automatically
* dereferenced upon return.
* This is non-pageable because it could be called on the paging path
*
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance, its associated volume and
* file object.
* @param ParameterSnapshot
* @param OperationStatus 
* @param RequesterContext context for the completion routine for this operation.
* @return Operations Callback Status
*/
_Use_decl_annotations_
VOID
WinVirtUEOperationStatusCallback(
	PCFLT_RELATED_OBJECTS FltObjects,
	const PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	NTSTATUS OperationStatus,
	PVOID RequesterContext)
{
	UNREFERENCED_PARAMETER(FltObjects);

#ifndef WVU_DEBUG
	UNREFERENCED_PARAMETER(ParameterSnapshot);
	UNREFERENCED_PARAMETER(OperationStatus);
	UNREFERENCED_PARAMETER(RequesterContext);
#endif // WVU_DEBUG

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
		"WinVirtUE!WinVirtUEOperationStatusCallback: Entered\n");

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
		"WinVirtUE!WinVirtUEOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
			OperationStatus,
			RequesterContext,
			ParameterSnapshot->MajorFunction,
			ParameterSnapshot->MinorFunction,
			FltGetIrpName(ParameterSnapshot->MajorFunction));
}

/**
* @brief post-operation completion routine for this miniFilter.
* non-pageable because it could be called on the paging path
* @param Data Pointer to the filter callbackData that is passed to us
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance, its associated volume and
* file object.
* @param CompletionContext The context for the completion routine for this
* operation.
* @param Flags Denotes whether the completion is successful or is being drained
* @return Operations Callback Status
*/
_Use_decl_annotations_
FLT_POSTOP_CALLBACK_STATUS
WinVirtUEPostOperation(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
		"WinVirtUE!WinVirtUEPostOperation: Entered\n");

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/**
* @brief pre-operation dispatch routine for this miniFilter
* non-pageable because it could be called on the paging path
* @param Data Pointer to the filter callbackData that is passed to us
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance, its associated volume and
* file object.
* @param CompletionContext The context for the completion routine for this
* operation.
* @return Operations Callback Status
*/
_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS
WinVirtUEPreOperationNoPostOperation(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID *CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,	
		"WinVirtUE!WinVirtUEPreOperationNoPostOperation: Entered\n");

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/**
* @brief This identifies those operations we want the operation status for.  These
* are typically operations that return STATUS_PENDING as a normal completion
* status.
* @param Data Pointer to the filter callbackData that is passed to us
* @return TRUE If we want the operation status else FALSE
*/
_Use_decl_annotations_
BOOLEAN
WinVirtUEDoRequestOperationStatus(
	PFLT_CALLBACK_DATA Data)
{
	CONST PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	//
	//  return boolean state based on which operations we are interested in
	//

	return (BOOLEAN)

		//
		//  Check for oplock operations
		//

		(((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
		((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

			||

			//
			//    Check for change notification
			//

			((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
			(iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
			);
}


/**
* @brief pre-operation dispatch routine for this miniFilter
* non-pageable because it could be called on the paging path
* @param Data Pointer to the filter callbackData that is passed to us
* @param FltObjects Pointer to the FLT_RELATED_OBJECTS data structure containing
* opaque handles to this filter, instance, its associated volume and
* file object.
* @param CompletionContext The context for the completion routine for this
* operation.
* @return Operations Callback Status
*/
_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS
WinVirtUEShutdownPreOp(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID *CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	WVU_DEBUG_PRINT(LOG_OP_CALLBACKS, TRACE_LEVEL_ID,
		"WinVirtUE!WinVirtUEShutdownPreOp: Entered\n");
	Globals.ShuttingDown = TRUE;  // make sure we exit the loop/thread in the queue processor

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}