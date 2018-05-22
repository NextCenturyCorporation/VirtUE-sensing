/**
* @file WinVirtUe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup Module
*/
#pragma once
#include "common.h"
#include "ObjectCreateNotifyCB.h"
#include "RegistryModificationCB.h"
#include "externs.h"

KSTART_ROUTINE WVUMainThreadStart;
KSTART_ROUTINE WVUSensorThread;