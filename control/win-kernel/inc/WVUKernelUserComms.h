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

// port name used to communicate between user and kernel
const PWSTR WVUPortName = L"\\WVUPort";

#define MAXMESSAGELEN 1024

//
//  Command type enumeration, please see COMMAND_MESSAGE below
//
typedef enum _WVU_COMMAND : ULONG
{
    NOCOMMAND                            = 0,
    WVUEnableProtection                  = 1,
    WVUDisableProtection                 = 2,
    WVUEnableUnload                      = 3,
    WVUDisableUnload                     = 4,
    WVUMaxCommand                        = WVUDisableUnload
} WVU_COMMAND;

//
//  Response Type
//
typedef enum _WVU_RESPONSE : ULONG
{
    NORESPONSE                           = 0,
    WVUSuccess                           = 1,
    WVUFailure                           = 2,
    WVUCommandMessageResponse            = 3,
    WVUMaxResponse                       = WVUCommandMessageResponse
} WVU_RESPONSE;

//
// Defines the possible replies between the filter and user prograem
// Reply:  User <- Kernel
//
typedef struct _WVU_REPLY
{
    ULONG Size;             // Command Message Packet Size (not including the Size variable)
    WVU_RESPONSE Response;   // Command Response Type
#pragma warning( push )
#pragma warning(disable : 4200)
    UCHAR Data[0];           // Optional Response Packet Data
#pragma warning( pop )
} WVU_REPLY, *PWVU_REPLY;

//
//  Defines the commands between the user program and the filter
//  Command: User -> Kernel
//
typedef struct _COMMAND_MESSAGE {
    ULONG Size;             // Command Message Packet Size (not including the Size variable)
    WVU_COMMAND Command;     // The Command
#pragma warning( push )
#pragma warning(disable : 4200)
    UCHAR Data[0];           // Optional Command Message Data
#pragma warning( pop )
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


