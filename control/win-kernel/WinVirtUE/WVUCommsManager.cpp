/**
* @file WVUCommsManager.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Filter Communications Manager Class definition
*/
#include "WVUCommsManager.h"
#include "Driver.h"
#define COMMON_POOL_TAG WVU_FLTCOMMSMGR_POOL_TAG

CONST PWSTR WVUCommsManager::CommandName = L"\\WVUCommand"; // Command/Response Comms Port
CONST PWSTR WVUCommsManager::PortName = L"\\WVUPort";		 // Real-Time streaming data as sent to the API

/**
* @brief destroys this Filter Comms Manager Instance
* @param OnConnect If not NULL, then call this method when a user space application disconnects
* @param OnDisconnect If not NULL, then call this method when a user space application connects
*/
WVUCommsManager::WVUCommsManager()
	: usPortName({ 0,0,nullptr }), usCommandName({ 0,0,nullptr }),  WVUPortObjAttr({ 0,0,0,0,0,0 }),
	WVUComandObjAttr({ 0,0,0,0,0,0 }), pWVUPortSecDsc(nullptr), pWVUCommandSecDsc(nullptr),
	InitStatus(STATUS_UNSUCCESSFUL)	
{	
	WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "About to register filter manager callbacks!\n");
	
	//  Register with FltMgr to tell it our callback routines
	InitStatus = FltRegisterFilter(Globals.DriverObject, &::FilterRegistration, &Globals.FilterHandle);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltRegisterFilter() FAIL=%08x\n", InitStatus);
		goto ErrorExit;
	}

	//
	//  Create communications ports.
	//
	RtlInitUnicodeString(&usPortName, WVUCommsManager::PortName);
	RtlInitUnicodeString(&usCommandName, WVUCommsManager::CommandName);

	//
	//  We secure the port so only ADMINs & SYSTEM can acecss it.
	//
	InitStatus = FltBuildDefaultSecurityDescriptor(&pWVUPortSecDsc, FLT_PORT_ALL_ACCESS);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltBuildDefaultSecurityDescriptor(pWVUPortSecDsc) FAIL=%08x\n", InitStatus);
		goto ErrorExit;
	}

	InitStatus = FltBuildDefaultSecurityDescriptor(&pWVUCommandSecDsc, FLT_PORT_ALL_ACCESS);
	if (FALSE == NT_SUCCESS(InitStatus))
	{
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltBuildDefaultSecurityDescriptor(pWVUCommandSecDsc) FAIL=%08x\n", InitStatus);
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
WVUCommsManager::~WVUCommsManager()
{
	FltUnregisterFilter(Globals.FilterHandle);

	Globals.IsProtectionEnabled = FALSE;
}

/**
* @brief Enables the Filter Manager and Port Communications
*/
_Use_decl_annotations_
BOOLEAN 
WVUCommsManager::Start()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (FALSE == NT_SUCCESS(this->InitStatus))
	{		
		Status = InitStatus;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "WVUCommsManager::WVUCommsManager() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	//
	// create the filter data communcations port
	//
	Status = FltCreateCommunicationPort(Globals.FilterHandle,
		&Globals.WVUProbeDataStreamPort,
		&WVUPortObjAttr,
		Globals.DriverObject,
		(PFLT_CONNECT_NOTIFY)&WVUCommsManager::WVUDataStreamPortConnect,
		(PFLT_DISCONNECT_NOTIFY)&WVUCommsManager::WVUDataStreamPortDisconnect,
		NULL,
		NUMBER_OF_PERMITTED_CONNECTIONS);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltCreateCommunicationPort() Failed! - FAIL=%08x\n", Status);
		goto ErrorExit;
	}

	//
	// create the filter command communcations port
	//
	Status = FltCreateCommunicationPort(Globals.FilterHandle,
		&Globals.WVUCommandPort,
		&WVUComandObjAttr,
		Globals.DriverObject,
		(PFLT_CONNECT_NOTIFY)&WVUCommsManager::WVUCommandConnect,
		(PFLT_DISCONNECT_NOTIFY)&WVUCommsManager::WVUCommandDisconnect,
		(PFLT_MESSAGE_NOTIFY)&WVUCommsManager::WVUCommandMessageNotify,
		NUMBER_OF_PERMITTED_CONNECTIONS);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltCreateCommunicationPort() Failed! - FAIL=%08x\n", Status);
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
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "FltStartFiltering() Failed! - FAIL=%08x\n", Status);
		FltUnregisterFilter(Globals.FilterHandle);
		goto ErrorExit;
	}

	Globals.IsProtectionEnabled = TRUE;

