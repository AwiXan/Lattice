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
#include "scene/gui/tab_bar.h"

void EditorPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::BOOL, "vertical")));
	ADD_SIGNAL(MethodInfo("close_requested"));
	ADD_SIGNAL(MethodInfo("panels_changed"));
}

void EditorPane::_build_header() {
	header = memnew(HBoxContainer);
	add_child(header);

	tab_bar = memnew(TabBar);
	tab_bar->set_h_size_flags(SIZE_EXPAND_FILL);
	tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ACTIVE_ONLY);
	tab_bar->connect(SNAME("tab_selected"), callable_mp(this, &EditorPane::_tab_selected));
	tab_bar->connect(SNAME("tab_close_pressed"), callable_mp(this, &EditorPane::_tab_close_pressed));
	header->add_child(tab_bar);

	add_button = memnew(OptionButton);
	add_button->set_text(TTRC("Add"));
	add_button->set_tooltip_text(TTRC("Add another panel to this pane."));
	// Filled when opened rather than kept in step with the registry, so a type
	// registered later needs to tell nobody.
	add_button->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorPane::_update_add_list));
	add_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorPane::_add_selected));
	header->add_child(add_button);

	subject_button = memnew(OptionButton);
	subject_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	subject_button->set_tooltip_text(TTRC("The scene this panel is showing. It does not have to be the one the tab bar has selected."));
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
	close_button->set_text(TTRC("Close Pane"));
	close_button->set_tooltip_text(TTRC("Close this pane."));
	close_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_close_pressed));
	header->add_child(close_button);
}

String EditorPane::_title_of(const PanelEntry &p_entry) const {
	if (p_entry.adopted) {
		return p_entry.title;
	}
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_entry.type);
	return type && !type->title.is_empty() ? type->title : String(p_entry.type);
}

void EditorPane::_update_tabs() {
	rebuilding_tabs = true;
	tab_bar->clear_tabs();
	for (const PanelEntry &entry : panels) {
		tab_bar->add_tab(_title_of(entry));
	}
	if (current >= 0 && current < tab_bar->get_tab_count()) {
		tab_bar->set_current_tab(current);
	}

	const EditorPanelRegistry::PanelType *type = (current >= 0 && current < panels.size()) ? EditorPanelRegistry::get_type(panels[current].type) : nullptr;
	// Only a panel that shows one document offers a scene to choose; a resource
	// panel is pointed at its resource by whatever opened it.
	subject_button->set_visible(type && type->binding == EditorPanelRegistry::BINDING_DOCUMENT);
	if (subject_button->is_visible()) {
		_update_subject_list();
	}
	// A pane holding more than one panel always needs its tabs.
	if (panels.size() > 1) {
		header->show();
	}
	rebuilding_tabs = false;
}

void EditorPane::_show_only_current() {
	for (int i = 0; i < panels.size(); i++) {
		if (panels[i].control) {
			panels[i].control->set_visible(i == current);
		}
	}
}

void EditorPane::_update_add_list() {
	add_button->clear();
	int index = 0;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		add_button->add_item(type->title.is_empty() ? String(id) : type->title, index);
		add_button->set_item_metadata(index, String(id));
		index++;
	}
}

void EditorPane::_update_subject_list() {
	subject_button->clear();

	EditorData &editor_data = EditorNode::get_editor_data();
	const Variant subject = (current >= 0 && current < panels.size()) ? panels[current].subject : Variant();

	subject_button->add_item(TTRC("Follow the current scene"), 0);
	subject_button->set_item_metadata(0, -1);
	if (subject.get_type() != Variant::INT || (int)subject < 0) {
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
		if (subject.get_type() == Variant::INT && (int)subject == history_id) {
			subject_button->select(index);
		}
	}
}

void EditorPane::_tab_selected(int p_index) {
	if (rebuilding_tabs) {
		return;
	}
	set_current_panel(p_index);
}

void EditorPane::_tab_close_pressed(int p_index) {
	close_panel(p_index);
}

void EditorPane::_add_selected(int p_index) {
	const StringName id = StringName(String(add_button->get_item_metadata(p_index)));
	// A new panel starts on whatever this pane is already showing, so adding one
	// is a way to look at the same scene differently.
	add_panel(id, get_panel_subject());
}

void EditorPane::_subject_selected(int p_index) {
	set_panel_subject(subject_button->get_item_metadata(p_index));
}

void EditorPane::_split_pressed(bool p_vertical) {
	emit_signal(SNAME("split_requested"), p_vertical);
}

