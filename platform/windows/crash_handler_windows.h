/**************************************************************************/
/*  crash_handler_windows.h                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include <windows.h>

#include <cstdint>

// Crash handler exception only enabled with MSVC
#if defined(DEBUG_ENABLED)
#define CRASH_HANDLER_EXCEPTION 1

#ifdef _MSC_VER
extern DWORD CrashHandlerException(EXCEPTION_POINTERS *ep);
#endif

#endif

class CrashHandler {
	bool disabled;

public:
	void initialize();

	// For the editor: should its main loop stop going round for seconds on
	// end, the main thread's stack goes to the log, which the crash report of
	// the next start shows. See crash_handler_windows_seh.cpp.
	// p_quiet (a game): nothing printed, where it is stuck only kept for
	// OS.get_crash_log() until it goes on again.
	void start_stall_watchdog(uint64_t p_after_msec = 5000, bool p_quiet = false);
	void stop_stall_watchdog();

	void disable();
	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};
