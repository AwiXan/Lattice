/**************************************************************************/
/*  canvas_item_editor_chrome.cpp                                         */
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

#include "canvas_item_editor_chrome.h"

#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/gui/button.h"
#include "scene/gui/container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"

// The grid as the 2D view's Layout menu lays the presets out, and their
// icons.
static const struct {
	Control::LayoutPreset preset;
	const char *icon;
	const char *name;
} anchor_presets[16] = {
	{ Control::PRESET_TOP_LEFT, "ControlAlignTopLeft", TTRC("Top Left") },
	{ Control::PRESET_CENTER_TOP, "ControlAlignCenterTop", TTRC("Center Top") },
	{ Control::PRESET_TOP_RIGHT, "ControlAlignTopRight", TTRC("Top Right") },
	{ Control::PRESET_TOP_WIDE, "ControlAlignTopWide", TTRC("Top Wide") },
	{ Control::PRESET_CENTER_LEFT, "ControlAlignCenterLeft", TTRC("Center Left") },
	{ Control::PRESET_CENTER, "ControlAlignCenter", TTRC("Center") },
	{ Control::PRESET_CENTER_RIGHT, "ControlAlignCenterRight", TTRC("Center Right") },
	{ Control::PRESET_HCENTER_WIDE, "ControlAlignHCenterWide", TTRC("HCenter Wide") },
	{ Control::PRESET_BOTTOM_LEFT, "ControlAlignBottomLeft", TTRC("Bottom Left") },
	{ Control::PRESET_CENTER_BOTTOM, "ControlAlignCenterBottom", TTRC("Center Bottom") },
	{ Control::PRESET_BOTTOM_RIGHT, "ControlAlignBottomRight", TTRC("Bottom Right") },
	{ Control::PRESET_BOTTOM_WIDE, "ControlAlignBottomWide", TTRC("Bottom Wide") },
	{ Control::PRESET_LEFT_WIDE, "ControlAlignLeftWide", TTRC("Left Wide") },
	{ Control::PRESET_VCENTER_WIDE, "ControlAlignVCenterWide", TTRC("VCenter Wide") },
	{ Control::PRESET_RIGHT_WIDE, "ControlAlignRightWide", TTRC("Right Wide") },
	{ Control::PRESET_FULL_RECT, "ControlAlignFullRect", TTRC("Full Rect") },
};

int CanvasItemEditorItemPanel::get_anchor_preset(int p_index) {
	ERR_FAIL_INDEX_V(p_index, 16, -1);
	return anchor_presets[p_index].preset;
}

void CanvasItemEditorItemPanel::_anchors_pressed(int p_preset) {
	const Vector<CanvasItem *> items = _edited_items();
	if (items.is_empty()) {
		return;
	}
	// As the Layout menu does it.
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Change Anchors, Offsets, Grow Direction"), UndoRedo::MERGE_DISABLE, items[0]);
	for (CanvasItem *item : items) {
		Control *control = Object::cast_to<Control>(item);
		if (!control || Object::cast_to<Container>(control->get_parent())) {
			continue;
		}
		undo_redo->add_do_property(control, "layout_mode", 1); // Anchors.
		undo_redo->add_do_property(control, "anchors_preset", p_preset);
		undo_redo->add_undo_method(control, "_edit_set_state", control->_edit_get_state());
	}
	undo_redo->commit_action();
	refresh();
}

Vector<CanvasItem *> CanvasItemEditorItemPanel::_edited_items() const {
	Vector<CanvasItem *> items;
	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	for (Node *node : selection->get_top_selected_node_list()) {
		CanvasItem *item = Object::cast_to<CanvasItem>(node);
		if (item && item->is_inside_tree() && (Object::cast_to<Node2D>(item) || Object::cast_to<Control>(item))) {
			items.push_back(item);
		}
	}
	return items;
}

bool CanvasItemEditorItemPanel::_has_row(const CanvasItem *p_item, int p_row) {
	return p_row != ROW_SIZE || Object::cast_to<Control>(p_item);
}

