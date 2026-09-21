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
#include "core/input/input_event.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/editor_debugger_tree.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"

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
	// The buttons and the filter are about the scene being edited; the running
	// one is read-only and changes under them.
	toolbar->set_visible(!remote);
	remote_tree->set_visible(remote);
	_update_create_root();
	_update_sessions();
}

void EditorScenePanel::_filter_changed(const String &p_text) {
	local_tree->set_filter(p_text);
}

void EditorScenePanel::_activate() {
	// Working in this panel is working on its scene: the editor comes to it
	// first - the same as clicking into a view of it does - and the Scene dock
	// is told which of its views is being worked in, so that what it does next
	// happens here.
	const int document = local_tree->get_bound_document();
	if (document >= 0) {
		EditorData &editor_data = EditorNode::get_editor_data();
		const int index = editor_data.get_scene_index_by_history_id(document);
		if (index >= 0 && index != editor_data.get_edited_scene()) {
			EditorNode::get_singleton()->set_current_scene_index(index);
		}
	}
	SceneTreeDock *dock = SceneTreeDock::get_singleton();
	if (dock) {
		dock->set_active_tree_view(local_tree);
	}
}

void EditorScenePanel::_tree_input(const Ref<InputEvent> &p_event) {
	// Before the tree acts on a click, so that whatever the click leads to - a
	// context menu, a rename - is already about this panel.
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed()) {
		_activate();
		return;
	}

	const Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && !key->is_echo() && ED_IS_SHORTCUT("editor/open_search", p_event)) {
		// This panel's filter, not the dock's.
		filter->grab_focus();
		filter->select_all();
		local_tree->get_scene_tree()->accept_event();
	}
}

void EditorScenePanel::_local_node_selected() {
	Node *node = local_tree->get_selected();
	if (!node) {
		return;
	}

	_activate();
	// What the Scene dock does with a pick. Without it the node was selected -
	// the views outlined it - but nobody told the inspector.
	EditorNode *editor = EditorNode::get_singleton();
	if (editor->get_editor_selection_history()->get_current() != node->get_instance_id()) {
		editor->push_node_item(node);
	}
}

bool EditorScenePanel::_replace_in_toolbar(Node *p_original, Control *p_to) {
	LineEdit *original_filter = Object::cast_to<LineEdit>(p_original);
	if (!original_filter) {
		return false;
	}
	// Where the dock has its filter, this panel has its own: it narrows this
	// tree, and a panel showing another scene is filtered on its own.
	filter->set_placeholder(original_filter->get_placeholder());
	filter->set_tooltip_text(original_filter->get_tooltip_text());
	filter->set_h_size_flags(original_filter->get_h_size_flags());
	if (filter->get_parent()) {
		filter->get_parent()->remove_child(filter);
	}
	p_to->add_child(filter);
	return true;
}

void EditorScenePanel::_build_toolbar() {
	SceneTreeDock *dock = SceneTreeDock::get_singleton();
	if (!dock || !dock->get_toolbar() || filter->get_parent()) {
		return;
	}
	toolbar_mirror.mirror_all(dock->get_toolbar(), toolbar, callable_mp(this, &EditorScenePanel::_replace_in_toolbar));
	if (!filter->get_parent()) {
		toolbar->add_child(filter);
	}
}

void EditorScenePanel::_update_create_root() {
	const bool empty = !remote && local_tree->is_inside_tree() && !local_tree->get_scene_node();
	local_tree->set_visible(!remote && !empty);
	create_root_scroll->set_visible(empty);
	if (empty == showing_create_root) {
		return;
	}
	showing_create_root = empty;

	for (int i = create_root->get_child_count() - 1; i >= 0; i--) {
		Node *child = create_root->get_child(i);
		create_root->remove_child(child);
		child->queue_free();
	}
	create_root_mirror.clear();

	SceneTreeDock *dock = SceneTreeDock::get_singleton();
	if (empty && dock && dock->get_create_root_options()) {
		create_root_mirror.mirror_all(dock->get_create_root_options(), create_root);
	}
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
	// What the run bar shows for a running game, rather than RemoteDebug, which
	// reads as a broken file to anyone not steeped in the debugger's menus.
	remote_button->set_button_icon(base->get_editor_theme_icon(SNAME("PlayScene")));
}

void EditorScenePanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
			if (debugger) {
				debugger->register_remote_tree(remote_tree);
			}
			SceneTreeDock *dock = SceneTreeDock::get_singleton();
			if (dock) {
				dock->attach_tree_view(local_tree);
			}
			_build_toolbar();
			_update_theme();
			_show_current();
			set_process_internal(is_visible_in_tree());
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process_internal(is_visible_in_tree());
			if (is_visible_in_tree() && remote) {
				_update_sessions();
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			// The copies follow the dock's buttons - which of the script ones is
			// offered depends on what is selected - and a scene can gain or lose
			// its root at any time. Both are looked at rather than listened for;
			// neither says when it changes.
			toolbar_mirror.sync();
			_update_create_root();
			if (showing_create_root) {
				create_root_mirror.sync();
			}
		} break;

		case NOTIFICATION_PREDELETE: {
			// The debugger keeps a list of the trees it feeds, and the Scene dock
			// a list of its views, and this one is about to stop existing.
			EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
			if (debugger && remote_tree) {
				debugger->unregister_remote_tree(remote_tree);
			}
			SceneTreeDock *dock = SceneTreeDock::get_singleton();
			if (dock && local_tree) {
				dock->detach_tree_view(local_tree);
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

	toolbar = memnew(HBoxContainer);
	add_child(toolbar);
	toolbar_mirror.set_before_press(callable_mp(this, &EditorScenePanel::_activate));
	toolbar_mirror.set_shortcut_context(this);

	filter = memnew(LineEdit);
	filter->set_placeholder(TTRC("Filter Nodes"));
	filter->set_clear_button_enabled(true);
	filter->set_h_size_flags(SIZE_EXPAND_FILL);
	filter->add_theme_constant_override("minimum_character_width", 0);
	filter->connect(SceneStringName(text_changed), callable_mp(this, &EditorScenePanel::_filter_changed));

	mode_bar = memnew(HBoxContainer);
	add_child(mode_bar);

	local_button = memnew(Button);
	local_button->set_toggle_mode(true);
	local_button->set_pressed(true);
	local_button->set_flat(true);
	local_button->set_focus_mode(FOCUS_NONE);
	local_button->set_text(TTRC("Local"));
	local_button->set_tooltip_text(TTRC("Show the scene being edited."));
	local_button->connect(SceneStringName(pressed), callable_mp(this, &EditorScenePanel::_mode_pressed).bind(false));
	mode_bar->add_child(local_button);

	remote_button = memnew(Button);
	remote_button->set_toggle_mode(true);
	remote_button->set_flat(true);
	remote_button->set_focus_mode(FOCUS_NONE);
	remote_button->set_text(TTRC("Remote"));
	remote_button->set_tooltip_text(TTRC("Show the scene the running game has."));
	remote_button->connect(SceneStringName(pressed), callable_mp(this, &EditorScenePanel::_mode_pressed).bind(true));
	mode_bar->add_child(remote_button);

	session_button = memnew(OptionButton);
	session_button->set_flat(true);
	session_button->set_h_size_flags(SIZE_EXPAND_FILL);
	session_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	session_button->set_tooltip_text(TTRC("Which running game this panel watches."));
	session_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorScenePanel::_session_selected));
	session_button->hide();
	mode_bar->add_child(session_button);

	local_tree = Object::cast_to<SceneTreeEditor>(SceneTreeEditor::create_panel());
	local_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	// Deferred, as the Scene dock does it: the tree reports a pick while it is
	// still in the middle of changing its own selection.
	local_tree->connect("node_selected", callable_mp(this, &EditorScenePanel::_local_node_selected), CONNECT_DEFERRED);
	// Connected before the Scene dock connects its own, so that by the time the
	// dock acts on a click, a drop or a rename here, it is acting on this
	// panel's scene. A drop in particular comes with nothing clicked here first.
	local_tree->get_scene_tree()->connect(SceneStringName(gui_input), callable_mp(this, &EditorScenePanel::_tree_input));
	local_tree->get_scene_tree()->connect(SceneStringName(focus_entered), callable_mp(this, &EditorScenePanel::_activate));
	local_tree->connect("nodes_rearranged", callable_mp(this, &EditorScenePanel::_activate).unbind(3));
	local_tree->connect("files_dropped", callable_mp(this, &EditorScenePanel::_activate).unbind(3));
	local_tree->connect("script_dropped", callable_mp(this, &EditorScenePanel::_activate).unbind(2));
	add_child(local_tree);

	create_root_scroll = memnew(ScrollContainer);
	create_root_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	create_root_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	create_root_scroll->hide();
	add_child(create_root_scroll);

	create_root = memnew(VBoxContainer);
	create_root->set_h_size_flags(SIZE_EXPAND_FILL);
	create_root_scroll->add_child(create_root);
	create_root_mirror.set_before_press(callable_mp(this, &EditorScenePanel::_activate));
	create_root_mirror.set_shortcut_context(this);

	remote_tree = memnew(EditorDebuggerTree);
	remote_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	remote_tree->hide();
	add_child(remote_tree);
}

EditorScenePanel::~EditorScenePanel() {
}
