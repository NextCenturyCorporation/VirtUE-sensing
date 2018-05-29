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
	/* All probes are required to include an Enable function */
	virtual BOOLEAN Enable() = 0;
	/* All probes are required to include an Disable function */
	virtual BOOLEAN Disable() = 0;
	/* All probes are required to include an State function */
	virtual BOOLEAN State() = 0;
	/* All probes are required to include an Mitigate function */
	virtual BOOLEAN Mitigate() = 0;
	_Must_inspect_impl_
		PVOID operator new(_In_ size_t size);
	VOID CDECL operator delete(_In_ PVOID ptr);
};

