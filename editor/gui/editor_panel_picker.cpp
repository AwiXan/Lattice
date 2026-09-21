/**************************************************************************/
/*  editor_panel_picker.cpp                                               */
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

#include "editor_panel_picker.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"

EditorPanelPicker *EditorPanelPicker::get_shared() {
	static ObjectID shared;
	EditorPanelPicker *picker = ObjectDB::get_instance<EditorPanelPicker>(shared);
	if (picker) {
		return picker;
	}
	picker = memnew(EditorPanelPicker);
	EditorNode::get_singleton()->get_gui_base()->add_child(picker);
	shared = picker->get_instance_id();
	return picker;
}

void EditorPanelPicker::pick(const Callable &p_chosen) {
	chosen = p_chosen;
	filter->clear();
	_update_list();
	popup_centered_clamped(Size2(560, 420) * EDSCALE, 0.8);
	filter->grab_focus();
}

void EditorPanelPicker::_update_list() {
	list->clear();
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	const String query = filter->get_text().strip_edges().to_lower();

	// Views first - what a pane is usually for - then everything else, each
	// group in the order a reader would look for a name in.
	struct Entry {
		StringName id;
		String title;
		Ref<Texture2D> icon;
		int group = 0;
	};
	Vector<Entry> entries;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		if (!type || type->binding == EditorPanelRegistry::BINDING_RESOURCE) {
			// A panel showing a resource is opened by that resource.
			continue;
		}
		Entry e;
		e.id = id;
		e.title = type->title.is_empty() ? String(id) : type->title;
		if (!query.is_empty() && !e.title.to_lower().contains(query)) {
			continue;
		}
		if (base && type->icon != StringName() && base->has_theme_icon(type->icon, EditorStringName(EditorIcons))) {
			e.icon = base->get_editor_theme_icon(type->icon);
		} else {
			e.icon = type->icon_texture;
		}
		e.group = type->featured ? 0 : (type->binding == EditorPanelRegistry::BINDING_DOCUMENT ? 1 : (type->lent ? 3 : 2));
		entries.push_back(e);
	}
	struct ByGroupThenTitle {
		bool operator()(const Entry &a, const Entry &b) const {
			if (a.group != b.group) {
				return a.group < b.group;
			}
			return a.title.naturalnocasecmp_to(b.title) < 0;
		}
	};
	entries.sort_custom<ByGroupThenTitle>();

	for (const Entry &e : entries) {
		const int index = list->add_item(e.title, e.icon);
		list->set_item_metadata(index, String(e.id));
	}
	if (list->get_item_count() > 0) {
		list->select(0);
	}
	get_ok_button()->set_disabled(list->get_item_count() == 0);
}

void EditorPanelPicker::_filter_changed(const String &p_text) {
	_update_list();
}

void EditorPanelPicker::_filter_input(const Ref<InputEvent> &p_event) {
	// The arrows move through the list while typing, so a search ends in Enter
	// without reaching for the mouse.
	const Ref<InputEventKey> key = p_event;
	if (key.is_null() || !key->is_pressed() || list->get_item_count() == 0) {
		return;
	}
	const Vector<int> selected = list->get_selected_items();
	int at = selected.is_empty() ? 0 : selected[0];
	if (key->get_keycode() == Key::DOWN) {
		at = MIN(at + 1, list->get_item_count() - 1);
	} else if (key->get_keycode() == Key::UP) {
		at = MAX(at - 1, 0);
	} else {
		return;
	}
	list->select(at);
	list->ensure_current_is_visible();
	filter->accept_event();
}

void EditorPanelPicker::_item_activated(int p_index) {
	list->select(p_index);
	ok_pressed();
}

void EditorPanelPicker::ok_pressed() {
	const Vector<int> selected = list->get_selected_items();
	if (!selected.is_empty() && chosen.is_valid()) {
		const StringName id = StringName(String(list->get_item_metadata(selected[0])));
		chosen.call(id);
	}
	chosen = Callable();
	hide();
}

void EditorPanelPicker::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			filter->set_right_icon(get_editor_theme_icon(SNAME("Search")));
		} break;
	}
}

EditorPanelPicker::EditorPanelPicker() {
	set_title(TTRC("Add a Panel"));
	set_ok_button_text(TTRC("Add"));

	VBoxContainer *vb = memnew(VBoxContainer);
	add_child(vb);

	filter = memnew(LineEdit);
	filter->set_placeholder(TTRC("Search panels"));
	filter->set_clear_button_enabled(true);
	filter->connect(SceneStringName(text_changed), callable_mp(this, &EditorPanelPicker::_filter_changed));
	filter->connect(SceneStringName(gui_input), callable_mp(this, &EditorPanelPicker::_filter_input));
	vb->add_child(filter);
	register_text_enter(filter);

	list = memnew(ItemList);
	list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	// Tiles rather than rows: a panel is recognised by its icon first.
	list->set_icon_mode(ItemList::ICON_MODE_TOP);
	list->set_max_columns(0);
	list->set_same_column_width(true);
	list->set_fixed_column_width(110 * EDSCALE);
	list->set_fixed_icon_size(Size2(32, 32) * EDSCALE);
	list->set_max_text_lines(2);
	list->connect("item_activated", callable_mp(this, &EditorPanelPicker::_item_activated));
	vb->add_child(list);
}
