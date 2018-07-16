/**
* @file WinVirtUe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup Module
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "WVUQueueManager.h"
#include "WVUCommsManager.h"
#include "AbstractVirtueProbe.h"
#include "ImageLoadProbe.h"
#include "ProcessListValidationProbe.h"
#include "ProcessCreateProbe.h"
#include "RegistryModificationProbe.h"

class WVUProbeManager
{
private:
	NTSTATUS Status;
	WVUProbeManager();
public:
	~WVUProbeManager();
	/**
	* @brief returns the one and only probe manager instance
	* @returns WVUProbeManager instance singleton
	*/
	static WVUProbeManager& GetInstance()
	{
		static WVUProbeManager instance;
		return instance;
	}
	/**
	* @brief copy constructor
	*/
	WVUProbeManager(const WVUProbeManager &t) = delete;

	/**
	* @brief assignment operator
	*/
	WVUProbeManager& operator=(const WVUProbeManager &rhs) = delete;

	/* construct a new instance of this class */
	_Must_inspect_impl_
		PVOID operator new(_In_ size_t size);
	/* destroy an instance of this class */
	VOID CDECL operator delete(_In_ PVOID ptr);	
};

