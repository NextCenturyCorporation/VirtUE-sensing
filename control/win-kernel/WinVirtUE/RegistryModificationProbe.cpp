/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Configuration Manger Registry Modification callback handlers
*/

#include "RegistryModificationProbe.h"

/** this probes name */
static ANSI_STRING probe_name = RTL_CONSTANT_STRING("RegistryModification");

/** this probes current known altitude */
static const UNICODE_STRING WinVirtUEAltitude = RTL_CONSTANT_STRING(L"360000");

static PEX_CALLBACK_FUNCTION g_pCallbackFcns[MaxRegNtNotifyClass];

/**
* @brief construct an instance of this probe
*/
RegistryModificationProbe::RegistryModificationProbe() : AbstractVirtueProbe(probe_name)
{
	Attributes = (ProbeAttributes)(ProbeAttributes::RealTime | ProbeAttributes::EnabledAtStart);
	
	/** create default callbacks */
	for(int ndx = 0; ndx < MaxRegNtNotifyClass;ndx ++)
		g_pCallbackFcns[ndx] = DefaultExCallback;

	/** specialized callback handlers */
	g_pCallbackFcns[RegNtPreDeleteKey] = RegNtPreDeleteKeyCallback;
	g_pCallbackFcns[RegNtPreSetValueKey] = RegNtPreSetValueKeyCallback;
	g_pCallbackFcns[RegNtPreDeleteValueKey] = RegNtPreDeleteValueKeyCallback;
	g_pCallbackFcns[RegNtPreRenameKey] = RegNtPreRenameKeyCallback;
	g_pCallbackFcns[RegNtPreQueryValueKey] = RegNtPreQueryValueKeyCallback;
	g_pCallbackFcns[RegNtPreQueryMultipleValueKey] = RegNtPreQueryMultipleValueKeyCallback;
	g_pCallbackFcns[RegNtPreCreateKeyEx] = RegNtPreCreateKeyExCallback;
	g_pCallbackFcns[RegNtPreOpenKeyEx] = RegNtPreOpenKeyExCallback;
	g_pCallbackFcns[RegNtPreLoadKey] = RegNtPreLoadKeyCallback;
	g_pCallbackFcns[RegNtPreUnLoadKey] = RegNtPreUnLoadKeyCallback;
	g_pCallbackFcns[RegNtPreReplaceKey] = RegNtPreReplaceKeyCallback;
}