Vector2 CanvasItemEditorItemPanel::_read(const CanvasItem *p_item, int p_row) {
	if (const Node2D *node = Object::cast_to<Node2D>(p_item)) {
		switch (p_row) {
			case ROW_POSITION:
				return node->get_position();
			case ROW_ROTATION:
				return Vector2(node->get_rotation_degrees(), 0);
			case ROW_SCALE:
				return node->get_scale();
			default:
				return Vector2();
		}
	}
	if (const Control *control = Object::cast_to<Control>(p_item)) {
		switch (p_row) {
			case ROW_POSITION:
				return control->get_position();
			case ROW_ROTATION:
				return Vector2(control->get_rotation_degrees(), 0);
			case ROW_SCALE:
				return control->get_scale();
			case ROW_SIZE:
				return control->get_size();
			default:
				return Vector2();
		}
	}
	return Vector2();
}

const char *CanvasItemEditorItemPanel::_setter(int p_row) {
	static const char *setters[ROW_MAX] = { "set_position", "set_rotation_degrees", "set_scale", "set_size" };
	return setters[p_row];
}

void CanvasItemEditorItemPanel::_field_changed(double p_value, int p_row, int p_axis) {
	const Vector<CanvasItem *> items = _edited_items();
	if (items.is_empty()) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Transform"), UndoRedo::MERGE_ENDS, items[0]);
	for (CanvasItem *item : items) {
		if (!_has_row(item, p_row)) {
			continue;
		}
		const Vector2 old_value = _read(item, p_row);
		if (p_row == ROW_ROTATION) {
			undo_redo->add_do_method(item, _setter(p_row), p_value);
			undo_redo->add_undo_method(item, _setter(p_row), old_value.x);
			continue;
		}
		Vector2 new_value = old_value;
		new_value[p_axis] = p_value;
		undo_redo->add_do_method(item, _setter(p_row), new_value);
		undo_redo->add_undo_method(item, _setter(p_row), old_value);
	}
	undo_redo->commit_action();
}

void CanvasItemEditorItemPanel::refresh() {
	since_refresh = 0.0;
	const Vector<CanvasItem *> items = _edited_items();
	nothing->set_visible(items.is_empty());
	rows_box->set_visible(!items.is_empty());
	if (items.is_empty()) {
		return;
	}
	const CanvasItem *first = items[0];
	// Anchors for a Control a container does not place.
	const Control *first_control = Object::cast_to<Control>(first);
	const bool contained = first_control && Object::cast_to<Container>(first_control->get_parent());
	anchors_box->set_visible(first_control != nullptr);
	anchors->set_visible(first_control && !contained);
	anchors_note->set_visible(contained);
	title->set_text(items.size() == 1 ? String(first->get_name()) : vformat(TTR("%d Nodes"), items.size()));
	type->set_text(items.size() == 1 ? first->get_class() : String());
	for (int row = 0; row < ROW_MAX; row++) {
		row_boxes[row]->set_visible(_has_row(first, row));
		const Vector2 value = _read(first, row);
		for (int axis = 0; axis < 2; axis++) {
			EditorSpinSlider *field = fields[row][axis];
			if (!field || field->has_focus() || field->is_grabbing()) {
				continue;
			}
			field->set_value_no_signal(value[axis]);
		}
	}
}

EditorSpinSlider *CanvasItemEditorItemPanel::get_field(Row p_row, int p_axis) const {
	ERR_FAIL_INDEX_V(p_row, ROW_MAX, nullptr);
	ERR_FAIL_INDEX_V(p_axis, 2, nullptr);
	return fields[p_row][p_axis];
}

void CanvasItemEditorItemPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
			if (!selection->is_connected("selection_changed", callable_mp(this, &CanvasItemEditorItemPanel::refresh))) {
				selection->connect("selection_changed", callable_mp(this, &CanvasItemEditorItemPanel::refresh));
			}
			refresh();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			EditorSelection *selection = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_selection() : nullptr;
			if (selection && selection->is_connected("selection_changed", callable_mp(this, &CanvasItemEditorItemPanel::refresh))) {
				selection->disconnect("selection_changed", callable_mp(this, &CanvasItemEditorItemPanel::refresh));
			}
		} break;

		case NOTIFICATION_PROCESS: {
			// Items move by the manipulator, the Inspector and undo too, and
			// none of them tells anyone in particular.
			if (!is_visible_in_tree()) {
				return;
			}
			since_refresh += get_process_delta_time();
			if (since_refresh > 0.1) {
				refresh();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			const StringName colors[2] = { SNAME("property_color_x"), SNAME("property_color_y") };
			for (int row = 0; row < ROW_MAX; row++) {
				for (int axis = 0; axis < 2; axis++) {
					if (fields[row][axis]) {
						fields[row][axis]->add_theme_color_override("label_color", get_theme_color(colors[axis], EditorStringName(Editor)));
					}
				}
			}
			type->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("readonly_font_color"), EditorStringName(Editor)));
			for (int i = 0; i < 16; i++) {
				anchor_buttons[i]->set_button_icon(get_editor_theme_icon(anchor_presets[i].icon));
			}
			anchors_note->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("readonly_font_color"), EditorStringName(Editor)));
		} break;
	}
}

