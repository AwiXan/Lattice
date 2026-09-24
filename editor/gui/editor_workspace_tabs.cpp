/**************************************************************************/
/*  editor_workspace_tabs.cpp                                             */
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

#include "editor_workspace_tabs.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/popup_menu.h"
#include "scene/resources/style_box_flat.h"

enum {
	CONTEXT_SAVE_HERE,
	CONTEXT_DELETE,
};

void EditorWorkspaceTabs::_bind_methods() {
	ADD_SIGNAL(MethodInfo("switch_requested", PropertyInfo(Variant::STRING, "name")));
	ADD_SIGNAL(MethodInfo("save_new_requested"));
	ADD_SIGNAL(MethodInfo("save_requested", PropertyInfo(Variant::STRING, "name")));
	ADD_SIGNAL(MethodInfo("delete_requested", PropertyInfo(Variant::STRING, "name")));
}

void EditorWorkspaceTabs::set_workspaces(const Vector<String> &p_names, const String &p_current) {
	if (p_names == names && p_current == current) {
		return;
	}
	names = p_names;
	current = p_current;
	while (tabs->get_child_count() > 0) {
		Node *tab = tabs->get_child(0);
		tabs->remove_child(tab);
		// Later: it can be the tab whose press is being answered.
		tab->queue_free();
	}
	for (int i = 0; i < names.size(); i++) {
		const String &name = names[i];
		Button *tab = memnew(Button);
		tab->set_text(name);
		tab->set_toggle_mode(true);
		tab->set_button_group(group);
		tab->set_focus_mode(FOCUS_ACCESSIBILITY);
		tab->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		String tooltip = TTR("Switch to this workspace; the one left keeps what was changed in it.\nRight click to save the arrangement there is into it, or to delete it.");
		const String shortcut_path = "layout/switch_" + itos(i + 1);
		if (i < 9 && EditorSettings::get_singleton()->has_shortcut(shortcut_path)) {
			tooltip += "\n" + EditorSettings::get_singleton()->get_shortcut(shortcut_path)->get_as_text();
		}
		tab->set_tooltip_text(tooltip);
		tab->set_pressed_no_signal(name == current);
		tab->connect(SceneStringName(pressed), callable_mp(this, &EditorWorkspaceTabs::_tab_pressed).bind(name));
		tab->connect(SceneStringName(gui_input), callable_mp(this, &EditorWorkspaceTabs::_tab_input).bind(name));
		tabs->add_child(tab);
		if (is_inside_tree()) {
			_style_tab(tab);
		}
	}
	add_button->set_text(names.is_empty() ? TTR("Workspace") : String());
}

Button *EditorWorkspaceTabs::get_tab(const String &p_name) const {
	for (int i = 0; i < tabs->get_child_count(); i++) {
		Button *tab = Object::cast_to<Button>(tabs->get_child(i));
		if (tab && tab->get_text() == p_name) {
			return tab;
		}
	}
	return nullptr;
}

void EditorWorkspaceTabs::_tab_pressed(const String &p_name) {
	if (p_name != current) {
		emit_signal(SNAME("switch_requested"), p_name);
	}
}

void EditorWorkspaceTabs::_add_pressed() {
	emit_signal(SNAME("save_new_requested"));
}

void EditorWorkspaceTabs::_tab_input(const Ref<InputEvent> &p_event, const String &p_name) {
	Ref<InputEventMouseButton> click = p_event;
	if (click.is_null() || !click->is_pressed() || click->get_button_index() != MouseButton::RIGHT) {
		return;
	}
	context_name = p_name;
	context_menu->clear();
	context_menu->add_item(vformat(TTR("Save the Arrangement There Is into \"%s\""), p_name), CONTEXT_SAVE_HERE);
	context_menu->add_separator();
	context_menu->add_item(vformat(TTR("Delete \"%s\"..."), p_name), CONTEXT_DELETE);
	context_menu->set_position(get_screen_transform().xform(get_local_mouse_position()));
	context_menu->reset_size();
	context_menu->popup();
	accept_event();
}

