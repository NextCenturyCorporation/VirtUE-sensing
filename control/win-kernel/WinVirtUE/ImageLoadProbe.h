/**
* @file ImageLoadProbe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief  ImageLoad Probe class declaration
*/
#pragma once
#include "common.h"
#include "externs.h"
#include "AbstractVirtueProbe.h"
class ImageLoadProbe :
	public AbstractVirtueProbe
{
private:
	static
	VOID ImageLoadNotificationRoutine(
		_In_ PUNICODE_STRING FullImageName,
		_In_ HANDLE ProcessId,
		_In_ PIMAGE_INFO pImageInfo);

public:
	ImageLoadProbe();
	~ImageLoadProbe() = default;
	_Success_(TRUE == return)
	BOOLEAN Enable();
	_Success_(TRUE == return)
	BOOLEAN Disable();
	_Must_inspect_result_
	BOOLEAN State();
	_Must_inspect_result_
	_Success_(TRUE == NT_SUCCESS(return))
	NTSTATUS Mitigate(
		_In_opt_count_(argc) PCHAR argv[],
		_In_ UINT32 argc);
	_Must_inspect_result_
		NTSTATUS OnRun();
};