#pragma region Registry Pre-Operation Handlers
/**
* @brief This function is called when a RegNtPreReplaceKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreReplaceKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreUnLoadKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreUnLoadKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreLoadKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreLoadKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreOpenKeyExCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreOpenKeyExCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreCreateKeyExCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreCreateKeyExCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreQueryMultipleValueKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreQueryMultipleValueKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreQueryValueKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreQueryValueKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_QUERY_VALUE_KEY_INFORMATION prgvki = (PREG_QUERY_VALUE_KEY_INFORMATION)Argument2;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	FLT_ASSERTMSG("Incorrect Operation!", RegNtPreQueryValueKey == Class);
	if (NULL == probe || NULL == prgvki)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegQueryValueKeyInfo + prgvki->ValueName->MaximumLength;
	PRegQueryValueKeyInfo pInfo = (PRegQueryValueKeyInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}
	pInfo->ProbeDataHeader.probe_type = ProbeType::RegistryModification;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->Class = (REG_NOTIFY_CLASS)Class;
	pInfo->Object = prgvki->Object;
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->KeyValueInformationClass = prgvki->KeyValueInformationClass;
	pInfo->ValueNameLength = prgvki->ValueName->MaximumLength;
	RtlMoveMemory(&pInfo->ValueName[0], &prgvki->ValueName->Buffer[0], pInfo->ValueNameLength);
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
#pragma warning(suppress: 26407)
		delete[] pInfo;

		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, 
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"ProcessId=%p, EProcess=%p,KeyValueInformationClass=%d\n",
			prgvki->ValueName, pInfo->ProcessId, pInfo->EProcess, pInfo->KeyValueInformationClass);
	}

	if (NULL != probe)
	{
		probe->IncrementOperationCount();
		KeQuerySystemTimePrecise(&probe->GetLastProbeRunTime());
	}
	
	Status = STATUS_SUCCESS;  // yup, make sure we say it's all good!

ErrorExit:

	return Status;
}
/**
* @brief This function is called when a RegNtPreRenameKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreRenameKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreDeleteValueKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreDeleteValueKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreSetValueKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreSetValueKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called when a RegNtPreDeleteKeyCallback request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::RegNtPreDeleteKeyCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}
/**
* @brief This function is called by default when a registry modification request
* is made.
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
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::DefaultExCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(CallbackContext);
	UNREFERENCED_PARAMETER(Argument1);
	UNREFERENCED_PARAMETER(Argument2);
	return STATUS_SUCCESS;
}

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
_Use_decl_annotations_
NTSTATUS 
RegistryModificationProbe::RegistryModificationCB(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{	
	UNREFERENCED_PARAMETER(Argument2);
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ANSI_STRING notification = { 0,0,NULL };

	if (NULL == probe)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context Passed to Callback Function!\n");
		goto ErrorExit;
	}

	notification = RTL_CONSTANT_STRING(probe->GetNotifyClassString(Class));

	WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, INFO_LEVEL_ID,
		"Callback Driver Object Context %p, Registry Notification Class=%w, Arg2=%08x\n", 
		probe, notification, Argument2);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);
	__try
	{
		if (NULL == g_pCallbackFcns[Class])
		{
			Status = STATUS_SUCCESS;
			__leave;
		}
		Status = (*(g_pCallbackFcns[Class]))(CallbackContext, Argument1, Argument2);
	}
	__finally
	{
		// Drop a rundown reference 
		ExReleaseRundownProtection(&Globals.RunDownRef);
	}

ErrorExit:

	return Status;
}
#pragma endregion

#pragma Probe Virtual Function Overrides

/**
* @brief Start the RegistryModificationProbe by setting the notification callback
* @returns TRUE if successfully installed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN RegistryModificationProbe::Start()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (TRUE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already enabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	if ((Attributes & ProbeAttributes::EnabledAtStart) != ProbeAttributes::EnabledAtStart)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w not enabled at start - probe is registered but not active\n",
			&this->ProbeName);
		goto ErrorExit;
	}


	Cookie.QuadPart = (LONGLONG)Globals.DriverObject;
	Status = CmRegisterCallbackEx(RegistryModificationCB, &WinVirtUEAltitude, Globals.DriverObject, this, &Cookie, NULL);
	if (FALSE == NT_SUCCESS(Status))
	{



		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "CmRegisterCallbackEx(...) failed with Status=%08x\n", Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "CmRegisterCallbackEx():  Successfully Enabled Registry Modification Sensor\n");
	this->Enabled = TRUE;

ErrorExit:

	return NT_SUCCESS(Status);
}

/**
* @brief Stop the RegistryModificationProbe by unsetting the notification callback
* @returns TRUE if successfully removed the notification routine callback
*/
_Use_decl_annotations_
BOOLEAN RegistryModificationProbe::Stop()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (FALSE == this->Enabled)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, WARNING_LEVEL_ID,
			"Probe %w already disabled - continuing!\n", &this->ProbeName);
		goto ErrorExit;
	}

	Status = CmUnRegisterCallback(this->Cookie);
	if (FALSE == NT_SUCCESS(Status))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, "CmRegisterCallbackEx(...) failed to unregister cookie 0x%llx with Status=%08x\n", Cookie.QuadPart, Status);
		goto ErrorExit;
	}

	WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, TRACE_LEVEL_ID, "CmRegisterCallbackEx(): Successfully unregistered cookie 0x%llx for the Register Modification Probe\n", Cookie.QuadPart);
	this->Enabled = FALSE;
ErrorExit:

	return NT_SUCCESS(Status);
}

/**
* @brief returns the probes current state
* @returns TRUE if enabled else FALSE
*/
_Use_decl_annotations_
BOOLEAN RegistryModificationProbe::IsEnabled()
{
	return this->Enabled;
}

/**
* @brief Mitigate known issues that this probe discovers
* @note Mitigation is not being called as of June 2018
* @param argv array of arguments
* @param argc argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS RegistryModificationProbe::Mitigate(
	PCHAR argv[],
	UINT32 argc)
{
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(argc);
	return NTSTATUS();
}

/**
* @brief called by system thread if polled thread has expired
* @return NTSTATUS of this running of the probe
*/
_Use_decl_annotations_
NTSTATUS
RegistryModificationProbe::OnRun()
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = AbstractVirtueProbe::OnRun();
	return Status;
}

