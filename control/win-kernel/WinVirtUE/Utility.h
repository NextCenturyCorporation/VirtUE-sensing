/**
* @file Utility.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Utility header file
*/
#pragma once
#include "common.h"
_IRQL_always_function_max_(DISPATCHER_LEVEL)
_IRQL_always_function_min_(PASSIVE_LEVEL)
_Check_return_
_Success_(0 == return)
LONG CompareAnsiString(_In_ CONST ANSI_STRING& string1, 
	_In_ CONST ANSI_STRING& string2, 
	_In_ BOOLEAN IgnoreCase = FALSE);

