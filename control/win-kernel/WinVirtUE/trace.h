/**
* @file trace.h
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief Defines WPP constants, macros and etc.
*/
#pragma once

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
		WinVirtUEGUID, (DF241AD6,F4BB,4F0A,865D,F7FE49059BC2),  \
		WPP_DEFINE_BIT(TRACE_ALL)		\
		WPP_DEFINE_BIT(TRACE_DRIVER)	\
        WPP_DEFINE_BIT(TRACE_UTIL)		\
        WPP_DEFINE_BIT(TRACE_CTX)		\
        WPP_DEFINE_BIT(TRACE_MAIN)		\
		WPP_DEFINE_BIT(TRACE_CRYPTo)    \
		WPP_DEFINE_BIT(TRACE_REGISTRY)  \
		WPP_DEFINE_BIT(TRACE_FLT_MGR)		\
		WPP_DEFINE_BIT(TRACE_CACHE)		\
		WPP_DEFINE_BIT(TRACE_FILE_OP)		\
		WPP_DEFINE_BIT(TRACE_FILE_CREATE)		\
		WPP_DEFINE_BIT(TRACE_NOTIFY_PROCS)	\
		WPP_DEFINE_BIT(TRACE_MAINTHREAD)	\
		WPP_DEFINE_BIT(TRACE_CONTAINER)	\
		WPP_DEFINE_BIT(TRACE_IOCTL)		\
		WPP_DEFINE_BIT(TRACE_PROCESS)		\
		WPP_DEFINE_BIT(TRACE_OP_CALLBACKS)

#define WPP_FLAG_LEVEL_LOGGER(flag, level) WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAGS=TRACE_ALL}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// FUNC KdPrint{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_DRIVER}((MSG, ...));
// end_wpp
//