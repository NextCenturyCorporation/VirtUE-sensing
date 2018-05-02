#include "Common.h"
#include "PortMsgTrans.h"
#include "Driver.h"

/**
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterConnectCommunicationPort to send a connection request to the mini-filter driver
* @param ClientPort Opaque handle for the new client port that is established between the user-mode application and the kernel-mode mini-filter driver.
* @param ServerPortCookie Pointer to context information defined by the mini-filter driver
* @param ConnectionContext Context information pointer that the user-mode application passed in the lpContext parameter to FilterConnectCommunicationPort.
* @param SizeOfContext Size, in bytes, of the buffer that ConnectionContextpoints to
* @param ConnectionPortCookie Pointer to information that uniquely identifies this client port
* @retval the driver entry's returned status
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS FLTAPI WVUPortConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ PVOID ServerPortCookie,
	_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID *ConnectionPortCookie)
{
	NTSTATUS Status = STATUS_SUCCESS;
	FLT_ASSERTMSG("ClientPort Must Be NULL!!", NULL == Globals.ClientPort);
	FLT_ASSERTMSG("UserProcess Must Be NULL!!", NULL == Globals.UserProcess);

	Globals.UserProcess = PsGetCurrentProcess();
	Globals.ClientPort = ClientPort;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	*ConnectionPortCookie = (PVOID)Globals.DriverObject;

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Port Connected by Process 0x%p Port 0x%p!\n",
		Globals.UserProcess, Globals.ClientPort);

	return Status;
}

/**
* @brief The driver unload callback.
* @note callback routine to be called whenever the user-mode handle count for the client port reaches zero or when the minifilter driver is about to be unloaded
* @param ConnectionCookie  Pointer to information that uniquely identifies this client port
* represent this driver
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID FLTAPI WVUPortDisconnect(
	_In_opt_ PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	WVU_DEBUG_PRINT(LOG_MAIN, TRACE_LEVEL_ID, "Port Disconnected - Port 0x%p!\n", Globals.ClientPort);
	// close our handle to the connection 
	FltCloseClientPort(Globals.FilterHandle, &Globals.ClientPort);

	// Reset the user process field
	Globals.UserProcess = NULL;

	// Ensure that the ClientPort is NULL as well
	Globals.ClientPort = NULL;

	Globals.ConnectionCookie = NULL;
}

/**
* @brief Changes the Protection State
* @param command specific command to change the state
* @retval Returned Status
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnProtectionStateChange(
	_In_ WVU_COMMAND command)
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
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnUnloadStateChange(
	_In_ WVU_COMMAND command)
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
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS OnCommandMessage(
	_In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength)
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
* @brief Filter Manager calls this routine whenever a user-mode application calls FilterSendMessage to send a message to the minifilter driver through the client port.
* @param PortCookie Pointer to information that uniquely identifies this client port.
* @param InputBuffer Pointer to a caller-allocated buffer containing the message to be sent to the minifilter driver.
* @param InputBufferLength Size, in bytes, of the buffer that InputBufferpoints
* @param OutputBuffer Pointer to a caller-allocated buffer that receives the reply (if any) from the minifilter driver.
* @param OutputBufferLength Size, in bytes, of the buffer that OutputBuffer points
* @param ReturnOutputBufferLength Pointer to a caller-allocated variable that receives the number of bytes returned in the buffer that OutputBuffer points to.
* @retval the driver entry's returned status
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS FLTAPI WVUMessageNotify(
	_In_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ PULONG ReturnOutputBufferLength)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(PortCookie);
	*ReturnOutputBufferLength = 0;

	if (0 < InputBufferLength && NULL != InputBuffer)
	{
		Status = OnCommandMessage(InputBuffer, InputBufferLength);
		if (sizeof(Status) + sizeof(WVU_REPLY) <= OutputBufferLength && NULL != OutputBuffer)
		{
			PWVU_REPLY pReply = (PWVU_REPLY)OutputBuffer;
			pReply->Size = sizeof(WVU_RESPONSE) + sizeof(Status);
			pReply->Response = NT_SUCCESS(Status) ? WVUSuccess : WVUFailure;
			memcpy((PUCHAR)pReply->Data, (PUCHAR)&Status, sizeof(Status));
			*ReturnOutputBufferLength = sizeof(Status) + sizeof(WVU_REPLY);
		}
	}

	return Status;
}