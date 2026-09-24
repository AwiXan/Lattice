/**************************************************************************/
/*  editor_view_header_group.cpp                                          */
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

#include "editor_view_header_group.h"

#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"

static const Control *_theme_source(const Control *p_for) {
	// A control not in the tree yet has no theme to ask - every colour comes
	// back black - but the editor's own does, always.
	if (p_for && p_for->is_inside_tree()) {
		return p_for;
	}
	return EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : p_for;
}

Ref<StyleBoxFlat> EditorViewHeaderGroup::make_style(const Control *p_for, bool p_over_view) {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	// A shade lighter than the bar it sits in, and a line lighter still
	// around it: set apart without a colour of its own, so it stays the
	// theme's, light or dark.
	const Control *theme = _theme_source(p_for);
	const Color base = theme->get_theme_color(SNAME("base_color"), EditorStringName(Editor));
	const Color mono = theme->get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	style->set_bg_color(base.lerp(mono, 0.08));
	style->set_border_color(base.lerp(mono, 0.22));
	style->set_border_width_all(MAX(1, (int)Math::round(EDSCALE)));
	const int radius = MAX(3, (int)Math::round((int)EDITOR_GET("interface/theme/corner_radius") * EDSCALE * 1.5));
	style->set_corner_radius_all(radius);
	style->set_content_margin(SIDE_LEFT, 4 * EDSCALE);
	style->set_content_margin(SIDE_RIGHT, 4 * EDSCALE);
	style->set_content_margin(SIDE_TOP, 2 * EDSCALE);
	style->set_content_margin(SIDE_BOTTOM, 2 * EDSCALE);
	if (p_over_view) {
		// Whatever the scene is - a bright sky, a dark cave - it reads the
		// same on it.
		Color bg = base.darkened(0.15);
		bg.a = 0.88;
		style->set_bg_color(bg);
		Color border = mono;
		border.a = 0.14;
		style->set_border_color(border);
		style->set_shadow_color(Color(0, 0, 0, 0.3));
		style->set_shadow_size(Math::round(5 * EDSCALE));
		style->set_shadow_offset(Vector2(0, 1) * EDSCALE);
		style->set_content_margin_all(2 * EDSCALE);
	}
	return style;
}

void EditorViewHeaderGroup::style_tool_button(Button *p_button) {
	ERR_FAIL_NULL(p_button);
	const Control *theme = _theme_source(p_button);
	const Color base = theme->get_theme_color(SNAME("base_color"), EditorStringName(Editor));
	const Color mono = theme->get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	const Color accent = theme->get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	const int radius = MAX(3, (int)Math::round((int)EDITOR_GET("interface/theme/corner_radius") * EDSCALE * 1.5));
	auto box = [&](const Color &p_color) {
		Ref<StyleBoxFlat> style;
		style.instantiate();
		style->set_bg_color(p_color);
		style->set_corner_radius_all(radius);
		style->set_content_margin_all(4 * EDSCALE);
		return style;
	};
	p_button->add_theme_style_override(SNAME("normal"), box(Color(0, 0, 0, 0)));
	p_button->add_theme_style_override(SNAME("hover"), box(base.lerp(mono, 0.16)));
	p_button->add_theme_style_override(SNAME("pressed"), box(accent.darkened(0.2)));
	p_button->add_theme_style_override(SNAME("hover_pressed"), box(accent.darkened(0.05)));
	p_button->add_theme_style_override(SNAME("disabled"), box(Color(0, 0, 0, 0)));
	Ref<StyleBoxEmpty> no_focus;
	no_focus.instantiate();
	p_button->add_theme_style_override(SNAME("focus"), no_focus);
	p_button->add_theme_color_override(SNAME("icon_pressed_color"), Color(1, 1, 1));
	p_button->add_theme_color_override(SNAME("icon_hover_pressed_color"), Color(1, 1, 1));
}

void EditorViewHeaderGroup::apply_style(PanelContainer *p_panel) {
	// Called by whoever owns the panel when its theme changes: setting a style
	// is a theme change too, so the panel cannot do it for itself from there.
	ERR_FAIL_NULL(p_panel);
	p_panel->add_theme_style_override(SceneStringName(panel), make_style(p_panel));
}

void EditorViewHeaderGroup::take(const Vector<Control *> &p_controls) {
	for (Control *control : p_controls) {
		if (!control) {
			continue;
		}
		if (control->get_parent()) {
			control->get_parent()->remove_child(control);
		}
		box->add_child(control);
	}
}

void EditorViewHeaderGroup::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED && !theming) {
		theming = true;
		add_theme_style_override(SceneStringName(panel), make_style(this, over_view));
		theming = false;
	}
}

EditorViewHeaderGroup::EditorViewHeaderGroup(bool p_vertical, bool p_over_view) {
	over_view = p_over_view;
	if (p_vertical) {
		box = memnew(VBoxContainer);
	} else {
		box = memnew(HBoxContainer);
	}
	box->add_theme_constant_override("separation", 2 * EDSCALE);
	add_child(box);
}
