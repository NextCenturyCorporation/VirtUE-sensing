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
PVOID CDECL operator new(size_t lBlockSize)
{
    PVOID pVoid = NULL;
    pVoid = ExAllocatePoolWithTag(POOL_TYPE::PagedPool, lBlockSize, COMMON_POOL_TAG);
    return pVoid;
}

/////////////////////////////////////////////////////////////////////
/// @fn operator new[]
/// @brief override the new operator
/// @param lBlockSize size of allocated memory block
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
PVOID CDECL operator new[](size_t lBlockSize)
{
    PVOID pVoid = NULL;
    pVoid = ExAllocatePoolWithTag(POOL_TYPE::PagedPool, lBlockSize, COMMON_POOL_TAG);
	return pVoid;
}


/////////////////////////////////////////////////////////////////////
/// @fn operator delete
/// @brief override the delete operator
/// @param ptr pointer to block of memory to be freed
/// @returns pointer to newly allocated memory
/////////////////////////////////////////////////////////////////////
VOID CDECL operator delete(PVOID ptr)
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
VOID CDECL operator delete[](PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}
