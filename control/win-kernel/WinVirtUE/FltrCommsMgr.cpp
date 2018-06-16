/**
* @file FltrCommsMgr.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Filter Communications Manager Class definition
*/
#include "FltrCommsMgr.h"
#include "Driver.h"
#define COMMON_POOL_TAG WVU_FLTCOMMSMGR_POOL_TAG

CONST PWSTR FltrCommsMgr::CommandName = L"\\WVUCommand"; // Command/Response Comms Port
CONST PWSTR FltrCommsMgr::PortName = L"\\WVUPort";		 // Real-Time streaming data as sent to the API

/**
* @brief destroys this Filter Comms Manager Instance
* @param OnConnect If not NULL, then call this method when a user space application disconnects
* @param OnDisconnect If not NULL, then call this method when a user space application connects
*/
FltrCommsMgr::FltrCommsMgr()
	: usPortName({ 0,0,nullptr }), usCommandName({ 0,0,nullptr }),  WVUPortObjAttr({ 0,0,0,0,0,0 }),
	WVUComandObjAttr({ 0,0,0,0,0,0 }), pWVUPortSecDsc(nullptr), pWVUCommandSecDsc(nullptr),
	InitStatus(STATUS_UNSUCCESSFUL)	
{
	

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "About to register filter manager callbacks!\n");

	//  Register with FltMgr to tell it our callback routines
	InitStatus = FltRegisterFilter(Globals.DriverObject, &::FilterRegistration, &Globals.FilterHandle);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltRegisterFilter() FAIL=%08x\n", InitStatus);
		goto ErrorExit;
	}

	//
	//  Create communications ports.
	//
	RtlInitUnicodeString(&usPortName, FltrCommsMgr::PortName);
	RtlInitUnicodeString(&usCommandName, FltrCommsMgr::CommandName);

	//
	//  We secure the port so only ADMINs & SYSTEM can acecss it.
	//
	InitStatus = FltBuildDefaultSecurityDescriptor(&pWVUPortSecDsc, FLT_PORT_ALL_ACCESS);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltBuildDefaultSecurityDescriptor(pWVUPortSecDsc) FAIL=%08x\n", InitStatus);
		goto ErrorExit;
	}

	InitStatus = FltBuildDefaultSecurityDescriptor(&pWVUCommandSecDsc, FLT_PORT_ALL_ACCESS);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltBuildDefaultSecurityDescriptor(pWVUCommandSecDsc) FAIL=%08x\n", InitStatus);
		goto ErrorExit;
	}
	
	InitializeObjectAttributes(&WVUPortObjAttr,
		&usPortName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		pWVUPortSecDsc);

	InitializeObjectAttributes(&WVUComandObjAttr,
		&usCommandName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		pWVUCommandSecDsc);
	
ErrorExit:

	return;
}

/**
* @brief destroys this Filter Comms Manager Instance
*/
FltrCommsMgr::~FltrCommsMgr()
{
	FltUnregisterFilter(Globals.FilterHandle);

	Globals.EnableProtection = FALSE;
}

/**
* @brief Enables the Filter Manager and Port Communications
*/
_Use_decl_annotations_
BOOLEAN 
FltrCommsMgr::Enable()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (FALSE == NT_SUCCESS(this->InitStatus))
	{		
		Status = InitStatus;
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltrCommsMgr::FltrCommsMgr() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	//
	// create the filter data communcations port
	//
	Status = FltCreateCommunicationPort(Globals.FilterHandle,
		&Globals.WVUDataPort,
		&WVUPortObjAttr,
		Globals.DriverObject,
		(PFLT_CONNECT_NOTIFY)&FltrCommsMgr::WVUPortConnect,
		(PFLT_DISCONNECT_NOTIFY)&FltrCommsMgr::WVUPortDisconnect,
		NULL,
		NUMBER_OF_PERMITTED_CONNECTIONS);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltCreateCommunicationPort() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	//
	// create the filter command communcations port
	//
	Status = FltCreateCommunicationPort(Globals.FilterHandle,
		&Globals.WVUCommandPort,
		&WVUComandObjAttr,
		Globals.DriverObject,
		(PFLT_CONNECT_NOTIFY)&FltrCommsMgr::WVUCommandConnect,
		(PFLT_DISCONNECT_NOTIFY)&FltrCommsMgr::WVUCommandDisconnect,
		(PFLT_MESSAGE_NOTIFY)&FltrCommsMgr::WVUCommandMessageNotify,
		NUMBER_OF_PERMITTED_CONNECTIONS);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltCreateCommunicationPort() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	//
	//  Free security descriptors in all cases. It is not needed once
	//  the call to FltCreateCommunicationPort() is made.
	//
	FltFreeSecurityDescriptor(pWVUPortSecDsc);
	FltFreeSecurityDescriptor(pWVUCommandSecDsc);
	
	//  Start filtering i/o
	Status = FltStartFiltering(Globals.FilterHandle);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_MAIN, ERROR_LEVEL_ID, "FltStartFiltering() Failed! - FAIL=%08x\n", Status);
		FltUnregisterFilter(Globals.FilterHandle);
		goto ErrorExit;
	}

	Globals.EnableProtection = TRUE;

