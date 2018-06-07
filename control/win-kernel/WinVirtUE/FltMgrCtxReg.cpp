/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Filter Manager Context Registration
*/
#include "FltMgrCtxReg.h"

#define COMMON_POOL_TAG WVU_CONTEXT_POOL_TAG

#if defined(WVU_DEBUG)
/*
* Instance context allocation balance. A negative number at
* any time will indicate a programmatic logic error.  At
* unload time we want this to read zero.
*/
volatile LONG g_InstanceAllocationBallance = 0;
#endif

/*
* The size of the filter manager context - does NOT include
* data that we add on to that context.
*/
static SIZE_T g_SizeOfFltrMgrInstCtx = 0;

#if defined(WVU_DEBUG)

#pragma region StreamContext Lookaside De/Alllocation Routines
_IRQL_requires_same_
_Function_class_(ALLOCATE_FUNCTION_EX)
static
PVOID
StreamContextLookasideAllocate(
    _In_ POOL_TYPE  PoolType,               // PagedPool 
    _In_ SIZE_T  NumberOfBytes,             // value of Size
    _In_ ULONG  Tag,                        // value of Tag
    _Inout_ PLOOKASIDE_LIST_EX Lookaside    // Lookaside list
)
{
    UNREFERENCED_PARAMETER(Lookaside);
    PVOID pVoid = NULL;
    InterlockedIncrement(&Globals.StreamContextLookasideAllocateCnt);
#pragma warning(suppress: 28160)  /* Invalid Warning */
    pVoid = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    return pVoid;
}

_IRQL_requires_same_
_Function_class_(FREE_FUNCTION_EX)
static
VOID StreamContextLookasideFree(
    _In_ PVOID pStreamContextListEntry,
    _Inout_ PLOOKASIDE_LIST_EX Lookaside)
{
    UNREFERENCED_PARAMETER(Lookaside);
    InterlockedDecrement(&Globals.StreamContextLookasideAllocateCnt);
    ExFreePoolWithTag(pStreamContextListEntry, STREAMCONTEXT_POOL_TAG);
}
#pragma endregion
#endif

//
// The data below is all discardable and pageable.
//
#pragma data_seg( "INIT" )
#pragma const_seg( "INIT" )

//
// Registration Structures
//

const FLT_CONTEXT_REGISTRATION ContextRegistrationData[] =
{
    { FLT_INSTANCE_CONTEXT,
    0,
    (PFLT_CONTEXT_CLEANUP_CALLBACK)InstanceContextCleanup,
    sizeof(WVU_INSTANCE_CONTEXT),   /* ignored */
    INSTANCECONTEXT_POOL_TAG,      /* ignored */
    (PFLT_CONTEXT_ALLOCATE_CALLBACK)InstanceContextAllocate,
    (PFLT_CONTEXT_FREE_CALLBACK)InstanceContextFree,
    NULL
    },

    { FLT_STREAM_CONTEXT,
    0,
    (PFLT_CONTEXT_CLEANUP_CALLBACK)StreamContextCleanup,
    sizeof(WVU_STREAM_CONTEXT),    /* ignored */
    STREAMCONTEXT_POOL_TAG,       /* ignored */
    (PFLT_CONTEXT_ALLOCATE_CALLBACK)StreamContextAllocate,
    (PFLT_CONTEXT_FREE_CALLBACK)StreamContextFree,
    NULL
    },

    { FLT_CONTEXT_END }
};

#pragma data_seg()
#pragma const_seg()


