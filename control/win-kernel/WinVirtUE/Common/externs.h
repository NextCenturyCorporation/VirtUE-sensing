/**
* @file externs.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief All global variables must have an extern entry in this file
*/
#pragma once
#include "common.h"

//
// Globals
//
extern CONST FLT_OPERATION_REGISTRATION OperationCallbacks[];
extern CONST FLT_CONTEXT_REGISTRATION ContextRegistrationData[];
extern CONST FLT_REGISTRATION FilterRegistration;
extern "C" POBJECT_TYPE *IoDriverObjectType; // POBJECT_TYPE 
extern WVUGlobals Globals;

extern const ULONG HEADER_BLOCK_SIZE;
extern const ULONG ENCRYPT_PAGE_SIZE;
extern const LONGLONG FILE_ALLOCATION_NA;

extern const ULONG FILE_ATTRIBUTES_NA;
extern class ProcessInfoMap *pProcInfoMap;
extern class ProcessTracker *pProcessTracker;

