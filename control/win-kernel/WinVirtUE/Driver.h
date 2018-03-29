/**
* @file Driver.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Define the drivers entry points, major function handlers and etc
*/
#pragma once
#include "common.h"
#include "PortMsgTrans.h"
#include "externs.h"

#define DEVICE_TYPE_SHUTDOWN    0xDEADBEEF
#define DRIVER_BINARY_PATH		L"\\??\\c:\\Windows\\System32\\Drivers\\WinVirtUE.sys"
#define DRIVER_SERVICE_KEY      L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\WinVirtUE"
#define DRIVER_SERVICE_IMAGE    L"\\SystemRoot\\System32\\Drivers\\WinVirtUE.sys"
#define DRIVER_SERVICE_START    2   // driver start type
#define DRIVER_SERVICE_TYPE     2   // File System Driver

EXTERN_C_START

_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject);

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath);

EXTERN_C_END