#pragma endregion

#pragma region registry modification utility functions

/**
* @brief Converts from NotifyClass to a string
* @return Returns a string of the name of NotifyClass
*/
_Use_decl_annotations_
CONST PCHAR
RegistryModificationProbe::GetNotifyClassString(
	USHORT NotifyClass)
{
	switch (NotifyClass) 
	{
	case RegNtPreDeleteKey:                 return ("RegNtPreDeleteKey");
	case RegNtPreSetValueKey:               return ("RegNtPreSetValueKey");
	case RegNtPreDeleteValueKey:            return ("RegNtPreDeleteValueKey");
	case RegNtPreSetInformationKey:         return ("RegNtPreSetInformationKey");
	case RegNtPreRenameKey:                 return ("RegNtPreRenameKey");
	case RegNtPreEnumerateKey:              return ("RegNtPreEnumerateKey");
	case RegNtPreEnumerateValueKey:         return ("RegNtPreEnumerateValueKey");
	case RegNtPreQueryKey:                  return ("RegNtPreQueryKey");
	case RegNtPreQueryValueKey:             return ("RegNtPreQueryValueKey");
	case RegNtPreQueryMultipleValueKey:     return ("RegNtPreQueryMultipleValueKey");
	case RegNtPreKeyHandleClose:            return ("RegNtPreKeyHandleClose");
	case RegNtPreCreateKeyEx:               return ("RegNtPreCreateKeyEx");
	case RegNtPreOpenKeyEx:                 return ("RegNtPreOpenKeyEx");
	case RegNtPreFlushKey:                  return ("RegNtPreFlushKey");
	case RegNtPreLoadKey:                   return ("RegNtPreLoadKey");
	case RegNtPreUnLoadKey:                 return ("RegNtPreUnLoadKey");
	case RegNtPreQueryKeySecurity:          return ("RegNtPreQueryKeySecurity");
	case RegNtPreSetKeySecurity:            return ("RegNtPreSetKeySecurity");
	case RegNtPreRestoreKey:                return ("RegNtPreRestoreKey");
	case RegNtPreSaveKey:                   return ("RegNtPreSaveKey");
	case RegNtPreReplaceKey:                return ("RegNtPreReplaceKey");

	case RegNtPostDeleteKey:                return ("RegNtPostDeleteKey");
	case RegNtPostSetValueKey:              return ("RegNtPostSetValueKey");
	case RegNtPostDeleteValueKey:           return ("RegNtPostDeleteValueKey");
	case RegNtPostSetInformationKey:        return ("RegNtPostSetInformationKey");
	case RegNtPostRenameKey:                return ("RegNtPostRenameKey");
	case RegNtPostEnumerateKey:             return ("RegNtPostEnumerateKey");
	case RegNtPostEnumerateValueKey:        return ("RegNtPostEnumerateValueKey");
	case RegNtPostQueryKey:                 return ("RegNtPostQueryKey");
	case RegNtPostQueryValueKey:            return ("RegNtPostQueryValueKey");
	case RegNtPostQueryMultipleValueKey:    return ("RegNtPostQueryMultipleValueKey");
	case RegNtPostKeyHandleClose:           return ("RegNtPostKeyHandleClose");
	case RegNtPostCreateKeyEx:              return ("RegNtPostCreateKeyEx");
	case RegNtPostOpenKeyEx:                return ("RegNtPostOpenKeyEx");
	case RegNtPostFlushKey:                 return ("RegNtPostFlushKey");
	case RegNtPostLoadKey:                  return ("RegNtPostLoadKey");
	case RegNtPostUnLoadKey:                return ("RegNtPostUnLoadKey");
	case RegNtPostQueryKeySecurity:         return ("RegNtPostQueryKeySecurity");
	case RegNtPostSetKeySecurity:           return ("RegNtPostSetKeySecurity");
	case RegNtPostRestoreKey:               return ("RegNtPostRestoreKey");
	case RegNtPostSaveKey:                  return ("RegNtPostSaveKey");
	case RegNtPostReplaceKey:               return ("RegNtPostReplaceKey");
	case RegNtCallbackObjectContextCleanup: return ("RegNtCallbackObjectContextCleanup");
	default:
		return ("Unsupported REG_NOTIFY_CLASS");
	}
}
#pragma endregion