/**
* @brief - custom memory allocator for a WVU_INSTANCE_CONTEXT object
*
* @note The InstanceContext allocators will allocate enough memory for both
* the Filter Mangers context for this stream as well as our own filter defined
* context for this stream.  We determine the size of the filter managers context
* by subtracting the size of WVU_STREAM_CONTEXT from the Size parameter.  That
* difference is the offset into the buffer into which our (bf) context starts.
*
* @param PoolType  - a pointer to the new stream context
* @param Size  - context of current filter instance
* @param ContextType  - structure containing file name information
*
* @return NULL if insufficient resources else pointer to instance context
*/
_Use_decl_annotations_
PVOID InstanceContextAllocate(
    POOL_TYPE PoolType,
    SIZE_T Size,
    FLT_CONTEXT_TYPE ContextType)
{
    PVOID pVoid = NULL;
    PWVU_INSTANCE_CONTEXT pInstanceContext = NULL;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ContextType); // release

    FLT_ASSERTMSG("This allocator is for InstanceContext only!",
        FLT_INSTANCE_CONTEXT == ContextType);

    /*
    * If this the first time we are executing the InstanceContextAllocate
    * function, set the size of the Filter Mangers Context.  This can be
    * derived by subtracting the size of the WVU Stream Context from the
    * the Size parameter as passed to us by the Filter Manager.
    */
    if (0 == g_SizeOfFltrMgrInstCtx)
    {
        g_SizeOfFltrMgrInstCtx = Size - sizeof(WVU_INSTANCE_CONTEXT);
    }

    /*
    * If for some reason the context size changes, let's immediately assert!
    */
    FLT_ASSERTMSG("The g_SizeOfFltrMgrStrmCtx has changed size!",
        g_SizeOfFltrMgrInstCtx == Size - sizeof(WVU_INSTANCE_CONTEXT));

    /*
    * Allocate the actual stream context.
    */
    pVoid = ExAllocatePoolWithTag(NonPagedPool, Size, INSTANCECONTEXT_POOL_TAG);
    if (NULL == pVoid)
    {
        goto Done;
    }

    WVU_DEBUG_PRINT(LOG_CTX, INFO_LEVEL_ID, "Allocated %lu bytes at address %p to %p\n",
        (ULONG)Size, (BYTE*)pVoid, (BYTE*)Add2Ptr(pVoid, Size - 1));
    /*
    * ensure we have a zero'd buffer
    */
    RtlSecureZeroMemory(pVoid, Size);

    /*
    * Since we are appended to the FilterMangers Context, we need to find the
    * address where our context actually starts.  Since we now know how big
    * the Filter Mangers context is, we set the pointer that we'll use to
    * locate this context data by adding that discovered size to the address
    * of the allocated buffer.  From that point on to the end of the buffer
    * is ours to use as we see fit.
    */

    pInstanceContext = (PWVU_INSTANCE_CONTEXT)Add2Ptr(pVoid, g_SizeOfFltrMgrInstCtx);

#if defined(WVU_DEBUG)
    InterlockedIncrement(&g_InstanceAllocationBallance);
#endif

Done:

    return pVoid;
}

/**
* @brief - cleanup a WVU_INSTANCE_CONTEXT object
* @note The Cleanup callbacks provide the 'adjusted' pointer value that does not include
* include the header prefix the context; therefore no pointer adjustment is needed.
* @param pVoid  - a pointer to the instance context to be released
* @param ContextType  - context type of current filter instance
*/
_Use_decl_annotations_
VOID
InstanceContextCleanup(
    PVOID pVoid,
    FLT_CONTEXT_TYPE ContextType)
{
    UNREFERENCED_PARAMETER(ContextType); // release

    FLT_ASSERTMSG("This deallocator is for WVU_INSTANCE_CONTEXT only!",
        FLT_INSTANCE_CONTEXT == ContextType);

    FLT_ASSERTMSG("Find out why we are freeing a NULL WVU_INSTANCE_CONTEXT pointer!",
        NULL != pVoid);

    const PWVU_INSTANCE_CONTEXT pInstanceContext = (PWVU_INSTANCE_CONTEXT)pVoid;
    UNREFERENCED_PARAMETER(pInstanceContext);
}

