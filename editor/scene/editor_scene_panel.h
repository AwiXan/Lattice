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

#include "editor/gui/editor_button_mirror.h"
#include "scene/gui/box_container.h"

class Button;
class EditorDebuggerTree;
class LineEdit;
class OptionButton;
class SceneTreeEditor;
class ScrollContainer;

// A scene tree in a pane: the one being edited, or the one running.
//
// The editor has always had both, but only one of each and only in the Scene
// dock, so "remote" was a mode the whole editor was in. Here it belongs to the
// panel: one pane can show the scene being edited while another shows what the
// game is doing with it, and with two games running each pane can watch its own
// session. That is the point of it - comparing two sessions, or a session
// against the scene it came from, is otherwise a matter of clicking back and
// forth and remembering.
//
// Everything else is the Scene dock's. The dock serves every view of the
// scene, not only its own tree: its buttons, its context menu, renaming,
// dragging, dropping and the dialogs all of those open work here as they do
// there, with as many of these panels as there are panes to put them in.
class EditorScenePanel : public VBoxContainer {
	GDCLASS(EditorScenePanel, VBoxContainer);

	// The Scene dock's own row of buttons, copied - with this panel's filter
	// where the dock has its own.
	HBoxContainer *toolbar = nullptr;
	EditorButtonMirror toolbar_mirror;
	LineEdit *filter = nullptr;

	HBoxContainer *mode_bar = nullptr;
	Button *local_button = nullptr;
	Button *remote_button = nullptr;
	// Which running game to watch. Only shown when there is more than one, and
	// the first entry follows whichever session is in front.
	OptionButton *session_button = nullptr;

	SceneTreeEditor *local_tree = nullptr;
	EditorDebuggerTree *remote_tree = nullptr;

	// What the dock offers a scene with no root yet, copied the same way: the
	// only way to give one a root from here, the context menu being for nodes
	// that exist.
	ScrollContainer *create_root_scroll = nullptr;
	VBoxContainer *create_root = nullptr;
	EditorButtonMirror create_root_mirror;
	bool showing_create_root = false;

	bool remote = false;

	void _mode_pressed(bool p_remote);
	void _session_selected(int p_index);
	void _filter_changed(const String &p_text);
	void _local_node_selected();
	void _update_sessions();
	void _update_theme();
	void _show_current();

	void _activate();
	void _tree_input(const Ref<InputEvent> &p_event);
	void _build_toolbar();
	bool _replace_in_toolbar(Node *p_original, Control *p_to);
	void _update_create_root();

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
