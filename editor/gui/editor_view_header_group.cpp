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
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/resources/style_box_flat.h"

Ref<StyleBoxFlat> EditorViewHeaderGroup::make_style(const Control *p_for) {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	// A shade lighter than the bar it sits in, and a line lighter still
	// around it: set apart without a colour of its own, so it stays the
	// theme's, light or dark.
	const Color base = p_for->get_theme_color(SNAME("base_color"), EditorStringName(Editor));
	const Color mono = p_for->get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	style->set_bg_color(base.lerp(mono, 0.08));
	style->set_border_color(base.lerp(mono, 0.22));
	style->set_border_width_all(MAX(1, (int)Math::round(EDSCALE)));
	const int radius = MAX(3, (int)Math::round((int)EDITOR_GET("interface/theme/corner_radius") * EDSCALE * 1.5));
	style->set_corner_radius_all(radius);
	style->set_content_margin(SIDE_LEFT, 4 * EDSCALE);
	style->set_content_margin(SIDE_RIGHT, 4 * EDSCALE);
	style->set_content_margin(SIDE_TOP, 2 * EDSCALE);
	style->set_content_margin(SIDE_BOTTOM, 2 * EDSCALE);
	return style;
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
		add_theme_style_override(SceneStringName(panel), make_style(this));
		theming = false;
	}
}

EditorViewHeaderGroup::EditorViewHeaderGroup() {
	box = memnew(HBoxContainer);
	box->add_theme_constant_override("separation", 2 * EDSCALE);
	add_child(box);
}