ErrorExit:

	return NT_SUCCESS(Status) ? TRUE : FALSE;
}
/**
* @brief Disables the Filter Manager and Port Communications 
*/
VOID 
WVUCommsManager::Stop()
{
	// close the server port
	FltCloseCommunicationPort(Globals.WVUProbeDataStreamPort);
	FltCloseCommunicationPort(Globals.WVUCommandPort);
	Globals.WVUProbeDataStreamPort = NULL;
	Globals.WVUCommandPort = NULL;
}

#pragma region Data Port Connection
/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterConnectCommunicationPort to send a connection request to the mini-filter driver
* @param WVUProbeDataStreamPort Opaque handle for the new client port that is established between the user-mode application and the kernel-mode mini-filter driver.
* @param ServerPortCookie Pointer to context information defined by the mini-filter driver
* @param ConnectionContext Context information pointer that the user-mode application passed in the lpContext parameter to FilterConnectCommunicationPort.
* @param SizeOfContext Size, in bytes, of the buffer that ConnectionContextpoints to
* @param ConnectionPortCookie Pointer to information that uniquely identifies this client port
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
NTSTATUS FLTAPI 
WVUCommsManager::WVUDataStreamPortConnect(
	PFLT_PORT WVUProbeDataStreamPort,
	PVOID ServerPortCookie,
	PVOID ConnectionContext,
	ULONG SizeOfContext,
	PVOID *ConnectionPortCookie)
{
	const NTSTATUS Status = STATUS_SUCCESS;

	Globals.DataStreamUserProcess = PsGetCurrentProcess();
	Globals.WVUProbeDataStreamPort = WVUProbeDataStreamPort;
	*ConnectionPortCookie = Globals.DriverObject;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "DataStream Port Connected by Process 0x%p Port 0x%p!\n",
		Globals.DataStreamUserProcess, Globals.WVUProbeDataStreamPort);

	WVUQueueManager::GetInstance().OnConnect();

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
WVUCommsManager::WVUDataStreamPortDisconnect(
	PVOID ConnectionPortCookie)
{
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Port Disconnected - Port 0x%p!\n", Globals.WVUProbeDataStreamPort);
	// close our handle to the connection 
	FltCloseClientPort(Globals.FilterHandle, &Globals.WVUProbeDataStreamPort);

	// Reset the user process field
	Globals.DataStreamUserProcess = NULL;

	// Ensure that the WVUProbeDataStreamPort is NULL as well
	Globals.WVUProbeDataStreamPort = NULL;

	WVUQueueManager::GetInstance().OnDisconnect();
}	
#pragma endregion

