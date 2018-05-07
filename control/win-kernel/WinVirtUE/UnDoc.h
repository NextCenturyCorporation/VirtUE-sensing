/**
* @file UnDoc.h
* @version 0.1.0.0
* @copyright (2018) Two Six Labs
* @brief Undefined/Unsupported/Undocumented Windows System Calls
*/
#pragma once
#ifdef __cplusplus 
extern "C" {
#endif
#include <fltKernel.h>
	NTSYSAPI
		NTSTATUS NTAPI
		ObReferenceObjectByName(
			PUNICODE_STRING ObjectPath,
			ULONG Attributes,
			PACCESS_STATE PassedAccessState,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE ObjectType,
			KPROCESSOR_MODE AccessMode,
			PVOID ParseContext,
			OUT PVOID *ObjectPtr);

#ifdef __cplusplus 
}
#endif