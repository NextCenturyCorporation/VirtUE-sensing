/**
* @file common.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief All header files are included here to be included elsewhere
*/
#pragma once

// wdm warnings that need to be eliminated for us to run 
// @ all warnings and warnings are errors
#pragma warning( push )
#pragma warning( disable : 4365 4668 4471 4510 4512 4610 )

#include <fltKernel.h>
#include <Ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdmsec.h>
#include <ntifs.h>
#include <ntddkbd.h>
#include <sal.h>
#include <concurrencysal.h>
#include <bcrypt.h>

#pragma warning( pop )

#include "Utility.h"
#include "config.h"
#include "trace.h"
#include "CPPRuntime.h"
#include "cmn_pool_tag.h"
#include "types.h"
#include "cmn.h"
#include "UnDoc.h"
#include "debug.h"
#include "WVUKernelUserComms.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

/******************************************************************************
* Helper macros                                                              *
******************************************************************************/
#ifndef NUMBER_OF
#define NUMBER_OF(x) ( sizeof(x) / sizeof(x[0]) )
#endif

/** if it's a kernel address the high 16 bits are set? */
#define IS_KERNEL_ADDRESS(addr) (0xFFFF000000000000 == (0xFFFF000000000000 & addr))

#define ABS(N) ((N<0)?(-N):(N))

/* Wait / Time macros */
#define ABSOLUTE(wait) (wait)

#define RELATIVE(wait) (-(wait))

#define NANOSECONDS(nanos) \
    (((signed __int64) (nanos)) / 100L)

#define MICROSECONDS( micros ) \
    (((signed __int64) (micros)) * NANOSECONDS(1000L))

#define MILLISECONDS( milli ) \
    (((signed __int64) (milli)) * MICROSECONDS(1000L))

#define SECONDS( seconds ) \
	(((signed __int64) (seconds) * MILLISECONDS(1000)))

#define ALLOC_POOL(PoolType, Size) ExAllocatePoolWithTag(PoolType, Size, COMMON_POOL_TAG)
#define FREE_POOL(Buffer)  ExFreePoolWithTag(Buffer, COMMON_POOL_TAG);

/** The header block size */
CONST ULONG HEADER_BLOCK_SIZE = 1024 * 4;
/** Encrypt Page size */
CONST ULONG ENCRYPT_PAGE_SIZE = 512;

/** When File Allocation Stream Size is Not Applicable */
CONST LONGLONG FILE_ALLOCATION_NA = 0;
/** When File Attributes are not used */
CONST ULONG FILE_ATTRIBUTES_NA = 0;


/** an easier way to loop through a linked list */
#define LIST_FOR_EACH_ENTRY(pos, head, T, Field) \
    for (T* pos = CONTAINING_RECORD((head).Flink, T, Field); \
		!IsListEmpty(&head) && NULL != pos && pos != (T*)(&head); \
		pos = (T*)pos->ListEntry.Flink )

/** Assume that ListEntry is the list name */
#define LIST_FOR_EACH(pos, head, T) \
	LIST_FOR_EACH_ENTRY(pos, head, T, ListEntry)

#define TOLOWERCASE(ch) (ch | (1 << 5))
#define TOUPPERCASE(ch) (ch & ~(1<< 5))