#pragma region Command Port Operation
/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterConnectCommunicationPort to send a connection request to the mini-filter driver
* @param WVUCommandPort Opaque handle for the new client port that is established between the user-mode application and the kernel-mode mini-filter driver.
* @param ServerPortCookie Pointer to context information defined by the mini-filter driver
* @param ConnectionContext Context information pointer that the user-mode application passed in the lpContext parameter to FilterConnectCommunicationPort.
* @param SizeOfContext Size, in bytes, of the buffer that ConnectionContextpoints to
* @param ConnectionPortCookie Pointer to information that uniquely identifies this client port
* @retval the driver entry's returned status
*/
_Use_decl_annotations_
NTSTATUS FLTAPI 
WVUCommsManager::WVUCommandConnect(
	PFLT_PORT WVUCommandPort,
	PVOID ServerPortCookie,
	PVOID ConnectionContext, 
	ULONG SizeOfContext, 
	PVOID * ConnectionPortCookie)
{
	const NTSTATUS Status = STATUS_SUCCESS;

	Globals.CommandUserProcess = PsGetCurrentProcess();
	Globals.WVUCommandPort = WVUCommandPort;
	*ConnectionPortCookie = Globals.DriverObject;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Command Port Connected by Process 0x%p Port 0x%p!\n",
		Globals.CommandUserProcess, Globals.WVUProbeDataStreamPort);

	Globals.IsCommandConnected = TRUE;

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
WVUCommsManager::WVUCommandDisconnect(
	PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Command Port Disconnected - Port 0x%p!\n", Globals.WVUProbeDataStreamPort);
	// close our handle to the connection 
	FltCloseClientPort(Globals.FilterHandle, &Globals.WVUCommandPort);

	// Reset the user process field
	Globals.CommandUserProcess = NULL;

	// Ensure that the WVUCommandPort is NULL as well
	Globals.WVUCommandPort = NULL;

	Globals.IsCommandConnected = FALSE;
}

/**
* @brief Changes the Protection State
* @param command specific command to change the state
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
WVUCommsManager::OnProtectionStateChange(
	WVU_COMMAND command)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;		

	switch (command)
	{
	case WVU_COMMAND::DisableProtection:		
		Globals.IsProtectionEnabled = FALSE; 
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Windows VirtUE Protection Has Been Disabled!\n");
		break;
	case WVU_COMMAND::EnableProtection:
		Globals.IsProtectionEnabled = TRUE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Windows VirtUE Protection Has Been Enabled!\n");
		break;
	default:
		Status = STATUS_INVALID_PARAMETER_1;
		FLT_ASSERTMSG("Invalid Protection Change State!", FALSE);
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "Invalid Protection Change State %d!\n", (signed)command);
		goto ExitError;
		break;
	}
	
	/** enable/disasble each probe */
	LIST_FOR_EACH(pProbeInfo, WVUQueueManager::GetInstance().GetProbeList(), WVUQueueManager::ProbeInfo)
	{
		/** if we are disabling, and the probe is enabled; then stop the probe */
		if (WVU_COMMAND::DisableProtection == command
			&& TRUE == pProbeInfo->Probe->IsEnabled())
		{
			pProbeInfo->Probe->Stop();
		}
		/** If we are enabling probes, the probe is disabled AND it's marked EnableAtStart, then start the probe */
		else if (WVU_COMMAND::EnableProtection == command
			&& FALSE == pProbeInfo->Probe->IsEnabled()
			&& (pProbeInfo->Probe->GetProbeAttribtes() 
				& AbstractVirtueProbe::ProbeAttributes::EnabledAtStart)
			== AbstractVirtueProbe::ProbeAttributes::EnabledAtStart)
		{
			pProbeInfo->Probe->Start();
		}
	}

ExitError:

	return Status;
}

