/**************************************************************************/
/*  editor_pane.cpp                                                       */
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

#include "editor_pane.h"

#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"

void EditorPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::BOOL, "vertical")));
	ADD_SIGNAL(MethodInfo("close_requested"));
}

void EditorPane::_build_header() {
	header = memnew(HBoxContainer);
	add_child(header);

	type_button = memnew(OptionButton);
	type_button->set_h_size_flags(SIZE_EXPAND_FILL);
	type_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	type_button->set_tooltip_text(TTRC("What this pane shows."));
	// Filled when opened rather than kept in step with the registry, so a type
	// registered later needs to tell nobody.
	type_button->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorPane::_update_type_list));
	type_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorPane::_type_selected));
	header->add_child(type_button);

	subject_button = memnew(OptionButton);
	subject_button->set_h_size_flags(SIZE_EXPAND_FILL);
	subject_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	subject_button->set_tooltip_text(TTRC("The scene this pane is showing. It does not have to be the one the tab bar has selected."));
	subject_button->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorPane::_update_subject_list));
	subject_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorPane::_subject_selected));
	subject_button->hide();
	header->add_child(subject_button);

	split_right_button = memnew(Button);
	split_right_button->set_flat(true);
	split_right_button->set_text(TTRC("Split"));
	split_right_button->set_tooltip_text(TTRC("Put another pane beside this one."));
	split_right_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_split_pressed).bind(false));
	header->add_child(split_right_button);

	split_down_button = memnew(Button);
	split_down_button->set_flat(true);
	split_down_button->set_text(TTRC("Split Down"));
	split_down_button->set_tooltip_text(TTRC("Put another pane below this one."));
	split_down_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_split_pressed).bind(true));
	header->add_child(split_down_button);

	close_button = memnew(Button);
	close_button->set_flat(true);
	close_button->set_text(TTRC("Close"));
	close_button->set_tooltip_text(TTRC("Close this pane."));
	close_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_close_pressed));
	header->add_child(close_button);
}

void EditorPane::_update_type_list() {
	type_button->clear();
	if (panel_adopted) {
		return;
	}
	int index = 0;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		type_button->add_item(type->title.is_empty() ? String(id) : type->title, index);
		type_button->set_item_metadata(index, String(id));
		if (id == panel_type) {
			type_button->select(index);
		}
		index++;
	}
}

void EditorPane::_update_subject_list() {
	subject_button->clear();

	EditorData &editor_data = EditorNode::get_editor_data();
	subject_button->add_item(TTRC("Follow the current scene"), 0);
	subject_button->set_item_metadata(0, -1);
	if (panel_subject.get_type() == Variant::NIL || (int)panel_subject < 0) {
		subject_button->select(0);
	}

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		const int history_id = editor_data.get_scene_history_id(i);
		String title = editor_data.get_scene_title(i);
		if (title.is_empty()) {
			title = TTR("[unsaved]");
		}
		const int index = subject_button->get_item_count();
		subject_button->add_item(title, index);
		subject_button->set_item_metadata(index, history_id);
		if (panel_subject.get_type() == Variant::INT && (int)panel_subject == history_id) {
			subject_button->select(index);
		}
	}
}

void EditorPane::_type_selected(int p_index) {
	const StringName id = StringName(String(type_button->get_item_metadata(p_index)));
	if (id == panel_type) {
		return;
	}
	// Keeping what it was pointed at: switching a pane from a 3D view to a tree
	// should leave it on the same scene.
	set_panel_type(id, panel_subject);
}

void EditorPane::_subject_selected(int p_index) {
	const Variant subject = subject_button->get_item_metadata(p_index);
	panel_subject = subject;
	EditorPanelRegistry::bind_panel(panel_type, panel, panel_subject);
}

void EditorPane::_split_pressed(bool p_vertical) {
	emit_signal(SNAME("split_requested"), p_vertical);
}

void EditorPane::_close_pressed() {
	emit_signal(SNAME("close_requested"));
}

void EditorPane::set_panel_type(const StringName &p_type, const Variant &p_subject) {
	ERR_FAIL_COND_MSG(panel_adopted, "This pane shows a Control the editor owns and cannot be given another.");

	Control *new_panel = EditorPanelRegistry::create_panel(p_type);
	if (!new_panel) {
		// The type refused - a plugin that cannot be shown twice says so by
		// handing back nothing - and the pane keeps what it had.
		return;
	}

	if (panel) {
		memdelete(panel);
		panel = nullptr;
	}

	panel_type = p_type;
	panel_subject = p_subject;
	panel = new_panel;
	panel->set_v_size_flags(SIZE_EXPAND_FILL);
	panel->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(panel);

	EditorPanelRegistry::bind_panel(panel_type, panel, panel_subject);

	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(panel_type);
	type_button->set_text(type && !type->title.is_empty() ? type->title : String(panel_type));
	// Only a panel that shows one document offers a scene to choose; a resource
	// panel is pointed at its resource by whatever opened it.
	subject_button->set_visible(type && type->binding == EditorPanelRegistry::BINDING_DOCUMENT);
	_update_subject_list();
}

void EditorPane::adopt_panel(Control *p_panel, const String &p_title) {
	ERR_FAIL_NULL(p_panel);
	ERR_FAIL_COND_MSG(panel, "This pane already shows something.");

	panel = p_panel;
	panel_adopted = true;
	adopted_title = p_title;
	panel->set_v_size_flags(SIZE_EXPAND_FILL);
	panel->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(panel);

	type_button->set_text(p_title);
	type_button->set_disabled(true);
	subject_button->hide();
}

void EditorPane::set_closable(bool p_closable) {
	close_button->set_visible(p_closable);
}

void EditorPane::set_header_visible(bool p_visible) {
	header->set_visible(p_visible);
}

Control *EditorPane::release_adopted_panel() {
	if (!panel_adopted || !panel) {
		return nullptr;
	}
	Control *released = panel;
	remove_child(released);
	panel = nullptr;
	panel_adopted = false;
	return released;
}

EditorPane::EditorPane() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);
	_build_header();
}
