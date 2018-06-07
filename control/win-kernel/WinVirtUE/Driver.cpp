/**
* @file Driver.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief main module of the Windows VirtUE miniFilter driver.
*/
#include "Driver.h"
#define COMMON_POOL_TAG WVU_DRIVER_POOL_TAG

// globaL scoped variables
WVUGlobals Globals;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

/**
* @brief retrieve and display the operating system version an service pack information
* @note side effect is to store off to global var OS version Information
* @retval this operations status
*/
static NTSTATUS GetOsVersion()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RtlSecureZeroMemory(&Globals.lpVersionInformation, sizeof(Globals.lpVersionInformation));
	Globals.lpVersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&Globals.lpVersionInformation);
	if (!NT_SUCCESS(Status))
	{
		goto Error;
	}

	WVU_DEBUG_PRINT(LOG_MAIN, INFO_LEVEL_ID, "******************************\n");
	// TODO:  Generate a proper version number
	WVU_DEBUG_PRINT(LOG_MAIN, INFO_LEVEL_ID, "***** WinVirtUE.sys Version %d.%d.%d\n", 0, 1, 0);
	WVU_DEBUG_PRINT(LOG_MAIN, INFO_LEVEL_ID, "***** Windows Version %u.%u.%u Service Pack %u.%u\n",
		Globals.lpVersionInformation.dwMajorVersion, Globals.lpVersionInformation.dwMinorVersion, Globals.lpVersionInformation.dwBuildNumber,
		Globals.lpVersionInformation.wServicePackMajor, Globals.lpVersionInformation.wServicePackMinor);

	if (Globals.lpVersionInformation.szCSDVersion[0] != (TCHAR)0)
	{
		WVU_DEBUG_PRINT(LOG_MAIN, INFO_LEVEL_ID, "***** Service Pack: %ws\n", &Globals.lpVersionInformation.szCSDVersion[0]);
	}

	WVU_DEBUG_PRINT(LOG_MAIN, INFO_LEVEL_ID, "******************************\n");
Error:
	return Status;
}


/*************************************************************************
MiniFilter initialization and unload routines.
*************************************************************************/

/**
* @brief The driver unload callback.
* @note This call by default not made unless a ioctl is sent
* @param DriverObject  Pointer to driver object created by the system to
* represent this driver
*/
_Use_decl_annotations_
extern "C" VOID
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	Globals.ShuttingDown = TRUE;

	WVUDebugBreakPoint();

	// destroy global object instances
	CallGlobalDestructors();
	
	return _IRQL_requires_same_ VOID();
}