/**
* @brief - free a WVU_INSTANCE_CONTEXT object
* @note The free callbacks provide the raw pointer value that DOES include
* include the header prefix the context; therefore  pointer adjustment is needed
* to free pointer members.
* @param PVOID  - a void pointer to the entire stream context to be released
* @param ContextType  - context of current filter instance
*/
_Use_decl_annotations_
VOID
InstanceContextFree(
    PVOID pVoid,
    FLT_CONTEXT_TYPE ContextType)
{
    UNREFERENCED_PARAMETER(ContextType); // release

    FLT_ASSERTMSG("Find out why we are freeing a NULL WVU_INSTANCE_CONTEXT pointer!",
        NULL != pVoid);

    FLT_ASSERTMSG("This deallocator is for InstanceContext only!",
        FLT_INSTANCE_CONTEXT == ContextType);

    /*
    * Let's zero out memory to prevent information leakage.
    */
    RtlSecureZeroMemory(pVoid, sizeof(WVU_INSTANCE_CONTEXT));

    WVU_DEBUG_PRINT(LOG_CTX, INFO_LEVEL_ID, "Freeing at address %p\n", (BYTE*)pVoid);
    /*
    * Release the Stream Context
    */
    ExFreePoolWithTag(pVoid, INSTANCECONTEXT_POOL_TAG);
#if defined(WVU_DEBUG)
    InterlockedDecrement(&g_InstanceAllocationBallance);
#endif
}

/*
* The size of the filter manager context - does NOT include
* data that we add on to that context.
*/
static SIZE_T g_SizeOfFltrMgrStrmCtx = 0;

/**
* @brief - custom memory allocator for a streamcontext object
*
* @note The StreamContext allocators will allocate enough memory for both
* the Filter Mangers context for this stream as well as our own filter defined
* context for this stream.  We determine the size of the filter managers context
* by subtracting the size of WVU_STREAM_CONTEXT from the Size parameter.  That
* difference is the offset into the buffer into which our (bf) context starts.
*
* @param PoolType  - a pointer to the new stream context
* @param Size  - context of current filter instance
* @param ContextType  - structure containing file name information
*
* @return NULL if insufficient resources else pointer to stream contexts
*
*/
_Use_decl_annotations_
PVOID StreamContextAllocate(
    POOL_TYPE PoolType,
    SIZE_T Size,
    FLT_CONTEXT_TYPE ContextType)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PVOID pWVUStreamContext = NULL;
    PWVU_STREAM_CONTEXT pStreamContextPointer = NULL;

    UNREFERENCED_PARAMETER(ContextType); // release
    UNREFERENCED_PARAMETER(PoolType);

    FLT_ASSERTMSG("This allocator is for StreamContext only!",
        FLT_STREAM_CONTEXT == ContextType);

    /*
    * If this the first time we are executing the StreamContextAllocate
    * function, set the size of the Filter Mangers Context.  This can be
    * derived by subtracting the size of the WVU Stream Context from the
    * the Size parameter as passed to us by the Filter Manager.
    */
    if (0 == g_SizeOfFltrMgrStrmCtx)
    {
        g_SizeOfFltrMgrStrmCtx = Size - sizeof(WVU_STREAM_CONTEXT);

        status = ExInitializeLookasideListEx(
            &Globals.StreamContextLookaside,
            StreamContextLookasideAllocate,
            StreamContextLookasideFree,
            NonPagedPool,
            0,
            Size,
            STREAMCONTEXT_POOL_TAG,
            0);

        if (!NT_SUCCESS(status))
        {
            goto Done;
        }
    }

    /*
    * If for some reason the context size changes, let's immediately assert!
    */
    FLT_ASSERTMSG("The g_SizeOfFltrMgrStrmCtx has changed size!",
        g_SizeOfFltrMgrStrmCtx == Size - sizeof(WVU_STREAM_CONTEXT));

    /*
    * Allocate the actual stream context.
    */
    pWVUStreamContext = (PWVU_STREAM_CONTEXT)ExAllocateFromLookasideListEx(&Globals.StreamContextLookaside);
    if (NULL == pWVUStreamContext)
    {
        goto Done;
    }

    WVU_DEBUG_PRINT(LOG_CTX, INFO_LEVEL_ID, "Allocated %d bytes at address %p to %p\n",
        (int)Size, (BYTE*)pWVUStreamContext, (BYTE*)Add2Ptr(pWVUStreamContext, Size - 1));

    /*
    * ensure we have a zero'd buffer
    */
    RtlSecureZeroMemory(pWVUStreamContext, Size);

    /*
    * Since we are appended to the FilterMangers Context, we need to find the
    * address where our context actually starts.  Since we now know how big
    * the Filter Mangers context is, we set the pointer that we'll use to
    * locate this context data by adding that discovered size to the address
    * of the allocated buffer.  From that point on to the end of the buffer
    * is ours to use as we see fit.
    */
    pStreamContextPointer = (PWVU_STREAM_CONTEXT)Add2Ptr(pWVUStreamContext, g_SizeOfFltrMgrStrmCtx);

    /*
    * Setup the main context common header
    */
    PFSRTL_COMMON_FCB_HEADER comHeader = (PFSRTL_COMMON_FCB_HEADER)&pStreamContextPointer->ProbeDataHeader;
    comHeader->IsFastIoPossible = FastIoIsPossible;
    comHeader->NodeTypeCode = NodeTypeCode::StreamContext;
    comHeader->NodeByteSize = sizeof(WVU_STREAM_CONTEXT);

