/**
* @file WVUKernelUserComms.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Windows VirtUE User/Kernel Communications
*/
#pragma once

#if defined(NTDDI_VERSION)
    #include <ntddk.h>
#else
	#include <windows.h>
#endif

#define MAXMESSAGELEN 1024

//
//  Command type enumeration, please see COMMAND_MESSAGE below
//
typedef enum _WVU_COMMAND : ULONG
{
    NOCOMMAND				= 0,
    WVUEnableProtection		= 1,
    WVUDisableProtection    = 2,
    WVUEnableUnload         = 3,
    WVUDisableUnload        = 4,
	EnumerateProbes			= 5,
	ConfigureProbe			= 6,
	MAXCMDS
} WVU_COMMAND;

//
//  Response Type
//
typedef enum _WVU_RESPONSE : ULONG
{
    NORESPONSE	= 0,
    WVUSuccess	= 1,
    WVUFailure	= 2,
	MAXRESP		= 3
} WVU_RESPONSE;

//
// Defines the possible replies between the filter and user prograem
// Reply:  User <- Kernel
//
typedef struct _RESPONSE_MESSAGE
{
	LIST_ENTRY	ListEntry;	// The linked list entry
    ULONG Size;             // Command Message Packet Size
    WVU_RESPONSE Response;  // Command Response Type
	NTSTATUS Status;        // returned status from command
    UCHAR Data[1];			// Optional Response Packet Data
} RESPONSE_MESSAGE, *PRESPONSE_MESSAGE;

//
//  Defines the commands between the user program and the filter
//  Command: User -> Kernel
//
typedef struct _COMMAND_MESSAGE {
	LIST_ENTRY	ListEntry;	// The linked list entry
    ULONG Size;             // Command Message Packet Size
    WVU_COMMAND Command;    // The Command
    UCHAR Data[1];          // Optional Command Message Data
} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;


//
//  Connection type enumeration. It will be primarily used in connection context.
//
typedef enum _WVU_CONNECTION_TYPE {
    WVUConnectNotSecure = 1  // By default we have an unsecure connection between the user app and kernel filter
} WVU_CONNECTION_TYPE, *PWVU_CONNECTION_TYPE;

//
//  Connection context. It will be passed through FilterConnectCommunicationPort(...)
//
typedef struct _WVU_CONNECTION_CONTEXT {
    WVU_CONNECTION_TYPE   Type;  // The connection type
} WVU_CONNECTION_CONTEXT, *PWVU_CONNECTION_CONTEXT;

typedef struct _WVU_PROCESS_NOTIFICATION_INFO {


} WVU_PROCESS_NOTIFICATION_INFO, *PWVU_PROCESS_NOTIFICATION_INFO;