void EditorWorkspaceTabs::_context_pressed(int p_id) {
	switch (p_id) {
		case CONTEXT_SAVE_HERE: {
			emit_signal(SNAME("save_requested"), context_name);
		} break;
		case CONTEXT_DELETE: {
			delete_confirmation->set_text(vformat(TTR("Delete the workspace \"%s\"? The arrangement there is stays as it is."), context_name));
			delete_confirmation->popup_centered();
		} break;
	}
}

void EditorWorkspaceTabs::_delete_confirmed() {
	emit_signal(SNAME("delete_requested"), context_name);
}

void EditorWorkspaceTabs::_style_tab(Button *p_tab) {
	// Flat, lit under the mouse; the one in use raised a little and marked
	// in the accent colour along its foot.
	const Color base = get_theme_color(SNAME("base_color"), EditorStringName(Editor));
	const Color mono = get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	const int radius = MAX(3, (int)Math::round((int)EDITOR_GET("interface/theme/corner_radius") * EDSCALE));
	auto box = [&](const Color &p_color, bool p_marked) {
		Ref<StyleBoxFlat> style;
		style.instantiate();
		style->set_bg_color(p_color);
		style->set_corner_radius(CORNER_TOP_LEFT, radius);
		style->set_corner_radius(CORNER_TOP_RIGHT, radius);
		style->set_content_margin(SIDE_LEFT, 10 * EDSCALE);
		style->set_content_margin(SIDE_RIGHT, 10 * EDSCALE);
		style->set_content_margin(SIDE_TOP, 3 * EDSCALE);
		style->set_content_margin(SIDE_BOTTOM, 3 * EDSCALE);
		if (p_marked) {
			style->set_border_width(SIDE_BOTTOM, MAX(2, (int)Math::round(2 * EDSCALE)));
			style->set_border_color(accent);
		}
		return style;
	};
	p_tab->add_theme_style_override(SNAME("normal"), box(Color(0, 0, 0, 0), false));
	p_tab->add_theme_style_override(SNAME("hover"), box(base.lerp(mono, 0.1), false));
	p_tab->add_theme_style_override(SNAME("pressed"), box(base.lerp(mono, 0.14), true));
	p_tab->add_theme_style_override(SNAME("hover_pressed"), box(base.lerp(mono, 0.18), true));
	Ref<StyleBoxEmpty> no_focus;
	no_focus.instantiate();
	p_tab->add_theme_style_override(SNAME("focus"), no_focus);
	p_tab->add_theme_color_override(SNAME("font_color"), get_theme_color(SNAME("font_color"), SNAME("Button")) * Color(1, 1, 1, 0.7));
	p_tab->add_theme_color_override(SNAME("font_pressed_color"), get_theme_color(SNAME("font_color"), SNAME("Button")));
	p_tab->add_theme_color_override(SNAME("font_hover_pressed_color"), get_theme_color(SNAME("font_color"), SNAME("Button")));
}

void EditorWorkspaceTabs::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED && !theming) {
		theming = true;
		for (int i = 0; i < tabs->get_child_count(); i++) {
			Button *tab = Object::cast_to<Button>(tabs->get_child(i));
			if (tab) {
				_style_tab(tab);
			}
		}
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		theming = false;
	}
}

EditorWorkspaceTabs::EditorWorkspaceTabs() {
	set_name("WorkspaceTabs");
	add_theme_constant_override("separation", 2 * EDSCALE);
	group.instantiate();

	tabs = memnew(HBoxContainer);
	tabs->add_theme_constant_override("separation", 1 * EDSCALE);
	add_child(tabs);

	add_button = memnew(Button);
	add_button->set_flat(true);
	add_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	add_button->set_tooltip_text(TTRC("Save the arrangement there is - panes, panels, windows - as a new workspace."));
	add_button->set_accessibility_name(TTRC("New Workspace"));
	add_button->connect(SceneStringName(pressed), callable_mp(this, &EditorWorkspaceTabs::_add_pressed));
	add_child(add_button);

	context_menu = memnew(PopupMenu);
	context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorWorkspaceTabs::_context_pressed));
	add_child(context_menu);

	delete_confirmation = memnew(ConfirmationDialog);
	delete_confirmation->set_title(TTRC("Delete Workspace"));
	delete_confirmation->connect(SceneStringName(confirmed), callable_mp(this, &EditorWorkspaceTabs::_delete_confirmed));
	add_child(delete_confirmation);
}
