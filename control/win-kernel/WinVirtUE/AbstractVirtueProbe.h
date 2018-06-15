/**
* @file AbstractVirtueProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Abstract Base Class for Windows VirtUE Probes
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "FltCommandQueue.h"


#undef _HAS_EXCEPTIONS
#include <new.h>
#include <cstddef>

class AbstractVirtueProbe
{
protected:
	BOOLEAN Enabled;
	UNICODE_STRING ProbeName;
public:
	AbstractVirtueProbe() : Enabled(FALSE), ProbeName(RTL_CONSTANT_STRING(L"")) {}
	virtual ~AbstractVirtueProbe() = default;
	/* Enable the probe - required functionality */
	_Success_(TRUE == return)
	virtual BOOLEAN Enable() = 0;
	/* Disable the probe - required functionality */
	_Success_(TRUE == return)
	virtual BOOLEAN Disable() = 0;
	/* Determine probe state where TRUE is enabled else FALSE is disabled */
	_Must_inspect_result_
	virtual BOOLEAN State() = 0;
	/* Mitigate probed states - currently not utilized */
	_Must_inspect_result_ 
	_Success_(TRUE==NT_SUCCESS(return))
	virtual NTSTATUS Mitigate(
		_In_count_(argc) PCHAR argv[],
		_In_ UINT32 argc) = 0;
	/* construct a new instance of this probe class */
	_Must_inspect_impl_
	PVOID operator new(_In_ size_t size);
	/* destroy an instance of this probe class */
	VOID CDECL operator delete(_In_ PVOID ptr);
	/* return this probes name */
	UNICODE_STRING& GetProbeName() { return this->ProbeName; }
	_Must_inspect_result_
	virtual const AbstractVirtueProbe& operator+=(_In_ const FltCommandQueue& rhs);
	_Must_inspect_result_
	virtual const AbstractVirtueProbe& operator-=(_In_ const FltCommandQueue& rhs);
};