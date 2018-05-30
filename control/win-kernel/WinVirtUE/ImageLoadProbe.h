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
	~ImageLoadProbe();
	BOOLEAN Enable();
	BOOLEAN Disable();
	BOOLEAN State();
	BOOLEAN Mitigate();
};