/**
* @brief Changes the Unload State
* @param command specific command to change the state
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
WVUCommsManager::OnUnloadStateChange(
	WVU_COMMAND command)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	switch (command)
	{
	case WVU_COMMAND::DisableUnload:
#pragma warning(suppress: 28175)
		Globals.DriverObject->DriverUnload = NULL;
		Globals.AllowFilterUnload = FALSE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Windows VirtUE Driver Unload Has Been Disabled!\n");
		break;
	case WVU_COMMAND::EnableUnload:
#pragma warning(suppress: 28175)
		Globals.DriverObject->DriverUnload = DriverUnload;
		Globals.AllowFilterUnload = TRUE;
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, TRACE_LEVEL_ID, "Windows VirtUE Driver Unload Has Been Enabled!\n");
		break;
	default:
		Status = STATUS_INVALID_PARAMETER_1;
		FLT_ASSERTMSG("Invalid OnUnload State Change State!", FALSE);
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "Invalid OnUnload State Change State %d!\n", (signed)command);
		break;
	}

	return Status;
}

/**
* @brief returns to user space an enumeration of all active probes
* @retval Return a list active probes
*/
_Use_decl_annotations_
NTSTATUS
WVUCommsManager::OnEnumerateProbes(
	PCOMMAND_MESSAGE pCmdMsg,
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnOutputBufferLength)
{
	UNREFERENCED_PARAMETER(pCmdMsg);

	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	ULONG ProbeCount = AbstractVirtueProbe::GetProbeCount();
	ULONG ReponseBufferLength = sizeof(ProbeStatusHeader) + (ProbeCount * sizeof(ProbeStatus));
	
	if (ReponseBufferLength > OutputBufferLength)
	{
		Status = STATUS_BUFFER_TOO_SMALL;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, INFO_LEVEL_ID, "Buffer Length Provided is too small!\n");
		goto ErrorExit;
	}

	PBYTE pByte = new BYTE[ReponseBufferLength];
	if (NULL == pByte)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "Insufficient NonPaged Memory - Failed Allocation!\n");
		goto ErrorExit;
	}

	RtlSecureZeroMemory(pByte, ReponseBufferLength);

	/** write the header information */
	PProbeStatusHeader pProbeStatusHeader = (PProbeStatusHeader)pByte;
	pProbeStatusHeader->NumberOfEntries = ProbeCount;
	pProbeStatusHeader++;

	__try
	{
		PProbeStatus pProbeStatus = (PProbeStatus)pProbeStatusHeader;
		ULONG probe_cnt = 0L;

		LIST_FOR_EACH(pProbeInfo, WVUQueueManager::GetInstance().GetProbeList(), WVUQueueManager::ProbeInfo)
		{
			probe_cnt++;
			if (probe_cnt > ProbeCount)
			{
				Status = STATUS_NO_MORE_ENTRIES;
				WVU_DEBUG_PRINT(LOG_COMMS_MGR, ERROR_LEVEL_ID, "Probe Count Mismatch!\n");
				FLT_ASSERTMSG("Probe Count Mismatch!", FALSE);
				__leave;
			}
			pProbeStatus->ProbeNumber = pProbeInfo->Probe->GetProbeNumber();
			pProbeStatus->Attributes = pProbeInfo->Probe->GetProbeAttribtes();
			pProbeStatus->LastRunTime = pProbeInfo->Probe->GetLastProbeRunTime();
			pProbeStatus->OperationCount = pProbeInfo->Probe->GetOperationCount();
			pProbeStatus->RunInterval = pProbeInfo->Probe->GetRunInterval();
			ULONG length = pProbeInfo->Probe->GetProbeName().Length < MAXPROBENAMESZ
				? pProbeInfo->Probe->GetProbeName().Length
				: MAXPROBENAMESZ;
			RtlCopyMemory(pProbeStatus->ProbeName, pProbeInfo->Probe->GetProbeName().Buffer, length);
			pProbeStatus++;
		}
		RtlCopyMemory(OutputBuffer, pByte, ReponseBufferLength);
		*ReturnOutputBufferLength = ReponseBufferLength;
		Status = STATUS_SUCCESS;
	}
	__finally { delete[] pByte; pByte = NULL; }

ErrorExit:
	
	return Status;
}

