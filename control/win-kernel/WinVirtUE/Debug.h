/**
* @file externs.h
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief All global variables must have an extern entry in this file
*/
#pragma once
#include "common.h"

/* Custom Debug Print Levels */
#define FLOOD_LEVEL_ID (DPFLTR_MASK | 0x1000)

/* Default Component ID value */
#define COMPONENT_ID (DPFLTR_DEFAULT_ID)

/* Level IDs */
#define ERROR_LEVEL_ID      (DPFLTR_ERROR_LEVEL)
#define WARNING_LEVEL_ID    (DPFLTR_WARNING_LEVEL)
#define TRACE_LEVEL_ID      (DPFLTR_TRACE_LEVEL)
#define INFO_LEVEL_ID       (DPFLTR_INFO_LEVEL)
#define DPFLTR_MASK         0x80000000

/* Module IDs */
#define LOG_NONE            (1 << 0)
#define LOG_UTIL            (1 << 1)
#define LOG_CTX             (1 << 2)
#define LOG_MAIN            (1 << 3)
#define LOG_CRYPTO          (1 << 4)
#define LOG_REGISTRY        (1 << 5)
#define LOG_FLT_MGR         (1 << 6)

#define LOG_FILE_OP         (1 << 8)
#define LOG_FILE_CREATE     (1 << 9)

#define LOG_MAINTHREAD		(1 << 11)
#define LOG_CONTAINER		(1 << 12)
#define LOG_IOCTL           (1 << 13)
#define LOG_PROCESS         (1 << 14)
#define LOG_OP_CALLBACKS    (1 << 15)
#define LOG_NOTIFY_MODULE   (1 << 16)
#define LOG_NOTIFY_THREAD   (1 << 17)
#define LOG_NOTIFY_REGISTRY (1 << 18)
#define LOG_NOTIFY_PROCESS  (1 << 19)
/*
* These are meant to be used when you want to see the logs
* for the major components. Do not use these within an WVU_DEBUG_*
* statement.
*/
#define LOG_WVU                 (LOG_CRYPTO | LOG_REGISTRY | LOG_FILE_OP)
#define LOG_CORE                (LOG_CRYPTO | LOG_CTX | LOG_FLT_MGR)
#define LOG_ALL_FILE_OPS        (LOG_FILE_OP | LOG_FILE_CREATE)
#define LOG_ALL                 (~0)

typedef enum
{
    STRING_UNICODE = 0,
    STRING_WIDE = 1,
} STRING_TYPES;

#if defined(WVU_DEBUG)
/*
* If no default value is provided, set it to none.
*/
#ifndef LOG_MODULES
#define LOG_MODULES LOG_NONE
#endif

/**
* Print a message only when 'DEBUG' is defined.
*/
#define WVU_DEBUG_PRINT(Module, Level, Format, ...)                                         \
    for(;;)                                                                                 \
    {                                                                                       \
        if ((__FILE__,(LOG_MODULES & Module)) != 0)                                         \
        {                                                                                   \
            DbgPrintEx(COMPONENT_ID, Level, __FILE__ "::" __FUNCTION__ " (%d): " Format,    \
                __LINE__, __VA_ARGS__);                                                     \
        }                                                                                   \
        break;                                                                              \
    }

#define WVU_DEBUG_PRINT_ON_COND(Module, Level, Condition, Format, ...)                      \
    for(;;)                                                                                 \
    {                                                                                       \
        if ((__FILE__,(LOG_MODULES & Module)) != 0 && Condition)                            \
        {                                                                                   \
            DbgPrintEx(COMPONENT_ID, Level, __FILE__ "::" __FUNCTION__ " (%d): " Format,    \
                __LINE__, __VA_ARGS__);                                                     \
        }                                                                                   \
        break;                                                                              \
    }

/**
* Print a file name.
*/
#define WVU_DEBUG_PRINT_FILENAME(Module, String, Type, Level)   \
    for(;;)                                                     \
    {                                                           \
        if ((__FILE__,(LOG_MODULES & Module)) != 0)             \
        {                                                       \
            WVUDebugPrintFileName(String, Type, Level);         \
        }                                                       \
        break;                                                  \
    }

/**
* Cause a break point.
*/
#define WVU_DEBUG_BREAK()               \
    for(;;)                             \
    {                                   \
        void WVUDebugBreakPoint();      \
        break;                          \
    }

/**
* Print a buffer with some pretty formatting.
*/
#define WVU_DEBUG_PRINT_BUFFER(Module, Buffer, Size)    \
    for(;;)                                             \
    {                                                   \
        if ((__FILE__,(LOG_MODULES & Module)) != 0)     \
        {                                               \
            WVUDebugPrintBuffer(Buffer, Size);          \
        }                                               \
        break;                                          \
    }

/**
* Break if the Target string is contained within the String.
*/
#define WVU_DEBUG_BREAK_ON_FILENAME(String, Target, Type)       \
    for(;;)                                                     \
    {                                                           \
        WVUDebugBreakOnMatch(String, Target, Type);             \
        break;                                                  \
    }

void WVUDebugPrintFileName(PVOID String, INT32 Type, INT32 Level);
void WVUDebugBreakPoint();
void WVUDebugPrintBuffer(PUCHAR Buffer, UINT32 Size);
void WVUDebugBreakOnMatch(PVOID String, WCHAR *Target, INT32 Type);

#else /* No Debug */

#define WVU_DEBUG_PRINT(Module, Level, Format, ...) for(;;)break
#define WVU_DEBUG_PRINT_ON_COND(Module, Level, Condition, Format, ...) for(;;)break
#define WVU_DEBUG_PRINT_FILENAME(Module, String, Type) for(;;)break
#define WVU_DEBUG_BREAK() for(;;)break
#define WVU_DEBUG_PRINT_BUFFER(Module, Buffer, Size) for(;;)break
#define WVU_DEBUG_BREAK_ON_FILENAME(String, Target, Type) for(;;)break

#endif /* WVU_DEBUG */