ErrorExit:

	return NT_SUCCESS(Status) ? TRUE : FALSE;
}
/**
* @brief Disables the Filter Manager and Port Communications 
*/
VOID 
FltrCommsMgr::Disable()
{
	// close the server port
	FltCloseCommunicationPort(Globals.WVUDataPort);
	FltCloseCommunicationPort(Globals.WVUCommandPort);
	Globals.WVUDataPort = NULL;
	Globals.WVUCommandPort = NULL;
}

/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterConnectCommunicationPort to send a connection request to the mini-filter driver
* @param ClientPort Opaque handle for the new client port that is established between the user-mode application and the kernel-mode mini-filter driver.
* @param ServerPortCookie Pointer to context information defined by the mini-filter driver
* @param ConnectionContext Context information pointer that the user-mode application passed in the lpContext parameter to FilterConnectCommunicationPort.
* @param SizeOfContext Size, in bytes, of the buffer that ConnectionContextpoints to
* @param ConnectionPortCookie Pointer to information that uniquely identifies this client port
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
NTSTATUS FLTAPI 
FltrCommsMgr::WVUPortConnect(
	PFLT_PORT ClientPort,
	PVOID ServerPortCookie,
	PVOID ConnectionContext,
	ULONG SizeOfContext,
	PVOID *ConnectionPortCookie)
{
	const NTSTATUS Status = STATUS_SUCCESS;
	FLT_ASSERTMSG("ClientPort Must Be NULL!!", NULL == Globals.ClientPort);
	FLT_ASSERTMSG("UserProcess Must Be NULL!!", NULL == Globals.UserProcess);

	Globals.UserProcess = PsGetCurrentProcess();
	Globals.ClientPort = ClientPort;
	*ConnectionPortCookie = Globals.DriverObject;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Port Connected by Process 0x%p Port 0x%p!\n",
		Globals.UserProcess, Globals.ClientPort);
	
	if (nullptr != pPDQ)
	{
		pPDQ->OnConnect();
	}
	return Status;
}

/**
* @brief Filter Port Disconnect Callback
* @note callback routine to be called whenever the user-mode handle count for the client port reaches zero or when the minifilter driver is about to be unloaded
* @param ConnectionCookie  Pointer to information that uniquely identifies this client port
* represent this driver
*/
_Use_decl_annotations_
VOID FLTAPI 
FltrCommsMgr::WVUPortDisconnect(
	PVOID ConnectionPortCookie)
{
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Port Disconnected - Port 0x%p!\n", Globals.ClientPort);
	// close our handle to the connection 
	FltCloseClientPort(Globals.FilterHandle, &Globals.ClientPort);

	// Reset the user process field
	Globals.UserProcess = NULL;

	// Ensure that the ClientPort is NULL as well
	Globals.ClientPort = NULL;

	Globals.ConnectionCookie = NULL;

	if (nullptr != pPDQ)
	{
		pPDQ->OnDisconnect();
	}	
}