#pragma warning( push )
#pragma warning( disable : 4366)  // This is a WDM screw up
    /* Assign the advanced header data portion */
    comHeader->Resource = &pStreamContextPointer->Resource;
    comHeader->PagingIoResource = &pStreamContextPointer->PagingIoResource;
    /* Assign the standard header data portion */
    pStreamContextPointer->ProbeDataHeader.FastMutex = &pStreamContextPointer->FastMutex;
#pragma warning( pop )
    /* initialize the mutex and resources */
    ExInitializeFastMutex(pStreamContextPointer->ProbeDataHeader.FastMutex);
    ExInitializeResourceLite(comHeader->Resource);
    ExInitializeResourceLite(comHeader->PagingIoResource);

    /* configure the advanced header */
    FsRtlSetupAdvancedHeader(&pStreamContextPointer->ProbeDataHeader, pStreamContextPointer->ProbeDataHeader.FastMutex);

Done:

    return pWVUStreamContext;
}

/**
* @brief - cleanup a WVU_STREAM_CONTEXT object
* @note The Cleanup callbacks provide the 'adjusted' pointer value that does not include
* include the header prefix the context; therefore no pointer adjustment is needed.
* @param pVoid  - a pointer to the stream context to be released
* @param ContextType  - context of current filter instance
*/
_Use_decl_annotations_
VOID
StreamContextCleanup(
	_In_ PVOID pContext,
	_In_ FLT_CONTEXT_TYPE ContextType)
{
    UNREFERENCED_PARAMETER(ContextType); // release

    FLT_ASSERTMSG("This deallocator is for WVU_STREAM_CONTEXT only!",
        FLT_STREAM_CONTEXT == ContextType);

    FLT_ASSERTMSG("Find out why we are freeing a NULL WVU_STREAM_CONTEXT pointer!",
        NULL != pContext);

	if (NULL == pContext)
	{
		goto ErrorExit;
	}

    PWVU_STREAM_CONTEXT pStreamContext = (PWVU_STREAM_CONTEXT)pContext;
    pStreamContext = (PWVU_STREAM_CONTEXT)pContext;

    FltReleaseFileNameInformation(pStreamContext->FileNameInformation);

    FltReleaseContext(pStreamContext->InstanceContext);

#pragma warning(suppress: 26110)  // not true, this is a callback function with the lock held
    ExReleaseResourceLite(pStreamContext->ProbeDataHeader.Resource);

    /*
    * Delete the resources.
    */
    ExDeleteResourceLite(pStreamContext->ProbeDataHeader.PagingIoResource);

    FsRtlTeardownPerStreamContexts(&pStreamContext->ProbeDataHeader);

ErrorExit:

#pragma warning(suppress: 26135) // not true, properly annotated
	return;
}

