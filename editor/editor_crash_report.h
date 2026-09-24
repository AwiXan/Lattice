/**************************************************************************/
/*  editor_crash_report.h                                                 */
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

#include "scene/gui/dialogs.h"

class Button;
class Label;
class TextEdit;

// Whether the last session on this project ended without the editor closing,
// and what its log had to say about it.
//
// The editor keeps a log of each session on a project, rotated, in
// .godot/editor/logs - a crash's backtrace is printed as errors, which are
// written out at once, so it is in there even when nothing else made it. A
// session marks itself as running when it starts and unmarks itself when it
// closes; a mark left by a process that is no longer running is a session that
// never closed. The next one says so, with the backtrace, instead of the
// console window that closed too fast to read it.
class EditorCrashReport : public AcceptDialog {
	GDCLASS(EditorCrashReport, AcceptDialog);

	static inline bool previous_session_crashed = false;
	static inline bool backtrace_found = false;
	// The stall watchdog's report ends the log: it froze, and was stopped.
	static inline bool stall_found = false;
	static inline bool owns_marker = false;
	static inline bool tracking = false;
	static inline String previous_log;
	static inline String excerpt;

	Label *message = nullptr;
	TextEdit *text = nullptr;
	Button *restore_button = nullptr;

	static String _marker_path();
	static String _find_previous_log();
	static String _excerpt_of(const String &p_log);

	void _copy_pressed();
	void _open_folder_pressed();
	void _restore_pressed();

protected:
	void _notification(int p_what);

public:
	// Where this project's session logs go.
	static String get_logs_dir();

	// At startup: reads what the last session left behind, and marks this one
	// as running.
	static void begin_session();
	// On closing normally.
	static void end_session();

	static bool did_previous_session_crash() { return previous_session_crashed; }
	// Whether this run is a session someone sits in front of, marked as
	// running and reported on - and so whether it keeps copies of unsaved
	// scenes. See begin_session().
	static bool is_tracking_session() { return tracking; }
	static bool was_backtrace_found() { return backtrace_found; }
	static String get_excerpt() { return excerpt; }

	// Shows what happened last time, if anything did.
	void popup_if_needed();

	EditorCrashReport();
};
