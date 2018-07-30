/**
* @file WinVirtUe.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief VirtUE Startup Module
*/
#pragma once
#include "common.h"
#include "externs.h"

KSTART_ROUTINE WVUMainInitThread;
KSTART_ROUTINE WVUProbeCommsThread;
_Has_lock_kind_(_Lock_kind_semaphore_)
KSTART_ROUTINE WVUTemporalProbeThread;