CanvasItemEditorItemPanel::CanvasItemEditorItemPanel() {
	set_name("Item");
	set_process(true);

	HBoxContainer *head = memnew(HBoxContainer);
	add_child(head);
	title = memnew(Label);
	title->set_theme_type_variation("HeaderSmall");
	title->set_clip_text(true);
	title->set_h_size_flags(SIZE_EXPAND_FILL);
	head->add_child(title);
	type = memnew(Label);
	head->add_child(type);

	nothing = memnew(Label);
	nothing->set_text(TTRC("Select a Node2D or a Control to see and type in where it is, how it is turned and how big it is."));
	nothing->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	nothing->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	add_child(nothing);

	rows_box = memnew(VBoxContainer);
	add_child(rows_box);

	const char *row_names[ROW_MAX] = { TTRC("Position"), TTRC("Rotation"), TTRC("Scale"), TTRC("Size") };
	const char *suffixes[ROW_MAX] = { "px", "°", "", "px" };
	const double steps[ROW_MAX] = { 1.0, 0.1, 0.001, 1.0 };
	const char *axes[ROW_MAX][2] = { { "x", "y" }, { "", nullptr }, { "x", "y" }, { "w", "h" } };
	for (int row = 0; row < ROW_MAX; row++) {
		VBoxContainer *row_box = memnew(VBoxContainer);
		row_box->add_theme_constant_override("separation", 0);
		rows_box->add_child(row_box);
		row_boxes[row] = row_box;

		Label *name = memnew(Label);
		name->set_text(row_names[row]);
		row_box->add_child(name);

		HBoxContainer *line = memnew(HBoxContainer);
		row_box->add_child(line);
		for (int axis = 0; axis < 2; axis++) {
			if (!axes[row][axis]) {
				continue;
			}
			EditorSpinSlider *field = memnew(EditorSpinSlider);
			field->set_label(axes[row][axis]);
			field->set_accessibility_name(vformat("%s %s", row_names[row], String(axes[row][axis]).to_upper()));
			field->set_flat(true);
			field->set_hide_slider(true);
			field->set_min(row == ROW_SIZE ? 0 : -100000);
			field->set_max(100000);
			field->set_allow_greater(true);
			field->set_allow_lesser(row != ROW_SIZE);
			field->set_step(steps[row]);
			field->set_suffix(String::utf8(suffixes[row]));
			field->set_h_size_flags(SIZE_EXPAND_FILL);
			field->connect(SceneStringName(value_changed), callable_mp(this, &CanvasItemEditorItemPanel::_field_changed).bind(row, axis));
			line->add_child(field);
			fields[row][axis] = field;
		}
	}

	VBoxContainer *anchors_vbox = memnew(VBoxContainer);
	anchors_vbox->add_theme_constant_override("separation", 0);
	rows_box->add_child(anchors_vbox);
	anchors_box = anchors_vbox;
	Label *anchors_label = memnew(Label);
	anchors_label->set_text(TTRC("Anchors"));
	anchors_vbox->add_child(anchors_label);
	anchors = memnew(GridContainer);
	anchors->set_columns(4);
	anchors_vbox->add_child(anchors);
	for (int i = 0; i < 16; i++) {
		Button *button = memnew(Button);
		button->set_flat(true);
		button->set_tooltip_text(anchor_presets[i].name);
		button->set_accessibility_name(anchor_presets[i].name);
		button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditorItemPanel::_anchors_pressed).bind((int)anchor_presets[i].preset));
		anchors->add_child(button);
		anchor_buttons[i] = button;
	}
	anchors_note = memnew(Label);
	anchors_note->set_text(TTRC("Placed by the container it is in."));
	anchors_vbox->add_child(anchors_note);
}
