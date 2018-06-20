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
private:
	AbstractVirtueProbe() = default;
public:
	_Enum_is_bitflag_
		typedef enum _ProbeAttributes : USHORT
	{
		NoAttributes	= 0,		// No attributes
		RealTime		= 1 << 1,	// Real Time Probe that emits events as it happens
		Temporal		= 1 << 2,	// Emits events at regular time intervals
		EnabledAtStart	= 1 << 3	// If enabled at start, probe will be enumerated 
	} ProbeAttributes;

protected:

	/** this probes attributes */
	ProbeAttributes Attributes;
	/** True then probe is enabled */
	BOOLEAN Enabled;
	/** Probes Name */
	ANSI_STRING ProbeName;
	/** How often (in ms) does this probe run? */
	LARGE_INTEGER RunInterval;
	/** Last Time Probe Executed */
	LARGE_INTEGER LastProbeRunTime;

public:
	AbstractVirtueProbe(const ANSI_STRING& ProbeName);
	virtual ~AbstractVirtueProbe();
	/* Start the probe - required functionality */
	_Success_(TRUE == return)
		virtual BOOLEAN Start() = 0;
	/* Stop the probe - required functionality */
	_Success_(TRUE == return)
		virtual BOOLEAN Stop() = 0;
	/* Determine probe state where TRUE is enabled else FALSE is disabled */
	_Must_inspect_result_
		virtual BOOLEAN IsEnabled() = 0;
	/** called by the polling thread to do work */
	_Must_inspect_result_
		virtual BOOLEAN OnPoll();
	_Must_inspect_result_
		_Success_(TRUE == NT_SUCCESS(return))
		virtual NTSTATUS OnRun() = 0;
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
	_Must_inspect_result_
	const ANSI_STRING& GetProbeName() const { return this->ProbeName; }
	/** get the last time the probe ran in GMT */
	_Must_inspect_result_
	const LARGE_INTEGER& GetLastProbeRunTime() const { return this->LastProbeRunTime; }
	/** get this probes run interval in absolute time */
	_Must_inspect_result_
	const LARGE_INTEGER& GetRunInterval() const { return this->RunInterval; } 
	/** get probe attributes */
	_Must_inspect_result_
	const ProbeAttributes& GetProbeAttribtes() const { return this->Attributes; }
};