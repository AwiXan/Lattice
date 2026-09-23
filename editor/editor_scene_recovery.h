/**************************************************************************/
/*  editor_scene_recovery.h                                               */
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

#include "scene/main/node.h"

class Timer;

// Copies of scenes with unsaved changes, kept while the editor runs, for when
// it does not close properly.
//
// Every few minutes (interface/editor/behavior/recovery_copy_interval) each open scene
// that has changed since its last copy is packed into
// .godot/editor/recovery/current. A scene saved or closed takes its copy with
// it, and closing the editor properly takes them all: they only matter when
// something went wrong. After a crash, the next session sets them aside and
// the crash report offers them back - each opened in place of the file it
// came from, unsaved, to be looked over and saved.
class EditorSceneRecovery : public Node {
	GDCLASS(EditorSceneRecovery, Node);

	static inline EditorSceneRecovery *singleton = nullptr;
	static inline int previous_count = 0;

	Timer *timer = nullptr;
	// For each document, the version of its history last copied, so an
	// unchanged scene is not packed over and over.
	HashMap<int, uint64_t> copied_versions;
	HashMap<int, String> copied_files;

	static String _dir(const String &p_which);
	static void _remove_dir(const String &p_dir);
	static String _file_for(int p_history_id, const String &p_scene_path);

	void _forget(int p_history_id);
	void _scene_saved(const String &p_path);
	void _update_interval();

protected:
	void _notification(int p_what);

public:
	static EditorSceneRecovery *get_singleton() { return singleton; }

	// At startup, before anything is copied: what a session that crashed left
	// is set aside to be offered; anything else left over is thrown away.
	static void begin_session(bool p_previous_session_crashed);
	// On closing properly: nothing to recover.
	static void end_session();
	static int get_previous_count() { return previous_count; }

	// Copies every open scene with changes not yet copied.
	void save_now();
	// Opens what the crashed session left, in place of the files it came
	// from. Returns how many scenes were brought back.
	int restore_previous();

	EditorSceneRecovery();
	~EditorSceneRecovery();
};
