/**************************************************************************/
/*  node_3d_editor_chrome.cpp                                             */
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

#include "node_3d_editor_chrome.h"

#include "core/io/image.h"
#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/gui/label.h"
#include "scene/resources/image_texture.h"

Ref<Texture2D> Node3DEditorChrome::make_shading_icon(Shading p_shading, int p_size, const Color &p_ink, const Color &p_accent) {
	const int size = MAX(p_size, 8);
	Ref<Image> image = Image::create_empty(size, size, false, Image::FORMAT_RGBA8);

	const real_t center = size * 0.5;
	const real_t radius = size * 0.5 - 1.0;
	// Lit from the upper left and a little in front, as icons usually are.
	const Vector3 light = Vector3(-0.5, -0.6, 0.62).normalized();
	const Vector3 half = (light + Vector3(0, 0, 1)).normalized();

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			const real_t dx = x + 0.5 - center;
			const real_t dy = y + 0.5 - center;
			const real_t distance = Math::sqrt(dx * dx + dy * dy);
			// How much of this pixel the disc covers, for smooth edges.
			const real_t disc = CLAMP(radius + 0.5 - distance, 0.0, 1.0);
			if (disc <= 0.0) {
				image->set_pixel(x, y, Color(0, 0, 0, 0));
				continue;
			}

			Color color;
			real_t alpha = disc;
			switch (p_shading) {
				case SHADING_WIREFRAME: {
					// The outline, a meridian and the equator.
					const real_t outline = CLAMP(1.4 - Math::abs(distance - (radius - 0.7)), 0.0, 1.0);
					const real_t ellipse_x = dx / (radius * 0.42);
					const real_t ellipse_y = dy / radius;
					const real_t meridian = CLAMP(1.0 - Math::abs(Math::sqrt(ellipse_x * ellipse_x + ellipse_y * ellipse_y) - 1.0) * radius * 0.42 * 1.4, 0.0, 1.0);
					const real_t equator = CLAMP(1.0 - Math::abs(dy) * 1.4, 0.0, 1.0);
					color = p_ink;
					alpha = disc * MAX(outline, MAX(meridian, equator) * 0.8);
				} break;
				case SHADING_UNSHADED: {
					color = p_ink;
					alpha = disc * 0.85;
				} break;
				case SHADING_LIGHTING:
				case SHADING_NORMAL: {
					const real_t nx = dx / radius;
					const real_t ny = dy / radius;
					const Vector3 normal(nx, ny, Math::sqrt(MAX(0.0, 1.0 - nx * nx - ny * ny)));
					const real_t diffuse = MAX(0.0, normal.dot(light));
					if (p_shading == SHADING_LIGHTING) {
						color = p_ink * (0.22 + 0.78 * diffuse);
					} else {
						const real_t facing = MAX((real_t)0.0, normal.dot(half));
						const real_t specular = Math::pow(facing, (real_t)24.0);
						color = p_accent * (0.3 + 0.7 * diffuse);
						color = color.lerp(Color(1, 1, 1), specular * 0.7);
					}
				} break;
				default: {
				} break;
			}
			color.a = alpha;
			image->set_pixel(x, y, color);
		}
	}
	return ImageTexture::create_from_image(image);
}

// ------------------------------------------------------------ the Item page

Vector<Node3D *> Node3DEditorItemPanel::_edited_nodes() const {
	Vector<Node3D *> nodes;
	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	for (Node *node : selection->get_top_selected_node_list()) {
		Node3D *node_3d = Object::cast_to<Node3D>(node);
		if (node_3d && node_3d->is_inside_tree()) {
			nodes.push_back(node_3d);
		}
	}
	return nodes;
}

Vector3 Node3DEditorItemPanel::_read(const Node3D *p_node, int p_row) {
	switch (p_row) {
		case ROW_POSITION: {
			return p_node->get_position();
		}
		case ROW_ROTATION: {
			return p_node->get_rotation_degrees();
		}
		case ROW_SCALE: {
			return p_node->get_scale();
		}
		case ROW_SIZE: {
			const VisualInstance3D *visual = Object::cast_to<VisualInstance3D>(p_node);
			return visual ? visual->get_aabb().size * p_node->get_scale().abs() : Vector3();
		}
	}
	return Vector3();
}

void Node3DEditorItemPanel::_field_changed(double p_value, int p_row, int p_axis) {
	const Vector<Node3D *> nodes = _edited_nodes();
	if (nodes.is_empty()) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	// One step to undo for a whole drag of the field, not one per frame.
	undo_redo->create_action(TTR("Set Transform"), UndoRedo::MERGE_ENDS, nodes[0]);
	for (Node3D *node : nodes) {
		if (p_row == ROW_SIZE) {
			const VisualInstance3D *visual = Object::cast_to<VisualInstance3D>(node);
			const real_t drawn = visual ? visual->get_aabb().size[p_axis] : 0.0;
			if (drawn <= CMP_EPSILON) {
				// Flat along this axis, or not drawn at all: no scale makes it
				// this size.
				continue;
			}
			const Vector3 scale = node->get_scale();
			Vector3 new_scale = scale;
			new_scale[p_axis] = SIGN(scale[p_axis] == 0 ? 1 : scale[p_axis]) * p_value / drawn;
			undo_redo->add_do_method(node, "set_scale", new_scale);
			undo_redo->add_undo_method(node, "set_scale", scale);
			continue;
		}
		static const char *setters[] = { "set_position", "set_rotation_degrees", "set_scale" };
		const Vector3 old_value = _read(node, p_row);
		Vector3 new_value = old_value;
		new_value[p_axis] = p_value;
		undo_redo->add_do_method(node, setters[p_row], new_value);
		undo_redo->add_undo_method(node, setters[p_row], old_value);
	}
	undo_redo->commit_action();
}

