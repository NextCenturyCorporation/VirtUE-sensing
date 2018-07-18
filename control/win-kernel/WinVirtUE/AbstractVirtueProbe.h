/**
* @file AbstractVirtueProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Abstract Base Class for Windows VirtUE Probes
*/
#pragma once
#include "common.h"
#include "jsmn.h"
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
		EnabledAtStart	= 1 << 3,	// If enabled at start, probe will be enumerated 
		DynamicProbe	= 1 << 4	// Set if Probe Type is Dynamic (Loaded by external operation)
	} ProbeAttributes;

protected:
	/** this probes attributes */
	ProbeAttributes Attributes;
	/** True then probe is registered */
	BOOLEAN Registered;
	/** True then probe is enabled */
	BOOLEAN Enabled;
	/** Probes Name */
	ANSI_STRING ProbeName;
	/** How often (in ms) does this probe run? */
	LARGE_INTEGER RunInterval;
	/** Last Time Probe Executed */
	LARGE_INTEGER LastProbeRunTime;
	/** The number of discrete operations since loaded */
	volatile LONG OperationCount;
	/** The number of probes */
	static volatile LONG ProbeCount;
	/** This probes unique probe number */
	UUID ProbeId;

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
	_Must_inspect_result_
		virtual BOOLEAN Configure(_In_ const ANSI_STRING& config_data);
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
		_In_ UINT32 ArgC,
		_In_count_(ArgC) ANSI_STRING ArgV[]) = 0;
	/* construct a new instance of this probe class */
	_Must_inspect_impl_
	PVOID operator new(_In_ size_t size);
	/* destroy an instance of this probe class */
	VOID CDECL operator delete(_In_ PVOID ptr);	
	/* return this probes name */
	virtual const ANSI_STRING& GetProbeName() const { return this->ProbeName; }
	/** get the last time the probe ran in GMT */
	virtual LARGE_INTEGER& GetLastProbeRunTime() { return this->LastProbeRunTime; }
	/** get this probes run interval in absolute time */
	virtual const LARGE_INTEGER& GetRunInterval() const { return this->RunInterval; }
	/** get probe attributes */
	virtual const ProbeAttributes& GetProbeAttribtes() const { return this->Attributes; }
	/** get probe operation count */
	virtual volatile const LONG& GetOperationCount() { return this->OperationCount; }
	/** bump the operation count in a thread safe manner */
	void IncrementOperationCount() { InterlockedIncrement(&this->OperationCount); }
	/** return the number of registered probes */
	static const volatile LONG& GetProbeCount() { return AbstractVirtueProbe::ProbeCount; }
	/** retrieve this probes unique probe number */
	virtual const UUID& GetProbeId() { return this->ProbeId; }
	/** returns the registration state */
	virtual const BOOLEAN& GetIsRegistered() { return this->Registered; }	
};