/**
* @brief Changes the Unload State
* @param command specific command to change the state
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS
WVUCommsManager::OnConfigureProbe(
	PCOMMAND_MESSAGE pCmdMsg)
{
	UNREFERENCED_PARAMETER(pCmdMsg);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	return Status;
}

/**
* @brief Decodes Command Message Buffer and calls associated handlers
* @param Status The status as returned by the command
* @param OutputBuffer Pointer to a caller - allocated buffer that receives the reply(if any) from the minifilter driver.
* @param OutputBufferLength Size, in bytes, of the buffer that OutputBuffer points
* @param ReturnOutputBufferLength Pointer to a caller - allocated variable that receives the number of bytes returned in the buffer that OutputBuffer points to.
* @retval Returned Status
*/
_Use_decl_annotations_
VOID 
WVUCommsManager::CreateStandardResponse(
	NTSTATUS Status, 
	PVOID OutputBuffer, 
	ULONG OutputBufferLength, 
	PULONG ReturnOutputBufferLength)
{
	if (OutputBufferLength < sizeof(RESPONSE_MESSAGE) || NULL == OutputBuffer)
	{
		*ReturnOutputBufferLength = 0;
		goto ErrorExit;
	}

	PRESPONSE_MESSAGE pReply = (PRESPONSE_MESSAGE)OutputBuffer;
	pReply->DataSz = 0;
	pReply->Response = NT_SUCCESS(Status) ? Success : Failure;
	pReply->Status = Status;
	*ReturnOutputBufferLength = sizeof(RESPONSE_MESSAGE);

ErrorExit:

	return;
}

/**
* @brief Decodes Command Message Buffer and calls associated handlers
* @param InputBuffer Pointer to a caller-allocated buffer containing the message to be sent to the minifilter driver.
* @param InputBufferLength Size, in bytes, of the buffer that InputBufferpoints
* @param OutputBuffer Pointer to a caller-allocated buffer that receives the reply (if any) from the minifilter driver.
* @param OutputBufferLength Size, in bytes, of the buffer that OutputBuffer points
* @param ReturnOutputBufferLength Pointer to a caller-allocated variable that receives the number of bytes returned in the buffer that OutputBuffer points to.
* @retval Returned Status
*/
_Use_decl_annotations_
NTSTATUS 
WVUCommsManager::OnCommandMessage(
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnOutputBufferLength)
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
	case WVU_COMMAND::EnableUnload:
	case WVU_COMMAND::DisableUnload:
		Status = OnUnloadStateChange(pCmdMsg->Command);
		CreateStandardResponse(Status, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
		break;
	case WVU_COMMAND::EnableProtection:
	case WVU_COMMAND::DisableProtection:
		Status = OnProtectionStateChange(pCmdMsg->Command);
		CreateStandardResponse(Status, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
		break;
	case WVU_COMMAND::EnumerateProbes:
		Status = OnEnumerateProbes(pCmdMsg, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
		break;
	case WVU_COMMAND::ConfigureProbe:
		Status = OnConfigureProbe(pCmdMsg);
		CreateStandardResponse(Status, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
		break;
	case WVU_COMMAND::Echo:
		Status = STATUS_SUCCESS;
		CreateStandardResponse(Status, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);
		break;
	default:
		Status = STATUS_INVALID_MESSAGE;
		break;
	}

	return Status;
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
WVUCommsManager::WVUCommandMessageNotify(
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

	if (0L == InputBufferLength || NULL == InputBuffer)
	{
		WVU_DEBUG_PRINT(LOG_FLT_MGR, ERROR_LEVEL_ID, "Command Message Notification!\n");
		goto ErrorExit;
	}

	Status = OnCommandMessage(InputBuffer, 
		InputBufferLength, 
		OutputBuffer, 
		OutputBufferLength, 
		ReturnOutputBufferLength);

ErrorExit:

	return Status;
}


/**
* @brief construct an instance of this object utilizing non paged pool memory
* @return pListEntry an item that was on the probe data queue for further processing
*/
_Use_decl_annotations_
PVOID
WVUCommsManager::operator new(size_t size)
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
WVUCommsManager::operator delete(PVOID ptr)
{
	if (!ptr)
	{
		return;
	}
	ExFreePoolWithTag(ptr, COMMON_POOL_TAG);
}
#pragma endregion