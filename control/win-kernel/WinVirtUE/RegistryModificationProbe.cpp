/**
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Registry Modification Probe 
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
	g_pCallbackFcns[RegNtPreCreateKeyEx] = RegNtPreCreateKeyExCallback;
	g_pCallbackFcns[RegNtPostCreateKeyEx] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreOpenKeyEx] = RegNtPreOpenKeyExCallback;
	g_pCallbackFcns[RegNtPostOpenKeyEx] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreSetValueKey] = RegNtPreSetValueKeyCallback;
	g_pCallbackFcns[RegNtPostSetValueKey] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreQueryValueKey] = RegNtPreQueryValueKeyCallback;
	g_pCallbackFcns[RegNtPostQueryValueKey] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreDeleteValueKey] = RegNtPreDeleteValueKeyCallback;
	g_pCallbackFcns[RegNtPostDeleteValueKey] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreRenameKey] = RegNtPreRenameKeyCallback;
	g_pCallbackFcns[RegNtPostRenameKey] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreLoadKey] = RegNtPreLoadKeyCallback;
	g_pCallbackFcns[RegNtPostLoadKey] = RegNtPostOperationCallback;

	g_pCallbackFcns[RegNtPreReplaceKey] = RegNtReplaceKeyCallback;
	g_pCallbackFcns[RegNtPostReplaceKey] = RegNtReplaceKeyCallback;

	g_pCallbackFcns[RegNtPreQueryMultipleValueKey] = RegNtPreQueryMultipleValueKeyCallback;
	g_pCallbackFcns[RegNtPostQueryMultipleValueKey] = RegNtPostOperationCallback;
}

#pragma region Registry Pre-Operation Handlers
/**
* @brief This function is called when a RegNtReplaceKeyCallback request
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
RegistryModificationProbe::RegNtReplaceKeyCallback(
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
	return RegNtPreCreateKeyExCallback(CallbackContext, Argument1, Argument2);
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
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_CREATE_KEY_INFORMATION_V1 prcki = (PREG_CREATE_KEY_INFORMATION_V1)Argument2;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	FLT_ASSERTMSG("Incorrect Operation!", RegNtPreCreateKeyEx == Class || RegNtPreOpenKeyEx == Class);
	if (NULL == probe || NULL == prcki)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegCreateKeyInfo + prcki->CompleteName->Length;
	PRegCreateKeyInfo pInfo = (PRegCreateKeyInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}
	pInfo->ProbeDataHeader.probe_type = Class == RegNtPreCreateKeyEx 
		? ProbeType::RegCreateKeyInformation 
		: ProbeType::RegOpenKeyInformation;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->RootObject = prcki->RootObject;
	pInfo->Options = prcki->Options;
	pInfo->SecurityDescriptor = prcki->SecurityDescriptor;
	pInfo->SecurityQualityOfService = prcki->SecurityQualityOfService;
	pInfo->DesiredAccess = prcki->DesiredAccess;
	pInfo->GrantedAccess = prcki->GrantedAccess;
	pInfo->Version = prcki->Version;
	pInfo->Wow64Flags = prcki->Wow64Flags;
	pInfo->Attributes = prcki->Attributes;
	pInfo->CheckAccessMode = prcki->CheckAccessMode;
	pInfo->CompleteNameSz = prcki->CompleteName->Length;
	RtlMoveMemory(&pInfo->CompleteName[0], prcki->CompleteName->Buffer, pInfo->CompleteNameSz);
	
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"***** Registry Modification Probe Enqueue Operation Failed: CompleteName=%wZ,"
			"Class=%wZ,RemainingName=%wZ,ProcessId=%p, EProcess=%p\n",
			prcki->CompleteName, prcki->RemainingName, pInfo->ProcessId, pInfo->EProcess);
		delete[] pInfo;
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
	pInfo->ProbeDataHeader.probe_type = ProbeType::RegQueryValueKeyInformation;
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
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID, 
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"ProcessId=%p, EProcess=%p,KeyValueInformationClass=%d\n",
			prgvki->ValueName, pInfo->ProcessId, pInfo->EProcess, pInfo->KeyValueInformationClass);
		delete[] pInfo;
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
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_RENAME_KEY_INFORMATION prrki = (PREG_RENAME_KEY_INFORMATION)Argument2;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	FLT_ASSERTMSG("Incorrect Operation!", RegNtPreRenameKey == Class);
	if (NULL == probe || NULL == prrki)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegRenameKeyInfo + prrki->NewName->MaximumLength;
	PRegRenameKeyInfo pInfo = (PRegRenameKeyInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}
	pInfo->ProbeDataHeader.probe_type = ProbeType::RegRenameKeyInformation;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->Object = prrki->Object;
	pInfo->NewNameLength = prrki->NewName->MaximumLength;
	RtlMoveMemory(&pInfo->NewName[0], &prrki->NewName->Buffer[0], pInfo->NewNameLength);
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"ProcessId=%p, EProcess=%p\n",
			prrki->NewName, pInfo->ProcessId, pInfo->EProcess);
		delete[] pInfo;
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
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_DELETE_VALUE_KEY_INFORMATION prdvki = (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	FLT_ASSERTMSG("Incorrect Operation!", RegNtDeleteValueKey == Class);
	if (NULL == probe || NULL == prdvki)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegDeleteValueKeyInfo + prdvki->ValueName->MaximumLength;
	PRegDeleteValueKeyInfo pInfo = (PRegDeleteValueKeyInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}
	pInfo->ProbeDataHeader.probe_type = ProbeType::RegDeleteValueKeyInformation;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->Object = prdvki->Object;
	pInfo->ValueNameLength = prdvki->ValueName->MaximumLength;
	RtlMoveMemory(&pInfo->ValueName[0], &prdvki->ValueName->Buffer[0], pInfo->ValueNameLength);
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"ProcessId=%p, EProcess=%p\n",
			prdvki->ValueName, pInfo->ProcessId, pInfo->EProcess);
		delete[] pInfo;
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
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_SET_VALUE_KEY_INFORMATION psvki = (PREG_SET_VALUE_KEY_INFORMATION)Argument2;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	FLT_ASSERTMSG("Incorrect Operation!", RegNtPreSetValueKey == Class);
	if (NULL == probe || NULL == psvki)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegSetValueKeyInfo + psvki->ValueName->MaximumLength;
	PRegSetValueKeyInfo pInfo = (PRegSetValueKeyInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}
	pInfo->ProbeDataHeader.probe_type = ProbeType::RegSetValueKeyInformation;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->Object = psvki->Object;
	pInfo->Type = (RegObjectType)psvki->Type;
	pInfo->ValueNameLength = psvki->ValueName->MaximumLength;
	RtlMoveMemory(&pInfo->ValueName[0], &psvki->ValueName->Buffer[0], pInfo->ValueNameLength);
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"ProcessId=%p, EProcess=%p,Type=%ld\n",
			psvki->ValueName, pInfo->ProcessId, pInfo->EProcess, pInfo->Type);
		delete[] pInfo;
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
RegistryModificationProbe::RegNtPostOperationCallback(
	PVOID CallbackContext,
	PVOID Argument1,
	PVOID Argument2)
{
	UNREFERENCED_PARAMETER(Argument1);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
	PREG_POST_OPERATION_INFORMATION prpoi = (PREG_POST_OPERATION_INFORMATION)Argument2;

	if (NULL == probe || NULL == prpoi)
	{
		Status = STATUS_SUCCESS;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Invalid Context or arguments Passed to Callback Function!\n");
		goto ErrorExit;
	}
	USHORT bufsz = sizeof RegPostOperationInfo;
	PRegPostOperationInfo pInfo = (PRegPostOperationInfo)new BYTE[bufsz];
	if (NULL == pInfo)
	{
		Status = STATUS_NO_MEMORY;
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, ERROR_LEVEL_ID, "Unable to allocate non-paged memory!\n");
		goto ErrorExit;
	}

	PCUNICODE_STRING ObjectName = nullptr;
	ULONG_PTR ObjectId = 0L;
	ULONG Flags = 0L;
	__try
	{
		Status = CmCallbackGetKeyObjectIDEx((PLARGE_INTEGER)&probe->GetCookie(), prpoi->Object, &ObjectId, &ObjectName, Flags);
		if (FALSE == NT_SUCCESS(Status))
		{
			WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, TRACE_LEVEL_ID, "CmCallbackGetKeyObjectIDEx failed - Status=0x%08x\n",
				Status);
			goto Abend;
		}
		WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, TRACE_LEVEL_ID, "ObjectName=%wZ, ObjectId=%p\n", *ObjectName, ObjectId);
	}
	__finally
	{
		if (AbnormalTermination() == FALSE)
		{
			CmCallbackReleaseKeyObjectIDEx(ObjectName);
		}
	}

Abend:

	pInfo->ProbeDataHeader.probe_type = ProbeType::RegPostOperationInformation;
	pInfo->ProbeDataHeader.data_sz = bufsz;
	pInfo->ProbeDataHeader.probe_id = probe->GetProbeId();
	KeQuerySystemTimePrecise(&pInfo->ProbeDataHeader.current_gmt);
	pInfo->ProcessId = PsGetCurrentProcessId();
	pInfo->EProcess = PsGetCurrentProcess();
	pInfo->Object = prpoi->Object;
	pInfo->PreInformation = prpoi->PreInformation;
	pInfo->ReturnStatus = prpoi->ReturnStatus;
	pInfo->Status = pInfo->Status;
	if (FALSE == WVUQueueManager::GetInstance().Enqueue(&pInfo->ProbeDataHeader.ListEntry))
	{
		WVU_DEBUG_PRINT(LOG_NOTIFY_MODULE, ERROR_LEVEL_ID,
			"***** Registry Modification Probe Enqueue Operation Failed: ValueName=%wZ,"
			"Status=%08x, ReturnStatus=%08x,Object=%p,PreInformation=%p,ProcessId=%p, "
			"EProcess=%p\n", pInfo->Status, pInfo->ReturnStatus, pInfo->Object, 
			pInfo->PreInformation, pInfo->ProcessId, pInfo->EProcess);
		delete[] pInfo;
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
	RegistryModificationProbe *probe = (RegistryModificationProbe*)CallbackContext;
#pragma warning(push)
#pragma warning(disable:4302) // ignore type cast truncation error 
	USHORT Class = (USHORT)Argument1;
#pragma warning(pop)
	NTSTATUS Status = STATUS_SUCCESS;
	FLT_ASSERTMSG("Invalid Context Passed to Callback Function!", NULL != probe);
	const ANSI_STRING& notification = probe->GetNotifyClassString(Class);

	WVU_DEBUG_PRINT(LOG_NOTIFY_REGISTRY, INFO_LEVEL_ID,
		"Callback Driver Object Context %p, Registry Notification Class=%w, Arg2=%08x\n", 
		probe, notification, Argument2);

	// Take a rundown reference 
	(VOID)ExAcquireRundownProtection(&Globals.RunDownRef);
	__try
	{
		if (NULL != g_pCallbackFcns[Class])
		{
			Status = (*(g_pCallbackFcns[Class]))(CallbackContext, Argument1, Argument2);			
		}		
	}
	__finally
	{
		// Drop a rundown reference 
		ExReleaseRundownProtection(&Globals.RunDownRef);
	}

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
* @param ArgV array of arguments
* @param ArgC argument count
* @returns Status returns operational status
*/
_Use_decl_annotations_
NTSTATUS RegistryModificationProbe::Mitigate(
	UINT32 ArgC,
	ANSI_STRING ArgV[])
{
	UNREFERENCED_PARAMETER(ArgV);
	UNREFERENCED_PARAMETER(ArgC);
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
CONST ANSI_STRING&
RegistryModificationProbe::GetNotifyClassString(
	USHORT NotifyClass)
{
	static ANSI_STRING str = RTL_CONSTANT_STRING("Unsupported REG_NOTIFY_CLASS");

	switch (NotifyClass) 
	{
	case RegNtPreDeleteKey:                 str = RTL_CONSTANT_STRING("RegNtPreDeleteKey"); break;
	case RegNtPreSetValueKey:               str = RTL_CONSTANT_STRING("RegNtPreSetValueKey"); break;
	case RegNtPreDeleteValueKey:            str = RTL_CONSTANT_STRING("RegNtPreDeleteValueKey"); break;
	case RegNtPreSetInformationKey:         str = RTL_CONSTANT_STRING("RegNtPreSetInformationKey"); break;
	case RegNtPreRenameKey:                 str = RTL_CONSTANT_STRING("RegNtPreRenameKey"); break;
	case RegNtPreEnumerateKey:              str = RTL_CONSTANT_STRING("RegNtPreEnumerateKey"); break;
	case RegNtPreEnumerateValueKey:         str = RTL_CONSTANT_STRING("RegNtPreEnumerateValueKey"); break;
	case RegNtPreQueryKey:                  str = RTL_CONSTANT_STRING("RegNtPreQueryKey"); break;
	case RegNtPreQueryValueKey:             str = RTL_CONSTANT_STRING("RegNtPreQueryValueKey"); break;
	case RegNtPreQueryMultipleValueKey:     str = RTL_CONSTANT_STRING("RegNtPreQueryMultipleValueKey"); break;
	case RegNtPreKeyHandleClose:            str = RTL_CONSTANT_STRING("RegNtPreKeyHandleClose"); break;
	case RegNtPreCreateKeyEx:               str = RTL_CONSTANT_STRING("RegNtPreCreateKeyEx"); break;
	case RegNtPreOpenKeyEx:                 str = RTL_CONSTANT_STRING("RegNtPreOpenKeyEx"); break;
	case RegNtPreFlushKey:                  str = RTL_CONSTANT_STRING("RegNtPreFlushKey"); break;
	case RegNtPreLoadKey:                   str = RTL_CONSTANT_STRING("RegNtPreLoadKey"); break;
	case RegNtPreUnLoadKey:                 str = RTL_CONSTANT_STRING("RegNtPreUnLoadKey"); break;
	case RegNtPreQueryKeySecurity:          str = RTL_CONSTANT_STRING("RegNtPreQueryKeySecurity"); break;
	case RegNtPreSetKeySecurity:            str = RTL_CONSTANT_STRING("RegNtPreSetKeySecurity"); break;
	case RegNtPreRestoreKey:                str = RTL_CONSTANT_STRING("RegNtPreRestoreKey"); break;
	case RegNtPreSaveKey:                   str = RTL_CONSTANT_STRING("RegNtPreSaveKey"); break;
	case RegNtPreReplaceKey:                str = RTL_CONSTANT_STRING("RegNtPreReplaceKey"); break;

	case RegNtPostDeleteKey:                str = RTL_CONSTANT_STRING("RegNtPostDeleteKey"); break;
	case RegNtPostSetValueKey:              str = RTL_CONSTANT_STRING("RegNtPostSetValueKey"); break;
	case RegNtPostDeleteValueKey:           str = RTL_CONSTANT_STRING("RegNtPostDeleteValueKey"); break;
	case RegNtPostSetInformationKey:        str = RTL_CONSTANT_STRING("RegNtPostSetInformationKey"); break;
	case RegNtPostRenameKey:                str = RTL_CONSTANT_STRING("RegNtPostRenameKey"); break;
	case RegNtPostEnumerateKey:             str = RTL_CONSTANT_STRING("RegNtPostEnumerateKey"); break;
	case RegNtPostEnumerateValueKey:        str = RTL_CONSTANT_STRING("RegNtPostEnumerateValueKey"); break;
	case RegNtPostQueryKey:                 str = RTL_CONSTANT_STRING("RegNtPostQueryKey"); break;
	case RegNtPostQueryValueKey:            str = RTL_CONSTANT_STRING("RegNtPostQueryValueKey"); break;
	case RegNtPostQueryMultipleValueKey:    str = RTL_CONSTANT_STRING("RegNtPostQueryMultipleValueKey"); break;
	case RegNtPostKeyHandleClose:           str = RTL_CONSTANT_STRING("RegNtPostKeyHandleClose"); break;
	case RegNtPostCreateKeyEx:              str = RTL_CONSTANT_STRING("RegNtPostCreateKeyEx"); break;
	case RegNtPostOpenKeyEx:                str = RTL_CONSTANT_STRING("RegNtPostOpenKeyEx"); break;
	case RegNtPostFlushKey:                 str = RTL_CONSTANT_STRING("RegNtPostFlushKey"); break;
	case RegNtPostLoadKey:                  str = RTL_CONSTANT_STRING("RegNtPostLoadKey"); break;
	case RegNtPostUnLoadKey:                str = RTL_CONSTANT_STRING("RegNtPostUnLoadKey"); break;
	case RegNtPostQueryKeySecurity:         str = RTL_CONSTANT_STRING("RegNtPostQueryKeySecurity"); break;
	case RegNtPostSetKeySecurity:           str = RTL_CONSTANT_STRING("RegNtPostSetKeySecurity"); break;
	case RegNtPostRestoreKey:               str = RTL_CONSTANT_STRING("RegNtPostRestoreKey"); break;
	case RegNtPostSaveKey:                  str = RTL_CONSTANT_STRING("RegNtPostSaveKey"); break;
	case RegNtPostReplaceKey:               str = RTL_CONSTANT_STRING("RegNtPostReplaceKey"); break;
	case RegNtCallbackObjectContextCleanup: str = RTL_CONSTANT_STRING("RegNtCallbackObjectContextCleanup"); break;
	default: break;		
	}
	return str;
}
#pragma endregion