/**
* @file AbstractVirtueProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Abstract Base Class for Windows VirtUE Probes
*/
#pragma once
#include "common.h"
#include "externs.h"
class AbstractVirtueProbe
{
protected:
	BOOLEAN Enabled;
public:
	AbstractVirtueProbe();
	virtual ~AbstractVirtueProbe();
	/* Enable the probe - required functionality */
	virtual BOOLEAN Enable() = 0;
	/* Disable the probe - required functionality */
	virtual BOOLEAN Disable() = 0;
	/* Determine probe state where TRUE is enabled else FALSE is disabled */
	_Must_inspect_result_
	virtual BOOLEAN State() = 0;
	/* Mitigate probed states - currently not utilized */
	virtual BOOLEAN Mitigate() = 0;
	/* construct a new instance of this probe class */
	_Must_inspect_impl_
		PVOID operator new(_In_ size_t size);
	/* destroy an instance of this probe class */
	VOID CDECL operator delete(_In_ PVOID ptr);
};