/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterConnectCommunicationPort to send a connection request to the mini-filter driver
* @param ClientPort Opaque handle for the new client port that is established between the user-mode application and the kernel-mode mini-filter driver.
* @param ServerPortCookie Pointer to context information defined by the mini-filter driver
* @param ConnectionContext Context information pointer that the user-mode application passed in the lpContext parameter to FilterConnectCommunicationPort.
* @param SizeOfContext Size, in bytes, of the buffer that ConnectionContextpoints to
* @param ConnectionPortCookie Pointer to information that uniquely identifies this client port
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
NTSTATUS FLTAPI 
FltrCommsMgr::WVUCommandConnect(
	PFLT_PORT ClientPort, 
	PVOID ServerPortCookie,
	PVOID ConnectionContext, 
	ULONG SizeOfContext, 
	PVOID * ConnectionPortCookie)
{
	const NTSTATUS Status = STATUS_SUCCESS;
	FLT_ASSERTMSG("ClientPort Must Be NULL!!", NULL == Globals.ClientPort);
	FLT_ASSERTMSG("UserProcess Must Be NULL!!", NULL == Globals.UserProcess);

	Globals.UserProcess = PsGetCurrentProcess();
	Globals.ClientPort = ClientPort;
	*ConnectionPortCookie = Globals.DriverObject;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Port Connected by Process 0x%p Port 0x%p!\n",
		Globals.UserProcess, Globals.ClientPort);

	if (nullptr != pPDQ)
	{
		; // replace this with a relevant call
	}
	return Status;
}

/**
* @brief Filter Command Port Disconnect Callback
* @note callback routine to be called whenever the user-mode handle count for the client port reaches zero or when the minifilter driver is about to be unloaded
* @param ConnectionCookie  Pointer to information that uniquely identifies this client port
* represent this driver
*/
_Use_decl_annotations_
VOID FLTAPI 
FltrCommsMgr::WVUCommandDisconnect(
	PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Command Port Disconnected - Port 0x%p!\n", Globals.ClientPort);
	// close our handle to the connection 
	FltCloseClientPort(Globals.FilterHandle, &Globals.ClientPort);

	// Reset the user process field
	Globals.UserProcess = NULL;

	// Ensure that the ClientPort is NULL as well
	Globals.ClientPort = NULL;

	Globals.ConnectionCookie = NULL;

	if (nullptr != pPDQ)
	{
		;  // replace this with a relevant call
	}
}

/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterSendMessage to send a message to the minifilter driver through the client port.
* @param ServerPortCookie Pointer to information that uniquely identifies this client port.
* @param InputBuffer Pointer to a caller-allocated buffer containing the message to be sent to the minifilter driver.
* @param InputBufferLength Size, in bytes, of the buffer that InputBufferpoints
* @param OutputBuffer Pointer to a caller-allocated buffer that receives the reply (if any) from the minifilter driver.
* @param OutputBufferLength Size, in bytes, of the buffer that OutputBuffer points
* @param ReturnOutputBufferLength Pointer to a caller-allocated variable that receives the number of bytes returned in the buffer that OutputBuffer points to.
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
NTSTATUS FLTAPI
FltrCommsMgr::WVUCommandMessageNotify(
	PVOID ConnectionPortCookie,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnOutputBufferLength)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(ConnectionPortCookie);
	*ReturnOutputBufferLength = 0;

	WVU_DEBUG_PRINT(LOG_FLT_MGR, TRACE_LEVEL_ID, "Command Message Notification!\n");

	if (0 < InputBufferLength && NULL != InputBuffer)
	{
		Status = OnCommandMessage(InputBuffer, InputBufferLength);
		if (sizeof(Status) + sizeof(RESPONSE_MESSAGE) <= OutputBufferLength && NULL != OutputBuffer)
		{
			PRESPONSE_MESSAGE pReply = (PRESPONSE_MESSAGE)OutputBuffer;
			pReply->Size = sizeof(RESPONSE_MESSAGE);
			pReply->Response = NT_SUCCESS(Status) ? WVUSuccess : WVUFailure;
			memcpy(&pReply->Data[0], (PVOID)&Status, sizeof(Status));
			*ReturnOutputBufferLength = sizeof(Status) + sizeof(RESPONSE_MESSAGE);
		}
	}

	return Status;
}

