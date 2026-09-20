/**************************************************************************/
/*  editor_scene_panel.cpp                                                */
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

#include "editor_scene_panel.h"

#include "core/object/callable_mp.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/editor_debugger_tree.h"
#include "editor/editor_node.h"
#include "editor/scene/scene_tree_editor.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"

void EditorScenePanel::_mode_pressed(bool p_remote) {
	set_remote(p_remote);
}

void EditorScenePanel::set_remote(bool p_remote) {
	remote = p_remote;
	local_button->set_pressed_no_signal(!remote);
	remote_button->set_pressed_no_signal(remote);
	_show_current();

	if (remote) {
		// It has nothing in it until the session answers, so ask now rather
		// than waiting for the next refresh.
		EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
		if (debugger) {
			debugger->request_remote_tree();
		}
	}
}

void EditorScenePanel::_show_current() {
	local_tree->set_visible(!remote);
	remote_tree->set_visible(remote);
	_update_sessions();
}

void EditorScenePanel::_session_selected(int p_index) {
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (!debugger) {
		return;
	}
	// The first entry is "whichever is in front"; the rest are sessions by
	// number, so the metadata says which.
	debugger->set_remote_tree_session(remote_tree, session_button->get_item_metadata(p_index));
}

void EditorScenePanel::_update_sessions() {
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (!debugger) {
		session_button->hide();
		return;
	}

	const int count = debugger->get_session_count();
	// With one game running there is nothing to choose between, so the choice
	// is not offered.
	session_button->set_visible(remote && count > 1);
	if (!session_button->is_visible()) {
		return;
	}

	const int watched = debugger->get_remote_tree_session(remote_tree);
	session_button->clear();
	session_button->add_item(TTRC("Current session"), 0);
	session_button->set_item_metadata(0, EditorDebuggerNode::FOLLOW_CURRENT);
	if (watched == EditorDebuggerNode::FOLLOW_CURRENT) {
		session_button->select(0);
	}
	for (int i = 0; i < count; i++) {
		const int index = session_button->get_item_count();
		session_button->add_item(debugger->get_session_name(i), index);
		session_button->set_item_metadata(index, i);
		if (watched == i) {
			session_button->select(index);
		}
	}
}

void EditorScenePanel::_update_theme() {
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	if (!base) {
		return;
	}
	local_button->set_button_icon(base->get_editor_theme_icon(SNAME("PackedScene")));
	remote_button->set_button_icon(base->get_editor_theme_icon(SNAME("RemoteDebug")));
}

void EditorScenePanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
			if (debugger) {
				debugger->register_remote_tree(remote_tree);
			}
			_update_theme();
			_show_current();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree() && remote) {
				_update_sessions();
			}
		} break;

		case NOTIFICATION_PREDELETE: {
			// The debugger keeps a list of the trees it feeds, and this one is
			// about to stop existing.
			EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
			if (debugger && remote_tree) {
				debugger->unregister_remote_tree(remote_tree);
			}
		} break;
	}
}

Dictionary EditorScenePanel::save_state() const {
	Dictionary state;
	state["remote"] = remote;
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (debugger && remote_tree) {
		state["session"] = debugger->get_remote_tree_session(remote_tree);
	}
	return state;
}

void EditorScenePanel::load_state(const Dictionary &p_state) {
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (debugger && remote_tree) {
		debugger->set_remote_tree_session(remote_tree, p_state.get("session", EditorDebuggerNode::FOLLOW_CURRENT));
	}
	set_remote(p_state.get("remote", false));
}

EditorScenePanel::EditorScenePanel() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);

	bar = memnew(HBoxContainer);
	add_child(bar);

	local_button = memnew(Button);
	local_button->set_toggle_mode(true);
	local_button->set_pressed(true);
	local_button->set_flat(true);
	local_button->set_focus_mode(FOCUS_NONE);
	local_button->set_text(TTRC("Local"));
	local_button->set_tooltip_text(TTRC("Show the scene being edited."));
	local_button->connect(SceneStringName(pressed), callable_mp(this, &EditorScenePanel::_mode_pressed).bind(false));
	bar->add_child(local_button);

	remote_button = memnew(Button);
	remote_button->set_toggle_mode(true);
	remote_button->set_flat(true);
	remote_button->set_focus_mode(FOCUS_NONE);
	remote_button->set_text(TTRC("Remote"));
	remote_button->set_tooltip_text(TTRC("Show the scene the running game has."));
	remote_button->connect(SceneStringName(pressed), callable_mp(this, &EditorScenePanel::_mode_pressed).bind(true));
	bar->add_child(remote_button);

	session_button = memnew(OptionButton);
	session_button->set_flat(true);
	session_button->set_h_size_flags(SIZE_EXPAND_FILL);
	session_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	session_button->set_tooltip_text(TTRC("Which running game this panel watches."));
	session_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorScenePanel::_session_selected));
	session_button->hide();
	bar->add_child(session_button);

	local_tree = Object::cast_to<SceneTreeEditor>(SceneTreeEditor::create_panel());
	local_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(local_tree);

	remote_tree = memnew(EditorDebuggerTree);
	remote_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	remote_tree->hide();
	add_child(remote_tree);
}

EditorScenePanel::~EditorScenePanel() {
}
