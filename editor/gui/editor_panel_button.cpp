/**************************************************************************/
/*  editor_panel_button.cpp                                               */
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

#include "editor_panel_button.h"

#include "editor/editor_panel_registry.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/texture_rect.h"

void EditorPanelButton::_bind_methods() {
}

Variant EditorPanelButton::get_drag_data(const Point2 &p_point) {
	if (panel_type == StringName() || !EditorPanelRegistry::has_type(panel_type)) {
		return Variant();
	}

	// The same thing the button says, in the shape a pane reads. See
	// EditorPane::PanelDrop.
	Dictionary data;
	data["editor_panel"] = String(panel_type);

	// Something to see while it is in the air: what the button looks like, so
	// it is plainly the same thing being carried.
	Panel *preview = memnew(Panel);
	HBoxContainer *row = memnew(HBoxContainer);
	preview->add_child(row);
	row->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	if (get_button_icon().is_valid()) {
		TextureRect *icon = memnew(TextureRect);
		icon->set_texture(get_button_icon());
		icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		icon->set_custom_minimum_size(get_button_icon()->get_size());
		row->add_child(icon);
	}
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(panel_type);
	Label *label = memnew(Label(type && !type->title.is_empty() ? type->title : String(panel_type)));
	row->add_child(label);
	set_drag_preview(preview);

	return data;
}

EditorPanelButton::EditorPanelButton() {
	set_flat(true);
	set_focus_mode(FOCUS_NONE);
	// A press is a shortcut for a drag onto this very pane, so it has to happen
	// on release - otherwise the press would fire before the drag could start.
	set_action_mode(ACTION_MODE_BUTTON_RELEASE);
}
