/**
* @file ObjectCreateNotifyCB.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief  Object creation/destruction callback handlers
*/
#pragma once
#include "common.h"
#include "externs.h"


VOID
ProcessNotifyCallbackEx(
    _Inout_ PEPROCESS  Process,
    _In_ HANDLE  ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO  CreateInfo);

VOID
ImageLoadNotificationRoutine(
	_In_ PUNICODE_STRING FullImageName,
	_In_ HANDLE ProcessId,
	_In_ PIMAGE_INFO ImageInfo);

VOID
ThreadCreateCallback(
	_In_ HANDLE  ProcessId,
	_In_ HANDLE  ThreadId,
	_In_ BOOLEAN  Create
);
