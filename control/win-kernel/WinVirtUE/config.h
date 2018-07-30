/**
* @file config.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief configure the target for configuration, run-time features, components and etc.
*/
#pragma once

/*
* This should be specified when you want to use the debug functionality.This should
* always be turned off when your code is merged into master and should never be
* enabled when doing a release.
*/
#define WVU_DEBUG
//#undef WVU_DEBUG

/* This controls what modules are logged when debugging is enabled.The default
# is LOG_NONE(0).To see all, use LOG_ALL(0xFFFFFFFF).For other examples,
# see debug.h.
*/
//#define LOG_MODULES LOG_NONE
//#define LOG_MODULES (LOG_CORE|LOG_WVU_MAIN|LOG_NOTIFY_PROCESS|LOG_WVU_MAINTHREAD|LOG_CONTAINER|LOG_IOCTL)
//#define LOG_MODULES (LOG_WVU_MAINTHREAD|LOG_FILE_CREATE|LOG_FILE_OP|LOG_CTX)
#define LOG_MODULES (LOG_NOTIFY_THREAD)

/*
* If enabled, attaching a kernel debugger to the machine will BSOD the system.
*/
//#define DENY_KERNEL_DEBUGGING

/*
* If enabled, routines will be run that will attempt to protect the driver
*/
//#define WVU_PROTECT_DRIVER