/**
* @brief Changes the Protection State
* @param command specific command to change the state
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
FltrCommsMgr::OnProtectionStateChange(
	WVU_COMMAND command)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	switch (command)
	{
	case WVU_COMMAND::WVUDisableProtection:
		Globals.EnableProtection = FALSE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Windows VirtUE Protection Has Been Enabled!\n");
		break;
	case WVU_COMMAND::WVUEnableProtection:
		Globals.EnableProtection = TRUE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Windows VirtUE Protection Has Been Disabled!\n");
		break;
	default:
	case WVU_COMMAND::WVUDisableUnload:
	case WVU_COMMAND::WVUEnableUnload:
	case WVU_COMMAND::EnumerateProbes:
	case WVU_COMMAND::ConfigureProbe:
	case WVU_COMMAND::NOCOMMAND:
		Status = STATUS_INVALID_PARAMETER_1;
		break;
	}

	return Status;
}

/**
* @brief Changes the Unload State
* @param command specific command to change the state
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
FltrCommsMgr::OnUnloadStateChange(
	WVU_COMMAND command)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	switch (command)
	{
	case WVU_COMMAND::WVUDisableUnload:
#pragma warning(suppress: 28175)
		Globals.DriverObject->DriverUnload = NULL;
		Globals.AllowFilterUnload = FALSE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Windows VirtUE Driver Unload Has Been Disabled!\n");
		break;
	case WVU_COMMAND::WVUEnableUnload:
#pragma warning(suppress: 28175)
		Globals.DriverObject->DriverUnload = DriverUnload;
		Globals.AllowFilterUnload = TRUE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Windows VirtUE Driver Unload Has Been Enabled!\n");
		break;
	default:
	case WVU_COMMAND::NOCOMMAND:
	case WVU_COMMAND::WVUDisableProtection:
	case WVU_COMMAND::WVUEnableProtection:
		Status = STATUS_INVALID_PARAMETER_1;
		break;
	}

	return Status;
}

/**
* @brief Decodes Command Message Buffer and calls associated handlers
* @param InputBuffer Pointer to a caller-allocated buffer containing the message to be sent to the minifilter driver.
* @param InputBufferLength Size, in bytes, of the buffer that InputBufferpoints
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
FltrCommsMgr::OnCommandMessage(
	PVOID InputBuffer,
	ULONG InputBufferLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PCOMMAND_MESSAGE pCmdMsg = NULL;

	if (0 >= InputBufferLength && NULL == InputBuffer)
	{
		Status = STATUS_INVALID_PARAMETER;
		return Status;
	}

	pCmdMsg = (PCOMMAND_MESSAGE)InputBuffer;
	switch (pCmdMsg->Command)
	{
	case WVU_COMMAND::WVUEnableUnload:
	case WVU_COMMAND::WVUDisableUnload:
		FLT_ASSERTMSG("Unload State Changes must be 4 bytes!", sizeof(ULONG) == pCmdMsg->Size);
		Status = OnUnloadStateChange(pCmdMsg->Command);
		break;
	case WVU_COMMAND::WVUEnableProtection:
	case WVU_COMMAND::WVUDisableProtection:
		FLT_ASSERTMSG("Unload State Changes must be 4 bytes!", sizeof(ULONG) == pCmdMsg->Size);
		Status = OnProtectionStateChange(pCmdMsg->Command);
		break;
	default:
	case WVU_COMMAND::NOCOMMAND:
		Status = STATUS_INVALID_MESSAGE;
		break;
	}

	return Status;
}

/**
* @brief construct an instance of this object utilizing non paged pool memory
* @return pListEntry an item that was on the probe data queue for further processing
*/
_Use_decl_annotations_
PVOID
FltrCommsMgr::operator new(size_t size)
{
#pragma warning(suppress: 28160)  // cannot possibly allocate a must succeed - invalid
	PVOID pVoid = ExAllocatePoolWithTag(NonPagedPool, size, COMMON_POOL_TAG);
	return pVoid;
}

/**
* @brief destroys an instance of this object and releases its memory
* @param ptr pointer to the object instance to be destroyed
*/
_Use_decl_annotations_
VOID CDECL
FltrCommsMgr::operator delete(PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}