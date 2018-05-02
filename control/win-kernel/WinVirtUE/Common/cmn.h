/**
* @file cmn.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief Define common defines here
*/
#pragma once
#include "common.h"

#define STDCALL __stdcall
#define CDECL  __cdecl
#define INLINE __inline

#define INVALID_HANDLE_VALUE (HANDLE)-1
#define MAX_PATH 260
#define LENGTH_OF(ary)  (sizeof(ary) / sizeof(ary[0]))

#define LEAVE_IF_NOTTRUE_WITH_ACTION(cond,theAction) {\
	if (!(cond))\
			{\
	DbgPrint("ERROR in %s:%s(%d) " #cond " is %x instead of TRUE\n",\
	__FILE__,\
	__FUNCTION__,\
	__LINE__,\
	cond);\
	theAction;\
	__debugbreak();\
	__leave;\
			}\
}
#define LEAVE_IF_NOTTRUE(cond,status) LEAVE_IF_NOTTRUE_WITH_ACTION(cond,retVal=status)

#define LEAVE_IF_NOTSUCCESS(retVal) {\
	if (!NT_SUCCESS(retVal))\
			{\
	DbgPrint("ERROR in %s:%s(%d) " #retVal " is %x instead of success\n",\
	__FILE__,\
	__FUNCTION__,\
	__LINE__,\
	retVal);\
	__debugbreak();\
	__leave;\
			}\
}
