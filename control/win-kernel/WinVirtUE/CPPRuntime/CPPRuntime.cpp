/**
* @file CPPRuntime.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief define the c++ runtime
*/

#include "common.h"

#define COMMON_POOL_TAG CPPRUNTIME_POOL_TAG

#define _CRTALLOC(x) __declspec(allocate(x))

#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XPA",long,read)
#pragma section(".CRT$XPZ",long,read)
#pragma section(".CRT$XTA",long,read)
#pragma section(".CRT$XTZ",long,read)
#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)

_CRTALLOC(".CRT$XIA") PVFV __crtXia[] = { NULL };
_CRTALLOC(".CRT$XIZ") PVFV __crtXiz[] = { NULL };
_CRTALLOC(".CRT$XPA") PVFV __crtXpa[] = { NULL };
_CRTALLOC(".CRT$XPZ") PVFV __crtXpz[] = { NULL };
_CRTALLOC(".CRT$XTA") PVFV __crtXta[] = { NULL };
_CRTALLOC(".CRT$XTZ") PVFV __crtXtz[] = { NULL };
_CRTALLOC(".CRT$XCA") PVFV __crtXca[] = { NULL };
_CRTALLOC(".CRT$XCZ") PVFV __crtXcz[] = { NULL };

#pragma comment(linker, "/merge:.CRT=.rdata")

/////////////////////////////////////////////////////////////////////////////
/// @fn CallGlobalInitializers
/// @brief Initialize C++ Global Objects
/////////////////////////////////////////////////////////////////////////////
VOID CallGlobalInitializers()
{
    _Notnull_ PVFV * pfbegin = &__crtXca[0];
	_Notnull_ const PVFV * pfend = &__crtXcz[0];

    InitializeListHead(&exitList);

    while (pfbegin < pfend)
    {
        if (*pfbegin != NULL)
        {
            (**pfbegin)();
        }
        ++pfbegin;
    }
}

/////////////////////////////////////////////////////////////////////////////
/// @fn CallGlobalDestructors
/// @brief Destroy C++ Global Objects
/////////////////////////////////////////////////////////////////////////////
VOID CallGlobalDestructors()
{
    while (!IsListEmpty(&exitList))
    {
        PLIST_ENTRY pEntry = (PLIST_ENTRY)RemoveHeadList(&exitList);
        EXIT_FUNC_LIST *pFuncListEntry = CONTAINING_RECORD(pEntry, EXIT_FUNC_LIST, listEntry);

        if (pFuncListEntry->exitFunc)
        {
            pFuncListEntry->exitFunc();
        }
        FREE_POOL(pFuncListEntry);
    }
}

/////////////////////////////////////////////////////////////////////////////
/// @fn atexit
/// @brief adds a destructor to linked listed to be called at driver unload
/// @param func - the dtor to be called
/////////////////////////////////////////////////////////////////////////////
int CDECL atexit(PVFV func)
{
    PEXIT_FUNC_LIST pFuncListEntry =
        (PEXIT_FUNC_LIST)ALLOC_POOL(NonPagedPool, sizeof(EXIT_FUNC_LIST));

    if (!pFuncListEntry)
    {
        return NULL;
    }  
    RtlSecureZeroMemory(pFuncListEntry, sizeof(EXIT_FUNC_LIST));
    InitializeListHead(&pFuncListEntry->listEntry);
    pFuncListEntry->exitFunc = func;
    InsertHeadList(&exitList, &pFuncListEntry->listEntry);

    return func ? 0 : -1;
}
