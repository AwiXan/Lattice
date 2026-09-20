/**************************************************************************/
/*  editor_scene_panel.h                                                  */
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

#include "scene/gui/box_container.h"

class Button;
class EditorDebuggerTree;
class OptionButton;
class SceneTreeEditor;

// A scene tree in a pane: the one being edited, or the one running.
//
// The editor has always had both, but only one of each and only in the Scene
// dock, so "remote" was a mode the whole editor was in. Here it belongs to the
// panel: one pane can show the scene being edited while another shows what the
// game is doing with it, and with two games running each pane can watch its own
// session. That is the point of it - comparing two sessions, or a session
// against the scene it came from, is otherwise a matter of clicking back and
// forth and remembering.
class EditorScenePanel : public VBoxContainer {
	GDCLASS(EditorScenePanel, VBoxContainer);

	HBoxContainer *bar = nullptr;
	Button *local_button = nullptr;
	Button *remote_button = nullptr;
	// Which running game to watch. Only shown when there is more than one, and
	// the first entry follows whichever session is in front.
	OptionButton *session_button = nullptr;

	SceneTreeEditor *local_tree = nullptr;
	EditorDebuggerTree *remote_tree = nullptr;

	bool remote = false;

	void _mode_pressed(bool p_remote);
	void _session_selected(int p_index);
	void _update_sessions();
	void _update_theme();
	void _show_current();

protected:
	void _notification(int p_what);

public:
	void set_remote(bool p_remote);
	bool is_remote() const { return remote; }

	SceneTreeEditor *get_local_tree() const { return local_tree; }

	// What a pane records about this panel, so that a pane left on the running
	// game comes back on it.
	Dictionary save_state() const;
	void load_state(const Dictionary &p_state);

	EditorScenePanel();
	~EditorScenePanel();
};