void Node3DEditorItemPanel::refresh() {
	since_refresh = 0.0;
	const Vector<Node3D *> nodes = _edited_nodes();
	nothing->set_visible(nodes.is_empty());
	rows_box->set_visible(!nodes.is_empty());
	if (nodes.is_empty()) {
		return;
	}
	const Node3D *first = nodes[0];
	title->set_text(nodes.size() == 1 ? String(first->get_name()) : vformat(TTR("%d Nodes"), nodes.size()));
	type->set_text(nodes.size() == 1 ? first->get_class() : String());
	row_boxes[ROW_SIZE]->set_visible(Object::cast_to<VisualInstance3D>(first) != nullptr);

	for (int row = 0; row < ROW_MAX; row++) {
		const Vector3 value = _read(first, row);
		for (int axis = 0; axis < 3; axis++) {
			EditorSpinSlider *field = fields[row][axis];
			// Not under the hands of someone typing or dragging it.
			if (field->has_focus() || field->is_grabbing()) {
				continue;
			}
			field->set_value_no_signal(value[axis]);
		}
	}
}

EditorSpinSlider *Node3DEditorItemPanel::get_field(Row p_row, int p_axis) const {
	ERR_FAIL_INDEX_V(p_row, ROW_MAX, nullptr);
	ERR_FAIL_INDEX_V(p_axis, 3, nullptr);
	return fields[p_row][p_axis];
}

void Node3DEditorItemPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
			if (!selection->is_connected("selection_changed", callable_mp(this, &Node3DEditorItemPanel::refresh))) {
				selection->connect("selection_changed", callable_mp(this, &Node3DEditorItemPanel::refresh));
			}
			refresh();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			EditorSelection *selection = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_selection() : nullptr;
			if (selection && selection->is_connected("selection_changed", callable_mp(this, &Node3DEditorItemPanel::refresh))) {
				selection->disconnect("selection_changed", callable_mp(this, &Node3DEditorItemPanel::refresh));
			}
		} break;

		case NOTIFICATION_PROCESS: {
			// Nodes move by other routes too - the manipulator, the Inspector,
			// undo - and none of them says so to anyone in particular. A few
			// times a second is enough to follow them, and only while shown.
			if (!is_visible_in_tree()) {
				return;
			}
			since_refresh += get_process_delta_time();
			if (since_refresh > 0.1) {
				refresh();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			const StringName colors[3] = { SNAME("property_color_x"), SNAME("property_color_y"), SNAME("property_color_z") };
			for (int row = 0; row < ROW_MAX; row++) {
				for (int axis = 0; axis < 3; axis++) {
					fields[row][axis]->add_theme_color_override("label_color", get_theme_color(colors[axis], EditorStringName(Editor)));
				}
			}
			type->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("readonly_font_color"), EditorStringName(Editor)));
		} break;
	}
}

Node3DEditorItemPanel::Node3DEditorItemPanel() {
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
	nothing->set_text(TTRC("Select a 3D node to see and type in where it is, how it is turned and how big it is."));
	nothing->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	nothing->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	add_child(nothing);

	rows_box = memnew(VBoxContainer);
	add_child(rows_box);

	const char *row_names[ROW_MAX] = { TTRC("Position"), TTRC("Rotation"), TTRC("Scale"), TTRC("Size") };
	const char *suffixes[ROW_MAX] = { "m", "°", "", "m" };
	const double default_step = EDITOR_GET("interface/inspector/default_float_step");
	const double steps[ROW_MAX] = { default_step, 0.1, 0.001, default_step };
	const char *axes[3] = { "x", "y", "z" };
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
		for (int axis = 0; axis < 3; axis++) {
			EditorSpinSlider *field = memnew(EditorSpinSlider);
			field->set_label(axes[axis]);
			field->set_accessibility_name(vformat("%s %s", row_names[row], String(axes[axis]).to_upper()));
			field->set_flat(true);
			field->set_hide_slider(true);
			field->set_min(-100000);
			field->set_max(100000);
			field->set_allow_greater(true);
			field->set_allow_lesser(true);
			field->set_step(steps[row]);
			field->set_suffix(String::utf8(suffixes[row]));
			field->set_h_size_flags(SIZE_EXPAND_FILL);
			field->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditorItemPanel::_field_changed).bind(row, axis));
			line->add_child(field);
			fields[row][axis] = field;
		}
	}
}
