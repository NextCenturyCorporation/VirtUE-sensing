#pragma once
/**
* @file WinVirtUe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup Module
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "ProbeDataQueue.h"
#include "FltrCommsMgr.h"
#include "AbstractVirtueProbe.h"
#include "ImageLoadProbe.h"
#include "ProcessListValidationProbe.h"
#include "ProcessCreateProbe.h"


class WinVirtUEManager
{
private:
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
public:
	WinVirtUEManager();
	~WinVirtUEManager();
	/* construct a new instance of this class */
	_Must_inspect_impl_
		PVOID operator new(_In_ size_t size);
	/* destroy an instance of this class */
	VOID CDECL operator delete(_In_ PVOID ptr);	
};

