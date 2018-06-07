/**
* @file new.cpp
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief Redefine the new, new[], delete and delete[] operators
*/
// wdm struct padding, sign conversion not defined macros, unref'd inline function removal
// forward declared enum, default ctor or assign operator not generated, user defined ctor req'd
#include "cmn.h"

#define COMMON_POOL_TAG NEW_POOL_TAG

/////////////////////////////////////////////////////////////////////
/// @fn operator new
/// @brief override the new operator
/// @param lBlockSize size of allocated memory block
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
__drv_allocatesMem(object)
PVOID CDECL operator new(_In_ size_t lBlockSize)
{
    PVOID pVoid = NULL;
    pVoid = ExAllocatePoolWithTag(NonPagedPool, lBlockSize, COMMON_POOL_TAG);
    return pVoid;
}

/////////////////////////////////////////////////////////////////////
/// @fn operator new[]
/// @brief override the new operator
/// @param lBlockSize size of allocated memory block
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
__drv_allocatesMem(object)

PVOID CDECL operator new[](_In_ size_t lBlockSize)
{
    PVOID pVoid = NULL;
    pVoid = ExAllocatePoolWithTag(NonPagedPool, lBlockSize, COMMON_POOL_TAG);
	return pVoid;
}


/////////////////////////////////////////////////////////////////////
/// @fn operator delete
/// @brief override the delete operator
/// @param ptr pointer to block of memory to be freed
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID CDECL operator delete(
	_Pre_notnull_ 
	__drv_freesMem(object) PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}

/////////////////////////////////////////////////////////////////////
/// @fn operator delete[]
/// @brief override the delete operator
/// @param ptr pointer to block of memory to be freed
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID CDECL operator delete[](
	_Pre_notnull_ 
	__drv_freesMem(object) PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}
