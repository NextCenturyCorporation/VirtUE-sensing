/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief main module of the WVU miniFilter driver.
*/
#include "Debug.h"
#define COMMON_POOL_TAG WVU_DEBUG_POOL_TAG

#if defined(WVU_DEBUG)

/**
* Provide the string representation of the file name,
* and it will be printed to the debug console. This
* function will do nothing if 'DEBUG' is not defined.
*
* @param String -- UNICODE_STRING or WCHAR string.
* @param Type -- Type of string, see STRING_TYPES enum
* @param Level -- Print level
*/
_Use_decl_annotations_
void
WVUDebugPrintFileName(
	PVOID String, 
	INT32 Type, 
	INT32 Level)
{
    switch (Type)
    {
#pragma warning( push )
#pragma warning( disable : 4365)  // This is a WDM screwup
    case STRING_UNICODE:
        DbgPrintEx(COMPONENT_ID, Level, "%wZ", *((UNICODE_STRING*)String));
        break;
    case STRING_WIDE:
        DbgPrintEx(COMPONENT_ID, Level, "%S", (WCHAR *)String);
#pragma warning( pop )
        break;
    default:
        FLT_ASSERTMSG("Incorrect use of WVUDebugPrintFileName.", FALSE);
        return;
    }
}

/**
* Add a break point to the code. This function will
* do nothing if 'DEBUG' is not defined.
*/
void
WVUDebugBreakPoint()
{
    DbgBreakPoint();
}

/**
* Print a buffer with some pretty formatting. This function will
* do nothing if 'DEBUG' is not defined.
*
* @param Buffer -- the buffer you want to print
* @param Size -- Size of the buffer
*/
_Use_decl_annotations_
void
WVUDebugPrintBuffer(
	const PUCHAR Buffer,
	UINT32 Size)
{
	UINT32 i = 0;
    const UINT32 printSize = min(0x100, Size);

    for (; i < printSize; i++)
    {
        /* Formatting and such */
        switch (i % 0x10)
        {
        case 0:
            DbgPrintEx(COMPONENT_ID, FLOOD_LEVEL_ID, "%04x: %02x ", i,
                Buffer[i]);
            break;
        case 0xF:
            DbgPrintEx(COMPONENT_ID, FLOOD_LEVEL_ID, "%02x\n", Buffer[i]);
            break;
        default:
            DbgPrintEx(COMPONENT_ID, FLOOD_LEVEL_ID, "%02x ", Buffer[i]);
        }
    }

    if (printSize < Size)
    {
        DbgPrintEx(COMPONENT_ID, FLOOD_LEVEL_ID, "%04x: ...\n", i);
    }
    else
    {
        DbgPrintEx(COMPONENT_ID, FLOOD_LEVEL_ID, "\n");
    }
}

/**
* Break if the filename for the file being inspected matches
* the target. This function will do nothing if 'DEBUG' is
* not defined.
*
* @param String -- The file name we are inspecting
* @param Target -- The file name we care about
* @param Type -- UNICODE_STRING or WCHAR string, see STRING_TYPES enum
*/
_Use_decl_annotations_
void
WVUDebugBreakOnMatch(
	PVOID String, 
	const WCHAR *Target, 
	INT32 Type)
{
    WCHAR *wStr = NULL;
    PCUNICODE_STRING uStr = NULL;
    BOOLEAN match = FALSE;
    size_t wStrLen = 0;
    NTSTATUS status = STATUS_SUCCESS;

    switch (Type)
    {
    case STRING_UNICODE:
        uStr = (PCUNICODE_STRING)String;
        wStrLen = sizeof(WCHAR) * ((size_t)uStr->Length + 1);
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
        wStr = (PWCHAR)ALLOC_POOL(NonPagedPool, wStrLen);
        FLT_ASSERTMSG("ALLOC_POOL", NULL == wStr);
        if (NULL == wStr)
        {
            goto Done;
        }

        status = RtlStringCbCopyUnicodeString(wStr, wStrLen, uStr);
        FLT_ASSERTMSG("RtlStringCchCopyUnicodeString Failed!", NT_SUCCESS(status));
        if (!NT_SUCCESS(status))
        {
            goto Done;
        }
        break;
    case STRING_WIDE:
        wStr = (WCHAR*)String;
        break;
    default:
        FLT_ASSERTMSG("Incorrect use of DebugBreakOnMatch.", FALSE);
        return;
    }

    if (wcsstr(wStr, Target) != NULL)
    {
        match = TRUE;
    }

Done:
    if (NULL != wStr)
    {
        FREE_POOL(wStr);
    }

    if (match == TRUE)
    {
        DbgBreakPoint();
    }
}
#endif