void EditorPane::_close_pressed() {
	emit_signal(SNAME("close_requested"));
}

int EditorPane::add_panel(const StringName &p_type, const Variant &p_subject) {
	Control *control = EditorPanelRegistry::create_panel(p_type);
	if (!control) {
		// The type refused - a plugin that cannot be shown twice says so by
		// handing back nothing.
		return -1;
	}

	PanelEntry entry;
	entry.type = p_type;
	entry.subject = p_subject;
	entry.control = control;
	control->set_v_size_flags(SIZE_EXPAND_FILL);
	control->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(control);
	EditorPanelRegistry::bind_panel(p_type, control, p_subject);

	panels.push_back(entry);
	current = panels.size() - 1;
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
	return current;
}

void EditorPane::close_panel(int p_index) {
	ERR_FAIL_INDEX(p_index, panels.size());
	if (panels[p_index].adopted) {
		// This is the editor's main screen, which has to be somewhere.
		return;
	}

	if (panels[p_index].control) {
		memdelete(panels[p_index].control);
	}
	panels.remove_at(p_index);

	if (panels.is_empty()) {
		current = -1;
	} else {
		current = CLAMP(current >= p_index ? current - 1 : current, 0, panels.size() - 1);
	}
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
}

void EditorPane::set_current_panel(int p_index) {
	if (p_index < 0 || p_index >= panels.size() || p_index == current) {
		return;
	}
	current = p_index;
	_show_only_current();
	_update_tabs();
}

StringName EditorPane::get_panel_type_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return StringName();
	}
	return panels[p_index].type;
}

Variant EditorPane::get_panel_subject_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return Variant();
	}
	return panels[p_index].subject;
}

Control *EditorPane::get_panel_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return nullptr;
	}
	return panels[p_index].control;
}

bool EditorPane::is_panel_adopted_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return false;
	}
	return panels[p_index].adopted;
}

String EditorPane::get_panel_title_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return String();
	}
	return _title_of(panels[p_index]);
}

void EditorPane::set_panel_type(const StringName &p_type, const Variant &p_subject) {
	// Everything but the editor's main screen, which cannot be dropped.
	for (int i = panels.size() - 1; i >= 0; i--) {
		if (!panels[i].adopted) {
			if (panels[i].control) {
				memdelete(panels[i].control);
			}
			panels.remove_at(i);
		}
	}
	current = panels.is_empty() ? -1 : 0;
	add_panel(p_type, p_subject);
}

void EditorPane::set_panel_subject(const Variant &p_subject) {
	if (current < 0 || current >= panels.size()) {
		return;
	}
	panels.write[current].subject = p_subject;
	EditorPanelRegistry::bind_panel(panels[current].type, panels[current].control, p_subject);
}

void EditorPane::adopt_panel(Control *p_panel, const String &p_title) {
	ERR_FAIL_NULL(p_panel);
	ERR_FAIL_COND_MSG(is_adopting(), "This pane already shows the editor's main screen.");

	PanelEntry entry;
	entry.control = p_panel;
	entry.adopted = true;
	entry.title = p_title;
	p_panel->set_v_size_flags(SIZE_EXPAND_FILL);
	p_panel->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(p_panel);

	panels.insert(0, entry);
	current = 0;
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
}

bool EditorPane::is_adopting() const {
	for (const PanelEntry &entry : panels) {
		if (entry.adopted) {
			return true;
		}
	}
	return false;
}

String EditorPane::get_adopted_title() const {
	for (const PanelEntry &entry : panels) {
		if (entry.adopted) {
			return entry.title;
		}
	}
	return String();
}

Control *EditorPane::release_adopted_panel() {
	for (int i = 0; i < panels.size(); i++) {
		if (!panels[i].adopted) {
			continue;
		}
		Control *released = panels[i].control;
		if (released && released->get_parent() == this) {
			remove_child(released);
		}
		panels.remove_at(i);
		current = panels.is_empty() ? -1 : 0;
		_show_only_current();
		_update_tabs();
		return released;
	}
	return nullptr;
}

void EditorPane::set_closable(bool p_closable) {
	close_button->set_visible(p_closable);
}

void EditorPane::set_header_visible(bool p_visible) {
	// A pane holding more than one panel always needs its tabs, however few
	// panes there are.
	header->set_visible(p_visible || panels.size() > 1);
}

EditorPane::EditorPane() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);
	_build_header();
}