/**
* @brief This is the initialization routine for this miniFilter driver.
* This registers with FltMgr and initializes all global data structures.
* @param DriverObject  Pointer to driver object created by the system to
* represent this driver
* @param RegistryPath Unicode string identifying where the parameters for this
* driver are located in the registry
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
extern "C"
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES MainThdObjAttr = { 0,0,0,0,0,0 };
	OBJECT_ATTRIBUTES WVUPortObjAttr = { 0,0,0,0,0,0 };
	PSECURITY_DESCRIPTOR pWVUPortSecDsc = NULL;
	HANDLE MainThreadHandle = (HANDLE)-1;
	CLIENT_ID MainClientId = { (HANDLE)-1,(HANDLE)-1 };

	UNICODE_STRING usPortName = { 0,0,NULL };	

	LARGE_INTEGER timeout = { 0LL };
	timeout.QuadPart = RELATIVE(SECONDS(10)); // -1000 * 1000 * 10 * 10;  // ten second timeout

	WVUDebugBreakPoint();
	
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	Globals.DriverObject = DriverObject;  // let's save the DO off for future use

	DriverObject->DriverUnload = DriverUnload;  // For now, we unload by default

	Globals.AllowFilterUnload =
#if defined(WVU_DEBUG)
		TRUE;
#else
		FALSE;
#endif

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "About to call CallGlobalInitializers()!\n");

	CallGlobalInitializers();

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "CallGlobalInitializers() Completed!\n");

	// initialize the waiter.  Once the WVUThreadStart gets to the end of its
	// intialization, it will signal and wait simultaneously.  It will continue
	// to wait until the DriverUnload routine signals it.  When the thread
	// continues the objects created will have their destructors called.
	KeInitializeEvent(&Globals.WVUThreadStartEvent, EVENT_TYPE::SynchronizationEvent, FALSE);	

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "About to register filter manager callbacks!\n");

	//  Register with FltMgr to tell it our callback routines
	Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Globals.FilterHandle);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltRegisterFilter() FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	ExInitializeRundownProtection(&Globals.RunDownRef);
	Globals.ShuttingDown = FALSE;

	Status = GetOsVersion();
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, WARNING_LEVEL_ID, "RtlGetVersion Failed! Status=%08x\n", Status);
	}

	//
	//  Create a communication port.
	//
	RtlInitUnicodeString(&usPortName, WVUPortName);

	//
	//  We secure the port so only ADMINs & SYSTEM can acecss it.
	//
	Status = FltBuildDefaultSecurityDescriptor(&pWVUPortSecDsc, FLT_PORT_ALL_ACCESS);

	if (NT_SUCCESS(Status))
	{

		InitializeObjectAttributes(&WVUPortObjAttr,
			&usPortName,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			pWVUPortSecDsc);

		Status = FltCreateCommunicationPort(Globals.FilterHandle,
			&Globals.WVUServerPort,
			&WVUPortObjAttr,
			Globals.DriverObject,
			WVUPortConnect,
			WVUPortDisconnect,
			WVUMessageNotify,
			2);

		//
		//  Free the security descriptor in all cases. It is not needed once
		//  the call to FltCreateCommunicationPort() is made.
		//
		FltFreeSecurityDescriptor(pWVUPortSecDsc);

		if (NT_SUCCESS(Status))
		{
			Globals.EnableProtection = TRUE;

			//  Start filtering i/o
			Status = FltStartFiltering(Globals.FilterHandle);
			if (FALSE == NT_SUCCESS(Status))
			{
				WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltStartFiltering() Failed! - FAIL=%08x\n", Status);
				FltUnregisterFilter(Globals.FilterHandle);
				goto ErrorExit;
			}
		}
		else
		{
			WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltCreateCommunicationPort() Failed! - FAIL=%08x\n", Status);
			goto ErrorExit;
		}
	}
	
	InitializeObjectAttributes(&MainThdObjAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	// create thread, register stuff and etc
	Status = PsCreateSystemThread(&MainThreadHandle, GENERIC_ALL, &MainThdObjAttr, NULL, &MainClientId, WVUMainThreadStart, &Globals.WVUThreadStartEvent);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "PsCreateSystemThread() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}
	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "PsCreateSystemThread():  Successfully created Main thread %p process %p thread id %p\n",
		MainThreadHandle, MainClientId.UniqueProcess, MainClientId.UniqueThread);

	Status = KeWaitForSingleObject(&Globals.WVUThreadStartEvent, KWAIT_REASON::Executive, KernelMode, FALSE, &timeout);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAINTHREAD, ERROR_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Failed! Status=%08x\n", Status);
		goto ErrorExit;
	}
	switch (Status)
	{
	case STATUS_SUCCESS:
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Returned SUCCESS\n");
		break;
	case STATUS_TIMEOUT:
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Timed Out\n");
		Status = STATUS_TIMEOUT;
		goto ErrorExit;
		break;
	default:
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "KeWaitForSingleObject(WVUMainThreadStart,...) Thread Has Just Received Status=0x%08x\n", Status);
		goto ErrorExit;
		break;
	}
	
	goto Exit;  // normal non-error return


ErrorExit:

	KeSetEvent(&Globals.WVUThreadStartEvent, IO_NO_INCREMENT, FALSE);  // exits the intialization thread

	// wait for all of that to end
	ExWaitForRundownProtectionRelease(&Globals.RunDownRef);

	// destroy global object instances
	CallGlobalDestructors();

Exit:

	return Status;
}
