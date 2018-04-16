/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Configuration Manger Registry Modification callback handlers
*/
#include "RegistryModificationCB.h"
#define COMMON_POOL_TAG WVU_REGISTRY_MODIFICATION_POOL_TAG

/**
* @brief This function is called when a registry modifications occurs.  
* @note https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/registering-for-notifications
* @param CallbackContext The value that the driver passed as the Context parameter to CmRegisterCallback 
* or CmRegisterCallbackEx when it registered this RegistryCallback routine.
* @param Argument1 A REG_NOTIFY_CLASS-typed value that identifies the type 
* of registry operation that is being performed and whether the RegistryCallback 
* routine is being called before or after the registry operation is performed.
* @param Argument2 A pointer to a structure that contains information that is 
* specific to the type of registry operation. The structure type depends on the 
* REG_NOTIFY_CLASS-typed value for Argument1, as shown in the following table. 
* For information about which REG_NOTIFY_CLASS-typed values are available for which 
* operating system versions, see REG_NOTIFY_CLASS.
* @retval the driver entry's returned status
*/
NTSTATUS RegistryModificationCB(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	PDRIVER_OBJECT pDrvObject = (PDRIVER_OBJECT)CallbackContext;

	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);

	WVU_DEBUG_PRINT(LOG_NOTIFY_PROCS, TRACE_LEVEL_ID,
		"Callback Driver Object Context %p, Registry Notification Class=%p, Arg2=%p\n", pDrvObject, Argument1, Argument2);
		
	return STATUS_SUCCESS;
}