/**
* @brief - free a streamcontext object
* @note The free callbacks provide the raw pointer value that DOES include
* include the header prefix the context; therefore  pointer adjustment is needed
* to free pointer members.
* @param pStreamContext  - a pointer to the stream context to be released
* @param ContextType  - context of current filter instance
*/
_Use_decl_annotations_
VOID
StreamContextFree(
    PVOID pVoid,
    FLT_CONTEXT_TYPE ContextType)
{
    UNREFERENCED_PARAMETER(ContextType); // release

    FLT_ASSERTMSG("This deallocator is for WVU_STREAM_CONTEXT only!",
        FLT_STREAM_CONTEXT == ContextType);

    FLT_ASSERTMSG("Find out why we are freeing a NULL WVU_STREAM_CONTEXT pointer!",
        NULL != pVoid);

    /*
    * Let's zero out memory to prevent information leakage.
    */
    RtlSecureZeroMemory(pVoid, sizeof(WVU_STREAM_CONTEXT));

    WVU_DEBUG_PRINT(LOG_CTX, INFO_LEVEL_ID, "Freeing at address %p\n", (BYTE*)pVoid);
    /*
    * Release the Stream Context
    */
    ExFreeToLookasideListEx(&Globals.StreamContextLookaside, pVoid);
}

/**
* @brief - allocate and initialize a new streamcontext object then add to cache
*
* @param InstanceContext  - context of current filter instance
* @param FileNameInformation  - structure containing file name information
* @param Flags  - flags to be added to the stream context
* @param FileId - the file ID previously retrieved from NTFS
* @param status - STATUS_SUCCESS if created and added to cache
*
* @return pointer to a an allocated stream context
*/
_Use_decl_annotations_
PWVU_STREAM_CONTEXT
CreateStreamContext(
    PFLT_CALLBACK_DATA          Data,
    PFLT_FILE_NAME_INFORMATION  FileNameInformation,
    StreamFlags                 Flags,
    LARGE_INTEGER               FileId,
    PNTSTATUS                   Status)
{
    PWVU_STREAM_CONTEXT pStreamContext = NULL;

    /*
    * We allocate a new stream context we append our context to the end 
    * of the FilterManagers context.  The total size is calculated by 
    * adding the size of the FilterMangers Context to the size of the 
    * WVU_STREAM_CONTEXT as passed through this Filter Manger call.
    */
    *Status = FltAllocateContext(
        Globals.FilterHandle,
        FLT_STREAM_CONTEXT,
        sizeof(WVU_STREAM_CONTEXT),
        NonPagedPool,
        (PFLT_CONTEXT*)&pStreamContext);
    FLT_ASSERTMSG("Unable to allocadte StreamContext!", NT_SUCCESS(*Status));
    if (!NT_SUCCESS(*Status))
    {
        WVU_DEBUG_PRINT(LOG_CTX, ERROR_LEVEL_ID, "%s IRP with file %wZ Unable to allocate  the StreamContext (allocation balance=%ld)!\n",
            FltGetIrpName(Data->Iopb->MajorFunction), Data->Iopb->TargetFileObject->FileName, Globals.StreamContextLookasideAllocateCnt);
        pStreamContext = NULL;
        goto Done;
    }

    //
    // Get the instance context.
    //

    *Status = FltGetInstanceContext(Data->Iopb->TargetInstance, (PFLT_CONTEXT*)&pStreamContext->InstanceContext);
    FLT_ASSERTMSG("Unable to allocadte StreamContext!", NT_SUCCESS(*Status) && NULL != pStreamContext->InstanceContext);
    if (!NT_SUCCESS(*Status) || NULL == pStreamContext->InstanceContext)
    {
        WVU_DEBUG_PRINT(LOG_CTX, ERROR_LEVEL_ID, "%s IRP with file %wZ Unable to retrieve the InstanceContext (allocation balance=%ld)!\n",
            FltGetIrpName(Data->Iopb->MajorFunction), Data->Iopb->TargetFileObject->FileName, Globals.StreamContextLookasideAllocateCnt);
        goto Done;
    }

    pStreamContext->FileNameInformation = FileNameInformation;

    FltReferenceFileNameInformation(pStreamContext->FileNameInformation);

    /* add previously collected flags to context */
    InterlockedOr((PLONG)&pStreamContext->SFlags, Flags);

    FltAcquireResourceExclusive(pStreamContext->ProbeDataHeader.Resource);

    // assigne the file id
    pStreamContext->FileId = FileId;

Done:
    return pStreamContext;
}
