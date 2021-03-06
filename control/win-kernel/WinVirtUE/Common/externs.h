/**
* @file externs.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief All global variables must have an extern entry in this file
*/
#pragma once
#include "types.h"

//
// Globals
//
extern CONST FLT_OPERATION_REGISTRATION OperationCallbacks[];
extern CONST FLT_CONTEXT_REGISTRATION ContextRegistrationData[];
extern CONST FLT_REGISTRATION FilterRegistration;
extern "C" POBJECT_TYPE *IoDriverObjectType; // POBJECT_TYPE 
extern WVUGlobals Globals;

extern CONST ULONG HEADER_BLOCK_SIZE;

extern CONST LONGLONG FILE_ALLOCATION_NA;
extern CONST ULONG FILE_ATTRIBUTES_NA;

extern class ImageLoadProbe *pILP;
extern class ProcessCreateProbe *pPCP;
extern class ProcessListValidationProbe *pPLVP;
extern class RegistryModificationProbe *pRMP;
extern class ThreadCreateProbe *pTCP;

extern const ANSI_STRING one_shot_kill;
extern const UUID ZEROGUID;




