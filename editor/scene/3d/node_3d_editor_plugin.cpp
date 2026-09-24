/**************************************************************************/
/*  node_3d_editor_plugin.cpp                                             */
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

#include "node_3d_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/projection.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/3d/gizmos/audio_listener_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/audio_stream_player_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/camera_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/chain_ik_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/cpu_particles_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/decal_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/fog_volume_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/geometry_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/gpu_particles_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/gpu_particles_collision_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/label_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/light_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/lightmap_gi_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/lightmap_probe_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/marker_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/mesh_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/occluder_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/particles_3d_emission_shape_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_object_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_polygon_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_shape_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/joint_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/physics_bone_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/ray_cast_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/shape_cast_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/soft_body_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/spring_arm_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/vehicle_body_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/reflection_probe_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/spring_bone_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/sprite_base_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/two_bone_ik_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/visible_on_screen_notifier_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/voxel_gi_gizmo_plugin.h"
#include "editor/gui/editor_button_mirror.h"
#include "editor/gui/editor_pie_menu.h"
#include "editor/gui/editor_view_header_group.h"
#include "editor/gui/editor_view_hints.h"
#include "editor/gui/editor_view_sidebar.h"
#include "editor/scene/3d/node_3d_editor_chrome.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "editor/settings/editor_settings.h"
#include "editor/translations/editor_translation_preview_button.h"
#include "editor/translations/editor_translation_preview_menu.h"
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/decal.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/sky.h"
#include "scene/resources/surface_tool.h"
#include "servers/rendering/rendering_server.h"

constexpr real_t GIZMO_ARROW_SIZE = 0.35;
constexpr real_t GIZMO_RING_HALF_WIDTH = 0.1;
constexpr real_t GIZMO_PLANE_SIZE = 0.2;
constexpr real_t GIZMO_PLANE_DST = 0.3;
constexpr real_t GIZMO_CIRCLE_SIZE = 1.1;

constexpr real_t GIZMO_SCALE_OFFSET = GIZMO_CIRCLE_SIZE + 0.3;
constexpr real_t GIZMO_ARROW_OFFSET = GIZMO_CIRCLE_SIZE + 0.3;

constexpr real_t TRACKBALL_SENSITIVITY = 0.005;
constexpr int TRACKBALL_SPHERE_RINGS = 16;
constexpr int TRACKBALL_SPHERE_SECTORS = 32;
constexpr real_t TRACKBALL_HIGHLIGHT_ALPHA = 0.01;
constexpr int GIZMO_HIGHLIGHT_AXIS_VIEW_ROTATION = 15;

constexpr float VERTEX_SNAP_THRESHOLD = 30.0f;
constexpr int GIZMO_HIGHLIGHT_AXIS_TRACKBALL = 16;

constexpr real_t ZOOM_FREELOOK_INDICATOR_DELAY_S = 1.5;

constexpr real_t MIN_Z = 0.01;
constexpr real_t MAX_Z = 1000000.0;

constexpr real_t MIN_FOV = 0.01;
constexpr real_t MAX_FOV = 179;

void ViewportNavigationControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (viewport != nullptr) {
				_draw();
				_update_navigation();
			}
		} break;

		case NOTIFICATION_MOUSE_ENTER: {
			hovered = true;
			queue_redraw();
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			hovered = false;
			queue_redraw();
		} break;
	}
}

void ViewportNavigationControl::_draw() {
	if (nav_mode == View3DController::NAV_MODE_NONE) {
		return;
	}

	Vector2 center = get_size() / 2.0;
	float radius = get_size().x / 2.0;

	const bool focused = focused_index != -1;
	draw_circle(center, radius, Color(0.5, 0.5, 0.5, focused || hovered ? 0.35 : 0.15));

	const Color c = focused ? Color(0.9, 0.9, 0.9, 0.9) : Color(0.5, 0.5, 0.5, 0.25);

	Vector2 circle_pos = focused ? center.move_toward(focused_pos, radius) : center;

	draw_circle(circle_pos, AXIS_CIRCLE_RADIUS, c);
	draw_circle(circle_pos, AXIS_CIRCLE_RADIUS * 0.8, c.darkened(0.4));
}

void ViewportNavigationControl::_process_click(int p_index, Vector2 p_position, bool p_pressed) {
	hovered = false;
	queue_redraw();

	if (focused_index != -1 && focused_index != p_index) {
		return;
	}
	if (p_pressed) {
		if (p_position.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
			focused_pos = p_position;
			focused_index = p_index;
			queue_redraw();
		}
	} else {
		focused_index = -1;
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(focused_mouse_start);
		}
	}
}

void ViewportNavigationControl::_process_drag(int p_index, Vector2 p_position, Vector2 p_relative_position) {
	if (focused_index == p_index) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_VISIBLE) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
			focused_mouse_start = p_position;
		}
		focused_pos += p_relative_position;
		queue_redraw();
	}
}

void ViewportNavigationControl::gui_input(const Ref<InputEvent> &p_event) {
	// Mouse events
	const Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT) {
		_process_click(100, mouse_button->get_position(), mouse_button->is_pressed());
	}

	const Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		_process_drag(100, mouse_motion->get_global_position(), viewport->view_3d_controller->get_warped_mouse_motion(mouse_motion, viewport->surface->get_global_rect()));
	}

	// Touch events
	const Ref<InputEventScreenTouch> screen_touch = p_event;
	if (screen_touch.is_valid()) {
		_process_click(screen_touch->get_index(), screen_touch->get_position(), screen_touch->is_pressed());
	}

	const Ref<InputEventScreenDrag> screen_drag = p_event;
	if (screen_drag.is_valid()) {
		_process_drag(screen_drag->get_index(), screen_drag->get_position(), screen_drag->get_relative());
	}
}

void ViewportNavigationControl::_update_navigation() {
	if (focused_index == -1) {
		return;
	}

	Vector2 delta = focused_pos - (get_size() / 2.0);
	Vector2 delta_normalized = delta.normalized();
	switch (nav_mode) {
		case View3DController::NAV_MODE_MOVE: {
			real_t speed_multiplier = MIN(delta.length() / (get_size().x * 100.0), 3.0);
			real_t speed = viewport->view_3d_controller->get_freelook_speed() * speed_multiplier;

			Vector3 forward;
			if (viewport->view_3d_controller->get_freelook_scheme() == View3DController::FreelookScheme::FREELOOK_FULLY_AXIS_LOCKED) {
				// Forward/backward keys will always go straight forward/backward, never moving on the Y axis.
				forward = Vector3(0, 0, delta_normalized.y).rotated(Vector3(0, 1, 0), viewport->camera->get_rotation().y);
			} else {
				// Forward/backward keys will be relative to the camera pitch.
				forward = viewport->camera->get_transform().basis.xform(Vector3(0, 0, delta_normalized.y));
			}

			const Vector3 right = viewport->camera->get_transform().basis.xform(Vector3(delta_normalized.x, 0, 0));

			const Vector3 direction = forward + right;
			const Vector3 motion = direction * speed;
			viewport->view_3d_controller->cursor.pos += motion;
			viewport->view_3d_controller->cursor.eye_pos += motion;
		} break;

		case View3DController::NAV_MODE_LOOK: {
			real_t speed_multiplier = MIN(delta.length() / (get_size().x * 2.5), 3.0);
			real_t speed = viewport->view_3d_controller->get_freelook_speed() * speed_multiplier;
			viewport->view_3d_controller->cursor_look(nullptr, delta_normalized * speed);
		} break;

		case View3DController::NAV_MODE_PAN: {
			real_t speed_multiplier = MIN(delta.length() / (get_size().x), 3.0);
			real_t speed = viewport->view_3d_controller->get_freelook_speed() * speed_multiplier;
			viewport->view_3d_controller->cursor_pan(nullptr, -delta_normalized * speed);
		} break;
		case View3DController::NAV_MODE_ZOOM: {
			real_t speed_multiplier = MIN(delta.length() / (get_size().x), 3.0);
			real_t speed = viewport->view_3d_controller->get_freelook_speed() * speed_multiplier;
			viewport->view_3d_controller->cursor_zoom(nullptr, delta_normalized * speed);
		} break;
		case View3DController::NAV_MODE_ORBIT: {
			real_t speed_multiplier = MIN(delta.length() / (get_size().x), 3.0);
			real_t speed = viewport->view_3d_controller->get_freelook_speed() * speed_multiplier;
			viewport->view_3d_controller->cursor_orbit(nullptr, delta_normalized * speed);
		} break;
		case View3DController::NAV_MODE_NONE: {
		} break;
	}
}

void ViewportNavigationControl::set_navigation_mode(View3DController::NavigationMode p_nav_mode) {
	nav_mode = p_nav_mode;
}

void ViewportNavigationControl::set_viewport(Node3DEditorViewport *p_viewport) {
	viewport = p_viewport;
}

void ViewportRotationControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			axis_menu_options.clear();
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_RIGHT);
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_TOP);
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_FRONT);
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_LEFT);
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_BOTTOM);
			axis_menu_options.push_back(Node3DEditorViewport::VIEW_REAR);

			axis_colors.clear();
			axis_colors.push_back(get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)));
			axis_colors.push_back(get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)));
			axis_colors.push_back(get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor)));
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			if (viewport != nullptr) {
				_draw();
			}
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			focused_axis = -2;
			queue_redraw();
		} break;

		case NOTIFICATION_WM_WINDOW_FOCUS_OUT: {
			gizmo_activated = false;
		} break;
	}
}

void ViewportRotationControl::_draw() {
	const Vector2 center = get_size() / 2.0;
	const real_t radius = get_size().x / 2.0;

	if (focused_axis > -2 || orbiting_index != -1) {
		draw_circle(center, radius, Color(0.5, 0.5, 0.5, 0.25), true, -1.0, true);
	}

	Vector<Axis2D> axis_to_draw;
	_get_sorted_axis(axis_to_draw);
	for (int i = 0; i < axis_to_draw.size(); ++i) {
		_draw_axis(axis_to_draw[i]);
	}
}

void ViewportRotationControl::_draw_axis(const Axis2D &p_axis) {
	const bool focused = focused_axis == p_axis.axis;
	const bool positive = p_axis.is_positive;
	const int direction = p_axis.axis % 3;

	const Color axis_color = axis_colors[direction];
	const double min_alpha = 0.35;
	const double alpha = focused ? 1.0 : Math::remap((p_axis.z_axis + 1.0) / 2.0, 0, 0.5, min_alpha, 1.0);
	const Color c = focused ? Color(axis_color.lightened(0.25), 1.0) : Color(axis_color, alpha);

	// Highlight positive axis text when hovered.
	const Color c_positive_axis = focused ? Color(1.0, 1.0, 1.0, alpha) : Color(0.0, 0.0, 0.0, alpha * 0.6);

	// Highlight negative axis text when hovered, but hide when not focused.
	const Color c_negative_axis = focused ? Color(1.0, 1.0, 1.0, alpha) : Color(axis_color, 0);

	if (positive) {
		// Draw axis lines for the positive axes.
		const Vector2 center = get_size() / 2.0;
		const Vector2 diff = p_axis.screen_point - center;
		const float line_length = MAX(diff.length() - AXIS_CIRCLE_RADIUS - 0.5 * EDSCALE, 0);

		draw_line(center + diff.limit_length(0.5 * EDSCALE), center + diff.limit_length(line_length), c, 1.5 * EDSCALE, true);

		draw_circle(p_axis.screen_point, AXIS_CIRCLE_RADIUS, c, true, -1.0, true);

		// Draw the axis letter for the positive axes.
		const String axis_name = direction == 0 ? "X" : (direction == 1 ? "Y" : "Z");
		const Ref<Font> &font = get_theme_font(SNAME("rotation_control"), EditorStringName(EditorFonts));
		const int font_size = get_theme_font_size(SNAME("rotation_control_size"), EditorStringName(EditorFonts));
		const Size2 char_size = font->get_char_size(axis_name[0], font_size);
		const Vector2 char_offset = Vector2(-char_size.width / 2.0, char_size.height * 0.25);
		draw_char(font, p_axis.screen_point + char_offset, axis_name, font_size, c_positive_axis);
	} else {
		// Draw an outline around the negative axes.
		draw_circle(p_axis.screen_point, AXIS_CIRCLE_RADIUS, c, true, -1.0, true);
		draw_circle(p_axis.screen_point, AXIS_CIRCLE_RADIUS * 0.8, c.darkened(0.4), true, -1.0, true);

		// Draw the text for the negative axes.
		const String axis_name = direction == 0 ? "-X" : (direction == 1 ? "-Y" : "-Z");
		const Ref<Font> &font = get_theme_font(SNAME("rotation_control"), EditorStringName(EditorFonts));
		const int font_size = get_theme_font_size(SNAME("rotation_control_size"), EditorStringName(EditorFonts));
		const Size2 string_size = font->get_string_size(axis_name, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size);
		const float font_ascent = font->get_ascent(font_size);
		const float font_descent = font->get_descent(font_size);
		const float string_height = font_ascent + font_descent;
		const Vector2 offset(-string_size.width / 2.0, string_height * 0.25);
		draw_string(font, p_axis.screen_point + offset, axis_name, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, c_negative_axis);
	}
}

void ViewportRotationControl::_get_sorted_axis(Vector<Axis2D> &r_axis) {
	const Vector2 center = get_size() / 2.0;
	const real_t radius = get_size().x / 2.0 - AXIS_CIRCLE_RADIUS - 2.0 * EDSCALE;
	const Basis camera_basis = viewport->view_3d_controller->to_camera_transform().get_basis().inverse();

	for (int i = 0; i < 3; ++i) {
		Vector3 axis_3d = camera_basis.get_column(i);
		Vector2 axis_vector = Vector2(axis_3d.x, -axis_3d.y) * radius;

		if (Math::abs(axis_3d.z) < 1.0) {
			Axis2D pos_axis;
			pos_axis.axis = i;
			pos_axis.screen_point = center + axis_vector;
			pos_axis.z_axis = axis_3d.z;
			pos_axis.is_positive = true;
			r_axis.push_back(pos_axis);

			Axis2D neg_axis;
			neg_axis.axis = i + 3;
			neg_axis.screen_point = center - axis_vector;
			neg_axis.z_axis = -axis_3d.z;
			neg_axis.is_positive = false;
			r_axis.push_back(neg_axis);
		} else {
			// Special case when the camera is aligned with one axis.
			Axis2D axis;
			axis.axis = i + (axis_3d.z <= 0 ? 0 : 3);
			axis.screen_point = center;
			axis.z_axis = 1.0;
			// Invert display style to fix aligned axis rendering.
			axis.is_positive = (axis_3d.z > 0);
			r_axis.push_back(axis);
		}
	}

	r_axis.sort_custom<Axis2DCompare>();
}

void ViewportRotationControl::_process_click(int p_index, Vector2 p_position, bool p_pressed) {
	if (orbiting_index != -1 && orbiting_index != p_index) {
		return;
	}
	if (p_pressed) {
		if (p_position.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
			orbiting_index = p_index;
		}
	} else {
		if (focused_axis > -1 && gizmo_activated) {
			viewport->_menu_option(axis_menu_options[focused_axis]);
			_update_focus();
		}
		orbiting_index = -1;
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(orbiting_mouse_start);
		}
	}
}

void ViewportRotationControl::_process_drag(Ref<InputEventWithModifiers> p_event, int p_index, Vector2 p_position, Vector2 p_relative_position) {
	Point2 mouse_pos = get_local_mouse_position();
	const bool movement_threshold_passed = original_mouse_pos.distance_to(mouse_pos) > 4 * EDSCALE;
	if (orbiting_index == p_index && gizmo_activated && movement_threshold_passed) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_VISIBLE) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
			orbiting_mouse_start = p_position;
			saved_cursor = viewport->view_3d_controller->cursor;
		}
		viewport->view_3d_controller->cursor_orbit(p_event, p_relative_position);
		focused_axis = -1;
	} else {
		_update_focus();
	}
}

void ViewportRotationControl::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	// Key events
	const Ref<InputEventKey> k = p_event;

	if (k.is_valid() && k->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(orbiting_mouse_start);
			viewport->view_3d_controller->cursor = saved_cursor;
			gizmo_activated = false;
		}
	}

	// Mouse events
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			_process_click(100, mb->get_position(), mb->is_pressed());
			if (mb->is_pressed()) {
				gizmo_activated = true;
				original_mouse_pos = get_local_mouse_position();
				grab_focus();
			}
		} else if (mb->get_button_index() == MouseButton::RIGHT) {
			if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
				Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
				Input::get_singleton()->warp_mouse(orbiting_mouse_start);
				viewport->view_3d_controller->cursor = saved_cursor;
				gizmo_activated = false;
			}
		}
	}

	const Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		_process_drag(mm, 100, mm->get_global_position(), viewport->view_3d_controller->get_warped_mouse_motion(mm, viewport->surface->get_global_rect()));
	}

	// Touch events
	const Ref<InputEventScreenTouch> screen_touch = p_event;
	if (screen_touch.is_valid()) {
		_process_click(screen_touch->get_index(), screen_touch->get_position(), screen_touch->is_pressed());
	}

	const Ref<InputEventScreenDrag> screen_drag = p_event;
	if (screen_drag.is_valid()) {
		_process_drag(nullptr, screen_drag->get_index(), screen_drag->get_position(), screen_drag->get_relative());
	}
}

void ViewportRotationControl::_update_focus() {
	int original_focus = focused_axis;
	focused_axis = -2;
	Vector2 mouse_pos = get_local_mouse_position();

	if (mouse_pos.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
		focused_axis = -1;
	}

	Vector<Axis2D> axes;
	_get_sorted_axis(axes);

	for (int i = 0; i < axes.size(); i++) {
		const Axis2D &axis = axes[i];
		if (mouse_pos.distance_to(axis.screen_point) < AXIS_CIRCLE_RADIUS) {
			focused_axis = axis.axis;
		}
	}

	if (focused_axis != original_focus) {
		queue_redraw();
	}
}

void ViewportRotationControl::set_viewport(Node3DEditorViewport *p_viewport) {
	viewport = p_viewport;
}

void Node3DEditorViewport::_view_settings_confirmed(real_t p_interp_delta) {
	// Set FOV override multiplier back to the default, so that the FOV
	// setting specified in the View menu is correctly applied.
	view_3d_controller->cursor.fov_scale = 1.0;

	view_3d_controller->update_camera(p_interp_delta);
}

bool Node3DEditorViewport::_open_pie_for(const Ref<InputEvent> &p_event, Key p_key) {
	if (ED_IS_SHORTCUT("spatial_editor/pie_shading", p_event)) {
		open_pie("shading", p_key);
		return true;
	}
	if (ED_IS_SHORTCUT("spatial_editor/pie_view", p_event)) {
		open_pie("view", p_key);
		return true;
	}
	if (ED_IS_SHORTCUT("spatial_editor/pie_snap", p_event)) {
		open_pie("snap", p_key);
		return true;
	}
	return false;
}

void Node3DEditorViewport::open_pie(const StringName &p_name, Key p_key) {
	if (!pie) {
		pie = memnew(EditorPieMenu);
		add_child(pie);
		pie->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		// Back to the view once it closes, so its keys work again.
		pie->connect(SNAME("closed"), callable_mp(this, &Node3DEditorViewport::_pie_closed), CONNECT_DEFERRED);
	}
	pie->clear();
	if (p_name == StringName("shading")) {
		_fill_shading_pie();
	} else if (p_name == StringName("view")) {
		_fill_view_pie();
	} else if (p_name == StringName("snap")) {
		_fill_snap_pie();
	} else {
		return;
	}
	pie->open(pie->get_local_mouse_position(), p_key);
}

void Node3DEditorViewport::_fill_shading_pie() {
	pie->set_title(TTR("Shading"));
	const Color ink = get_theme_color(SNAME("icon_normal_color"), SNAME("Button"));
	const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	const int icon_size = get_editor_theme_icon(SNAME("ToolMove"))->get_width();
	const Viewport::DebugDraw drawn = viewport->get_debug_draw();

	struct Shading {
		EditorPieMenu::Direction direction;
		Node3DEditorChrome::Shading shading;
		const char *name;
		Viewport::DebugDraw draw;
	};
	const Shading shadings[] = {
		{ EditorPieMenu::DIRECTION_LEFT, Node3DEditorChrome::SHADING_WIREFRAME, TTRC("Wireframe"), Viewport::DEBUG_DRAW_WIREFRAME },
		{ EditorPieMenu::DIRECTION_BOTTOM, Node3DEditorChrome::SHADING_UNSHADED, TTRC("Unshaded"), Viewport::DEBUG_DRAW_UNSHADED },
		{ EditorPieMenu::DIRECTION_TOP, Node3DEditorChrome::SHADING_LIGHTING, TTRC("Lighting"), Viewport::DEBUG_DRAW_LIGHTING },
		{ EditorPieMenu::DIRECTION_RIGHT, Node3DEditorChrome::SHADING_NORMAL, TTRC("Normal"), Viewport::DEBUG_DRAW_DISABLED },
	};
	for (const Shading &shading : shadings) {
		EditorPieMenu::Item item;
		item.text = TTRGET(shading.name);
		item.icon = Node3DEditorChrome::make_shading_icon(shading.shading, icon_size, ink, accent);
		item.action = callable_mp(spatial_editor, &Node3DEditor::set_shading).bind((int)shading.shading);
		item.current = drawn == shading.draw;
		pie->set_item(shading.direction, item);
	}

	// And what else is drawn, one step away.
	struct Toggle {
		EditorPieMenu::Direction direction;
		Node3DEditor::Overlay overlay;
		const char *name;
		const char *icon;
	};
	const Toggle toggles[] = {
		{ EditorPieMenu::DIRECTION_TOP_LEFT, Node3DEditor::OVERLAY_GIZMOS, TTRC("Gizmos"), "Node3D" },
		{ EditorPieMenu::DIRECTION_TOP_RIGHT, Node3DEditor::OVERLAY_ENVIRONMENT, TTRC("Environment"), "WorldEnvironment" },
		{ EditorPieMenu::DIRECTION_BOTTOM_LEFT, Node3DEditor::OVERLAY_GRID, TTRC("Grid"), "Grid" },
	};
	for (const Toggle &toggle : toggles) {
		EditorPieMenu::Item item;
		item.text = TTRGET(toggle.name);
		item.icon = get_editor_theme_icon(toggle.icon);
		item.action = callable_mp(spatial_editor, &Node3DEditor::toggle_overlay).bind((int)toggle.overlay);
		item.current = spatial_editor->is_overlay_shown_everywhere(toggle.overlay);
		pie->set_item(toggle.direction, item);
	}
	EditorPieMenu::Item overdraw;
	overdraw.text = TTR("Overdraw");
	overdraw.action = callable_mp(spatial_editor, &Node3DEditor::set_display_everywhere).bind((int)VIEW_DISPLAY_OVERDRAW);
	overdraw.current = drawn == Viewport::DEBUG_DRAW_OVERDRAW;
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM_RIGHT, overdraw);
}

void Node3DEditorViewport::_fill_view_pie() {
	pie->set_title(TTR("View"));
	const View3DController::ViewType type = view_3d_controller->get_view_type();
	struct View {
		EditorPieMenu::Direction direction;
		int option;
		const char *name;
		View3DController::ViewType type;
	};
	const View views[] = {
		{ EditorPieMenu::DIRECTION_LEFT, VIEW_LEFT, TTRC("Left"), View3DController::VIEW_TYPE_LEFT },
		{ EditorPieMenu::DIRECTION_RIGHT, VIEW_RIGHT, TTRC("Right"), View3DController::VIEW_TYPE_RIGHT },
		{ EditorPieMenu::DIRECTION_BOTTOM, VIEW_BOTTOM, TTRC("Bottom"), View3DController::VIEW_TYPE_BOTTOM },
		{ EditorPieMenu::DIRECTION_TOP, VIEW_TOP, TTRC("Top"), View3DController::VIEW_TYPE_TOP },
		{ EditorPieMenu::DIRECTION_TOP_LEFT, VIEW_FRONT, TTRC("Front"), View3DController::VIEW_TYPE_FRONT },
		{ EditorPieMenu::DIRECTION_TOP_RIGHT, VIEW_REAR, TTRC("Rear"), View3DController::VIEW_TYPE_REAR },
	};
	for (const View &view : views) {
		EditorPieMenu::Item item;
		item.text = TTRGET(view.name);
		item.action = callable_mp(this, &Node3DEditorViewport::_menu_option).bind(view.option);
		item.current = type == view.type;
		pie->set_item(view.direction, item);
	}
	EditorPieMenu::Item projection;
	projection.text = view_3d_controller->is_orthogonal() ? TTR("Perspective") : TTR("Orthogonal");
	projection.icon = get_editor_theme_icon(SNAME("Camera3D"));
	projection.action = callable_mp(this, &Node3DEditorViewport::_menu_option).bind((int)VIEW_SWITCH_PERSPECTIVE_ORTHOGONAL);
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM_LEFT, projection);
	EditorPieMenu::Item focus;
	focus.text = TTR("Focus Selection");
	focus.icon = get_editor_theme_icon(SNAME("CenterView"));
	focus.action = callable_mp(this, &Node3DEditorViewport::_menu_option).bind((int)VIEW_CENTER_TO_SELECTION);
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM_RIGHT, focus);
}

void Node3DEditorViewport::_fill_snap_pie() {
	pie->set_title(TTR("Snap"));
	struct Toggle {
		EditorPieMenu::Direction direction;
		int option;
		const char *name;
		const char *icon;
	};
	const Toggle toggles[] = {
		{ EditorPieMenu::DIRECTION_TOP, Node3DEditor::TOOL_OPT_USE_SNAP, TTRC("Use Snap"), "Snap" },
		{ EditorPieMenu::DIRECTION_TOP_LEFT, Node3DEditor::TOOL_OPT_LOCAL_COORDS, TTRC("Local Space"), "Object" },
	};
	for (const Toggle &toggle : toggles) {
		EditorPieMenu::Item item;
		item.text = TTRGET(toggle.name);
		item.icon = get_editor_theme_icon(toggle.icon);
		item.action = callable_mp(spatial_editor, &Node3DEditor::toggle_tool_option).bind(toggle.option);
		item.current = spatial_editor->is_tool_option_on(toggle.option);
		pie->set_item(toggle.direction, item);
	}

	EditorPieMenu::Item floor;
	floor.text = TTR("Snap to Floor");
	floor.action = callable_mp(spatial_editor, &Node3DEditor::snap_selected_nodes_to_floor);
	pie->set_item(EditorPieMenu::DIRECTION_LEFT, floor);
	EditorPieMenu::Item align;
	align.text = TTR("Align Transform with View");
	align.action = callable_mp(this, &Node3DEditorViewport::_menu_option).bind((int)VIEW_ALIGN_TRANSFORM_WITH_VIEW);
	pie->set_item(EditorPieMenu::DIRECTION_RIGHT, align);
	EditorPieMenu::Item align_rotation;
	align_rotation.text = TTR("Align Rotation with View");
	align_rotation.action = callable_mp(this, &Node3DEditorViewport::_menu_option).bind((int)VIEW_ALIGN_ROTATION_WITH_VIEW);
	pie->set_item(EditorPieMenu::DIRECTION_TOP_RIGHT, align_rotation);
	EditorPieMenu::Item settings;
	settings.text = TTR("Snap Settings");
	settings.icon = get_editor_theme_icon(SNAME("Tools"));
	settings.action = callable_mp(spatial_editor, &Node3DEditor::show_snap_settings);
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM, settings);
	EditorPieMenu::Item reset_position;
	reset_position.text = TTR("Reset Position");
	reset_position.action = callable_mp(this, &Node3DEditorViewport::_reset_transform_by_index).bind((int)TransformType::POSITION);
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM_LEFT, reset_position);
	EditorPieMenu::Item reset_rotation;
	reset_rotation.text = TTR("Reset Rotation");
	reset_rotation.action = callable_mp(this, &Node3DEditorViewport::_reset_transform_by_index).bind((int)TransformType::ROTATION);
	pie->set_item(EditorPieMenu::DIRECTION_BOTTOM_RIGHT, reset_rotation);
}

void Node3DEditorViewport::_pie_closed() {
	// Unless it closed because the view went away - the editor quitting, the
	// pane closing - or because the focus went somewhere else on purpose.
	if (surface->is_visible_in_tree() && get_viewport() && !get_viewport()->gui_get_focus_owner()) {
		surface->grab_focus();
	}
}

bool Node3DEditorViewport::is_view_type_top() const {
	return view_3d_controller.is_valid() && view_3d_controller->get_view_type() == View3DController::VIEW_TYPE_TOP;
}

void Node3DEditorViewport::set_top_right_clearance(real_t p_width) {
	if (Math::is_equal_approx(top_right_clearance, p_width)) {
		return;
	}
	top_right_clearance = p_width;
	const real_t right = -10.0 * EDSCALE - p_width;
	top_right_vbox->set_offset(SIDE_RIGHT, right);
	top_right_vbox->set_offset(SIDE_LEFT, right - top_right_vbox->get_combined_minimum_size().width);
}

void Node3DEditorViewport::_update_navigation_controls_visibility() {
	bool show_viewport_rotation_gizmo = EDITOR_GET("editors/3d/navigation/show_viewport_rotation_gizmo") && (!previewing_cinema && !previewing_camera);
	rotation_control->set_visible(show_viewport_rotation_gizmo);

	bool show_viewport_navigation_gizmo = EDITOR_GET("editors/3d/navigation/show_viewport_navigation_gizmo") && (!previewing_cinema && !previewing_camera);
	position_control->set_visible(show_viewport_navigation_gizmo);
	look_control->set_visible(show_viewport_navigation_gizmo);
}

bool Node3DEditorViewport::_is_rotation_arc_visible() const {
	return _edit.mode == TRANSFORM_ROTATE && !Math::is_zero_approx(_edit.accumulated_rotation_angle) && _edit.gizmo_initiated;
}

int Node3DEditorViewport::get_selected_count() const {
	const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();

	int count = 0;

	for (const KeyValue<ObjectID, Object *> &E : selection) {
		Node3D *sp = ObjectDB::get_instance<Node3D>(E.key);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		count++;
	}

	return count;
}

void Node3DEditorViewport::cancel_transform() {
	const List<Node *> &selection = editor_selection->get_top_selected_node_list();

	for (Node *E : selection) {
		Node3D *sp = Object::cast_to<Node3D>(E);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		if (se && se->gizmo.is_valid()) {
			Vector<int> ids;
			Vector<Transform3D> restore;

			for (const KeyValue<int, Transform3D> &GE : se->subgizmos) {
				ids.push_back(GE.key);
				restore.push_back(GE.value);
			}

			se->gizmo->commit_subgizmos(ids, restore, true);
		}

		sp->set_global_transform(se->original);
	}

	for (const KeyValue<Node3D *, Transform3D> &pair : _edit.children_original_globals) {
		pair.key->set_global_transform(pair.value);
	}

	collision_reposition = false;
	finish_transform();
	set_message(TTRC("Transform Aborted."), 3);
}

void Node3DEditorViewport::_update_shrink() {
	const float scaling_3d_scale = GLOBAL_GET("rendering/scaling_3d/scale");
	const float shrink_factor = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_HALF_RESOLUTION)) ? 0.5 : 1.0;
	viewport->set_scaling_3d_scale(MAX(0.25, scaling_3d_scale * shrink_factor));
}

float Node3DEditorViewport::get_znear() const {
	return CLAMP(spatial_editor->get_znear(), MIN_Z, MAX_Z);
}

float Node3DEditorViewport::get_zfar() const {
	return CLAMP(spatial_editor->get_zfar(), MIN_Z, MAX_Z);
}

float Node3DEditorViewport::get_fov() const {
	return CLAMP(spatial_editor->get_fov() * view_3d_controller->cursor.fov_scale, MIN_FOV, MAX_FOV);
}

Transform3D Node3DEditorViewport::_get_camera_transform() const {
	return camera->get_global_transform();
}

Vector3 Node3DEditorViewport::_get_camera_position() const {
	return _get_camera_transform().origin;
}

Point2 Node3DEditorViewport::point_to_screen(const Vector3 &p_point) {
	return camera->unproject_position(p_point);
}

Vector3 Node3DEditorViewport::get_ray_pos(const Vector2 &p_pos) const {
	return camera->project_ray_origin(p_pos);
}

Vector3 Node3DEditorViewport::_get_camera_normal() const {
	return -_get_camera_transform().basis.get_column(2);
}

Vector3 Node3DEditorViewport::get_ray(const Vector2 &p_pos) const {
	return camera->project_ray_normal(p_pos);
}

void Node3DEditorViewport::_clear_selected() {
	if (previewing) {
		return;
	}

	_edit.gizmo = Ref<EditorNode3DGizmo>();
	_edit.gizmo_handle = -1;
	_edit.gizmo_handle_secondary = false;
	_edit.gizmo_initial_value = Variant();

	Node3D *selected = spatial_editor->get_single_selected_node();
	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;

	if (se && se->gizmo.is_valid()) {
		se->subgizmos.clear();
		se->gizmo->redraw();
		se->gizmo.unref();
		spatial_editor->update_transform_gizmo();
	} else {
		editor_selection->clear();
		spatial_editor->edit(nullptr);
	}
}

void Node3DEditorViewport::_select_clicked(bool p_allow_locked) {
	if (previewing) {
		clicked = ObjectID();
		return;
	}

	Node *node = ObjectDB::get_instance<Node3D>(clicked);
	Node3D *selected = Object::cast_to<Node3D>(node);
	clicked = ObjectID();

	if (!selected) {
		return;
	}

	Node *edited_scene = get_edited_scene();

	// Prevent selection of nodes not owned by the edited scene.
	while (node && node != edited_scene->get_parent()) {
		Node *node_owner = node->get_owner();
		if (node_owner == edited_scene || node == edited_scene || (node_owner != nullptr && edited_scene->is_editable_instance(node_owner))) {
			break;
		}
		node = node->get_parent();
		selected = Object::cast_to<Node3D>(node);
	}

	if (!p_allow_locked) {
		// Replace the node by the group if grouped
		while (node && node != edited_scene->get_parent()) {
			Node3D *selected_tmp = Object::cast_to<Node3D>(node);
			if (selected_tmp && node->has_meta("_edit_group_")) {
				selected = selected_tmp;
			}
			node = node->get_parent();
		}
	}

	if (p_allow_locked || (selected != nullptr && !_is_node_locked(selected))) {
		if (clicked_wants_append) {
			const List<Node *> &top_node_list = editor_selection->get_top_selected_node_list();
			const Node *active_node = top_node_list.is_empty() ? nullptr : top_node_list.back()->get();
			if (editor_selection->is_selected(selected)) {
				editor_selection->remove_node(selected);
				if (selected != active_node) {
					editor_selection->add_node(selected);
				}
			} else {
				editor_selection->add_node(selected);
			}
		} else {
			if (!editor_selection->is_selected(selected)) {
				editor_selection->clear();
				editor_selection->add_node(selected);
				EditorNode::get_singleton()->edit_node(selected);
			}
		}

		const List<Node *> &top_node_list = editor_selection->get_top_selected_node_list();
		if (top_node_list.size() == 1) {
			EditorNode::get_singleton()->edit_node(top_node_list.front()->get());
		}
	}
}

ObjectID Node3DEditorViewport::_select_ray(const Point2 &p_pos) const {
	Vector3 ray = get_ray(p_pos);
	Vector3 pos = get_ray_pos(p_pos);
	Vector2 shrinked_pos = p_pos;

	if (viewport->get_debug_draw() == Viewport::DEBUG_DRAW_SDFGI_PROBES) {
		RS::get_singleton()->sdfgi_set_debug_probe_select(pos, ray);
	}

	HashSet<Ref<EditorNode3DGizmo>> found_gizmos;

	Node *edited_scene = get_edited_scene();
	ObjectID closest;
	Node *item = nullptr;
	float closest_dist = 1e20;

	Vector<Node3D *> nodes_with_gizmos = spatial_editor->gizmo_bvh_ray_query(pos, pos + ray * camera->get_far());

	for (Node3D *spat : nodes_with_gizmos) {
		if (!spat || _is_node_locked(spat)) {
			continue;
		}

		Vector<Ref<Node3DGizmo>> gizmos = spat->get_gizmos();

		for (int j = 0; j < gizmos.size(); j++) {
			Ref<EditorNode3DGizmo> seg = gizmos[j];

			if (seg.is_null() || found_gizmos.has(seg)) {
				continue;
			}

			found_gizmos.insert(seg);
			Vector3 point;
			Vector3 normal;

			bool inters = seg->intersect_ray(camera, shrinked_pos, point, normal);

			if (!inters) {
				continue;
			}

			const real_t dist = pos.distance_to(point);

			if (dist < 0) {
				continue;
			}

			if (dist < closest_dist) {
				item = Object::cast_to<Node>(spat);
				if (item != edited_scene) {
					item = edited_scene->get_deepest_editable_node(item);
				}

				closest = item->get_instance_id();
				closest_dist = dist;
			}
		}
	}

	if (!item) {
		return ObjectID();
	}

	return closest;
}

float Node3DEditorViewport::_min_screen_dist_to_aabb(const AABB &p_aabb, const Transform3D &p_transform, const Point2 &p_cursor) const {
	Vector3 first_corner = p_transform.xform(p_aabb.get_endpoint(0));
	if (camera->is_position_behind(first_corner)) {
		return 0.0f;
	}
	Point2 screen_min = camera->unproject_position(first_corner);
	Point2 screen_max = screen_min;

	for (int i = 1; i < 8; i++) {
		Vector3 world_corner = p_transform.xform(p_aabb.get_endpoint(i));
		if (camera->is_position_behind(world_corner)) {
			return 0.0f;
		}
		Point2 s = camera->unproject_position(world_corner);
		screen_min = screen_min.min(s);
		screen_max = screen_max.max(s);
	}

	float dx = MAX(screen_min.x - p_cursor.x, MAX(0.0f, p_cursor.x - screen_max.x));
	float dy = MAX(screen_min.y - p_cursor.y, MAX(0.0f, p_cursor.y - screen_max.y));
	return Math::sqrt(dx * dx + dy * dy);
}

static bool _node_is_snap_source(Node *p_node, bool p_use_collision) {
	if (!p_use_collision) {
		return Object::cast_to<GeometryInstance3D>(p_node);
	}
	if (Object::cast_to<CollisionShape3D>(p_node)) {
		return true;
	}
	Node3D *n3d = Object::cast_to<Node3D>(p_node);
	if (!n3d) {
		return false;
	}
	for (const Ref<Node3DGizmo> &g : n3d->get_gizmos()) {
		Ref<EditorNode3DGizmo> seg = g;
		if (seg.is_valid() && seg->get_collision_meshes_are_snap_source()) {
			return true;
		}
	}
	return false;
}

static bool _node_has_snap_target(Node *p_node, bool p_use_collision) {
	if (_node_is_snap_source(p_node, p_use_collision)) {
		return true;
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		if (_node_has_snap_target(p_node->get_child(i), p_use_collision)) {
			return true;
		}
	}
	return false;
}

bool Node3DEditorViewport::_find_closest_vertex_on_node(const Point2 &p_screen_pos, Node3D *p_node, float &r_closest_screen_dist, Vector3 &r_vertex_world) const {
	bool found = false;
	bool use_collision = spatial_editor->is_vertex_snap_use_collision();
	bool walk_collision_segments = use_collision && Object::cast_to<CollisionShape3D>(p_node);

	Transform3D gt = p_node->get_global_transform();
	Vector<Ref<Node3DGizmo>> gizmos = p_node->get_gizmos();

	for (int i = 0; i < gizmos.size(); i++) {
		Ref<EditorNode3DGizmo> seg = gizmos[i];
		if (seg.is_null()) {
			continue;
		}

		if (walk_collision_segments) {
			const Vector<Vector3> &segments = seg->get_collision_segments();
			for (int si = 0; si < segments.size(); si++) {
				Vector3 world_v = gt.xform(segments[si]);
				if (camera->is_position_behind(world_v)) {
					continue;
				}
				Vector2 screen_v = camera->unproject_position(world_v);
				float dist = screen_v.distance_to(p_screen_pos);
				if (dist < r_closest_screen_dist) {
					r_closest_screen_dist = dist;
					r_vertex_world = world_v;
					found = true;
				}
			}
		}

		if (!use_collision || seg->get_collision_meshes_are_snap_source()) {
			const LocalVector<Ref<TriangleMesh>> &meshes = seg->get_collision_meshes();
			for (const Ref<TriangleMesh> &tm : meshes) {
				if (tm.is_null() || !tm->is_valid()) {
					continue;
				}

				const Vector<TriangleMesh::BVH> &bvh = tm->get_bvh();
				const Vector<TriangleMesh::Triangle> &triangles = tm->get_triangles();
				const Vector<Vector3> &vertices = tm->get_vertices();

				if (bvh.is_empty()) {
					continue;
				}

				// Traverse the TriangleMesh BVH, pruning branches whose screen-space
				// AABB is farther than the current best.
				LocalVector<int> stack;
				stack.push_back(bvh.size() - 1);

				while (!stack.is_empty()) {
					int node_idx = stack[stack.size() - 1];
					stack.resize(stack.size() - 1);

					const TriangleMesh::BVH &b = bvh[node_idx];

					if (_min_screen_dist_to_aabb(b.aabb, gt, p_screen_pos) >= r_closest_screen_dist) {
						continue;
					}

					if (b.face_index >= 0) {
						const TriangleMesh::Triangle &tri = triangles[b.face_index];
						for (int vi = 0; vi < 3; vi++) {
							Vector3 world_v = gt.xform(vertices[tri.indices[vi]]);
							if (camera->is_position_behind(world_v)) {
								continue;
							}
							Vector2 screen_v = camera->unproject_position(world_v);
							float dist = screen_v.distance_to(p_screen_pos);
							if (dist < r_closest_screen_dist) {
								r_closest_screen_dist = dist;
								r_vertex_world = world_v;
								found = true;
							}
						}
					} else {
						stack.push_back(b.left);
						stack.push_back(b.right);
					}
				}
			}
		}
	}

	return found;
}

bool Node3DEditorViewport::_find_closest_vertex_in_scene(const Point2 &p_screen_pos, float p_threshold, Vector3 &r_vertex_world, const HashMap<ObjectID, Vector3> *p_exclude) {
	float closest_screen_dist = p_threshold;
	bool found = false;

	Point2 min_pos(p_screen_pos.x - p_threshold, p_screen_pos.y - p_threshold);
	Point2 max_pos(p_screen_pos.x + p_threshold, p_screen_pos.y + p_threshold);
	Vector<Node3D *> nodes_with_gizmos = spatial_editor->gizmo_bvh_frustum_query(_build_screen_frustum(min_pos, max_pos));

	bool use_collision = spatial_editor->is_vertex_snap_use_collision();

	for (Node3D *spat : nodes_with_gizmos) {
		if (!spat) {
			continue;
		}

		if (!_node_is_snap_source(spat, use_collision)) {
			continue;
		}

		if (p_exclude) {
			bool should_skip = false;
			Node *current = spat;
			while (current) {
				if (p_exclude->has(current->get_instance_id())) {
					should_skip = true;
					break;
				}
				current = current->get_parent();
			}
			if (should_skip) {
				continue;
			}
		}

		if (_find_closest_vertex_on_node(p_screen_pos, spat, closest_screen_dist, r_vertex_world)) {
			found = true;
		}
	}

	return found;
}

void Node3DEditorViewport::_vertex_snap_commit() {
	for (const KeyValue<ObjectID, Vector3> &E : vertex_snap_original_positions) {
		Node3D *node = ObjectDB::get_instance<Node3D>(E.key);
		if (node) {
			vertex_snap_source += node->get_global_position() - E.value;
			break;
		}
	}

	HashMap<ObjectID, Vector3> original_positions;
	for (const KeyValue<ObjectID, Vector3> &E : vertex_snap_original_positions) {
		original_positions[E.key] = E.value;
	}
	vertex_snap_dragging = false;
	vertex_snap_has_target = false;
	vertex_snap_original_positions.clear();
	if (!vertex_snap_mode) {
		vertex_snap_has_source = false;
		set_message("");
	}
	surface->queue_redraw();

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Vertex Snap"));
	for (const KeyValue<ObjectID, Vector3> &E : original_positions) {
		Node3D *node = ObjectDB::get_instance<Node3D>(E.key);
		if (node) {
			undo_redo->add_do_method(node, "set_global_position", node->get_global_position());
			undo_redo->add_undo_method(node, "set_global_position", E.value);
		}
	}
	undo_redo->commit_action(false);
	_reset_follow_mode_count();
}

void Node3DEditorViewport::_vertex_snap_cancel() {
	vertex_snap_mode = false;
	vertex_snap_keycode = Key::NONE;
	vertex_snap_dragging = false;
	vertex_snap_has_source = false;
	vertex_snap_has_target = false;

	for (const KeyValue<ObjectID, Vector3> &E : vertex_snap_original_positions) {
		Node3D *node = ObjectDB::get_instance<Node3D>(E.key);
		if (node) {
			node->set_global_position(E.value);
		}
	}
	vertex_snap_original_positions.clear();
	set_message(TTR("Vertex Snap Canceled."), 3);
	surface->queue_redraw();
}

bool Node3DEditorViewport::_is_vertex_occluded(const Vector3 &p_world_pos, const Vector2 &p_screen_pos) const {
	Vector3 ray_pos = get_ray_pos(p_screen_pos);
	float vertex_dist = ray_pos.distance_to(p_world_pos);
	Vector<Node3D *> hits = spatial_editor->gizmo_bvh_ray_query(ray_pos, ray_pos + get_ray(p_screen_pos) * camera->get_far());
	for (Node3D *spat : hits) {
		if (!spat) {
			continue;
		}
		Vector<Ref<Node3DGizmo>> gizmos = spat->get_gizmos();
		for (int i = 0; i < gizmos.size(); i++) {
			Ref<EditorNode3DGizmo> seg = gizmos[i];
			if (seg.is_null()) {
				continue;
			}
			Vector3 point, normal;
			if (seg->intersect_ray(camera, p_screen_pos, point, normal)) {
				if (ray_pos.distance_to(point) < vertex_dist - 0.01f) {
					return true;
				}
			}
		}
	}
	return false;
}

void Node3DEditorViewport::_vertex_snap_update_source(const Point2 &p_screen_pos) {
	const List<Node *> &selection = editor_selection->get_top_selected_node_list();
	if (selection.is_empty()) {
		vertex_snap_has_source = false;
		return;
	}

	float threshold = VERTEX_SNAP_THRESHOLD * EDSCALE;
	Vector3 vw;
	bool found = false;

	if (spatial_editor->is_vertex_snap_origin_mode()) {
		found = _find_closest_vertex_in_scene(p_screen_pos, threshold, vw);
	} else {
		bool use_collision = spatial_editor->is_vertex_snap_use_collision();
		bool selection_has_snap_target = false;
		for (Node *E : selection) {
			Node3D *sp = Object::cast_to<Node3D>(E);
			if (!sp) {
				continue;
			}

			LocalVector<Node3D *> descendants;
			descendants.push_back(sp);
			while (!descendants.is_empty()) {
				Node3D *node = descendants[descendants.size() - 1];
				descendants.resize(descendants.size() - 1);
				if (_node_is_snap_source(node, use_collision)) {
					selection_has_snap_target = true;
					if (_find_closest_vertex_on_node(p_screen_pos, node, threshold, vw)) {
						found = true;
					}
				}
				for (int i = 0; i < node->get_child_count(); i++) {
					Node3D *child = Object::cast_to<Node3D>(node->get_child(i));
					if (child) {
						descendants.push_back(child);
					}
				}
			}
		}

		if (!found && !selection_has_snap_target) {
			found = _find_closest_vertex_in_scene(p_screen_pos, threshold, vw);
		}
	}

	if (found) {
		vertex_snap_source = vw;
		vertex_snap_has_source = true;
	} else {
		vertex_snap_has_source = false;
	}
}

void Node3DEditorViewport::_find_items_at_pos(const Point2 &p_pos, Vector<_RayResult> &r_results, bool p_include_locked_nodes) {
	Vector3 ray = get_ray(p_pos);
	Vector3 pos = get_ray_pos(p_pos);

	Vector<Node3D *> nodes_with_gizmos = spatial_editor->gizmo_bvh_ray_query(pos, pos + ray * camera->get_far());

	HashSet<Node3D *> found_nodes;

	for (Node3D *spat : nodes_with_gizmos) {
		if (!spat) {
			continue;
		}

		if (found_nodes.has(spat)) {
			continue;
		}

		if (!p_include_locked_nodes && _is_node_locked(spat)) {
			continue;
		}

		Vector<Ref<Node3DGizmo>> gizmos = spat->get_gizmos();
		for (int j = 0; j < gizmos.size(); j++) {
			Ref<EditorNode3DGizmo> seg = gizmos[j];

			if (seg.is_null()) {
				continue;
			}

			Vector3 point;
			Vector3 normal;

			bool inters = seg->intersect_ray(camera, p_pos, point, normal);

			if (!inters) {
				continue;
			}

			const real_t dist = pos.distance_to(point);

			if (dist < 0) {
				continue;
			}

			found_nodes.insert(spat);

			_RayResult res;
			res.item = spat;
			res.depth = dist;
			r_results.push_back(res);
			break;
		}
	}

	r_results.sort();
}

Vector3 Node3DEditorViewport::_get_screen_to_space(const Vector3 &p_vector3) {
	Projection cm;
	if (view_3d_controller->is_orthogonal()) {
		cm.set_orthogonal(camera->get_size(), get_size().aspect(), get_znear() + p_vector3.z, get_zfar());
	} else {
		cm.set_perspective(get_fov(), get_size().aspect(), get_znear() + p_vector3.z, get_zfar());
	}
	Vector2 screen_he = cm.get_viewport_half_extents();

	Transform3D camera_transform;
	camera_transform.translate_local(view_3d_controller->cursor.pos);
	camera_transform.basis.rotate(Vector3(1, 0, 0), -view_3d_controller->cursor.x_rot);
	camera_transform.basis.rotate(Vector3(0, 1, 0), -view_3d_controller->cursor.y_rot);
	camera_transform.translate_local(0, 0, view_3d_controller->cursor.distance);

	return camera_transform.xform(Vector3(((p_vector3.x / get_size().width) * 2.0 - 1.0) * screen_he.x, ((1.0 - (p_vector3.y / get_size().height)) * 2.0 - 1.0) * screen_he.y, -(get_znear() + p_vector3.z)));
}

Vector<Plane> Node3DEditorViewport::_build_screen_frustum(const Point2 &p_min, const Point2 &p_max) {
	const real_t z_offset = MAX(0.0, 5.0 - get_znear());
	Vector3 box[4] = {
		Vector3(p_min.x, p_min.y, z_offset),
		Vector3(p_max.x, p_min.y, z_offset),
		Vector3(p_max.x, p_max.y, z_offset),
		Vector3(p_min.x, p_max.y, z_offset),
	};

	Vector<Plane> frustum;
	Vector3 cam_pos = _get_camera_position();
	for (int i = 0; i < 4; i++) {
		Vector3 a = _get_screen_to_space(box[i]);
		Vector3 b = _get_screen_to_space(box[(i + 1) % 4]);
		if (view_3d_controller->is_orthogonal()) {
			frustum.push_back(Plane((a - b).normalized(), a));
		} else {
			frustum.push_back(Plane(a, b, cam_pos));
		}
	}
	Plane near_plane = Plane(-_get_camera_normal(), cam_pos);
	near_plane.d -= get_znear();
	frustum.push_back(near_plane);
	Plane far_plane = -near_plane;
	far_plane.d += get_zfar();
	frustum.push_back(far_plane);
	return frustum;
}

void Node3DEditorViewport::_select_region() {
	View3DController::Cursor cursor = view_3d_controller->cursor;

	if (cursor.region_begin == cursor.region_end) {
		if (!clicked_wants_append) {
			_clear_selected();
		}
		return;
	}

	Point2 region_min = cursor.region_begin.min(cursor.region_end);
	Point2 region_max = cursor.region_begin.max(cursor.region_end);
	Vector<Plane> frustum = _build_screen_frustum(region_min, region_max);

	if (spatial_editor->get_single_selected_node()) {
		Node3D *single_selected = spatial_editor->get_single_selected_node();
		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(single_selected);

		if (se) {
			Ref<EditorNode3DGizmo> old_gizmo;
			if (!clicked_wants_append) {
				se->subgizmos.clear();
				old_gizmo = se->gizmo;
				se->gizmo.unref();
			}

			bool found_subgizmos = false;
			Vector<Ref<Node3DGizmo>> gizmos = single_selected->get_gizmos();
			for (int j = 0; j < gizmos.size(); j++) {
				Ref<EditorNode3DGizmo> seg = gizmos[j];
				if (seg.is_null()) {
					continue;
				}

				if (se->gizmo.is_valid() && se->gizmo != seg) {
					continue;
				}

				Vector<int> subgizmos = seg->subgizmos_intersect_frustum(camera, frustum);
				if (!subgizmos.is_empty()) {
					se->gizmo = seg;
					for (int i = 0; i < subgizmos.size(); i++) {
						int subgizmo_id = subgizmos[i];
						if (!se->subgizmos.has(subgizmo_id)) {
							se->subgizmos.insert(subgizmo_id, se->gizmo->get_subgizmo_transform(subgizmo_id));
						}
					}
					found_subgizmos = true;
					break;
				}
			}

			if (!clicked_wants_append || found_subgizmos) {
				if (se->gizmo.is_valid()) {
					se->gizmo->redraw();
				}

				if (old_gizmo != se->gizmo && old_gizmo.is_valid()) {
					old_gizmo->redraw();
				}

				spatial_editor->update_transform_gizmo();
			}

			if (found_subgizmos) {
				return;
			}
		}
	}

	if (!clicked_wants_append) {
		_clear_selected();
	}

	Vector<Node3D *> nodes_with_gizmos = spatial_editor->gizmo_bvh_frustum_query(frustum);
	HashSet<Node3D *> found_nodes;
	Vector<Node *> selected;

	Node *edited_scene = get_edited_scene();
	if (edited_scene == nullptr) {
		return;
	}

	for (Node3D *sp : nodes_with_gizmos) {
		if (!sp || _is_node_locked(sp)) {
			continue;
		}

		if (found_nodes.has(sp)) {
			continue;
		}
		found_nodes.insert(sp);

		Node *node = Object::cast_to<Node>(sp);

		// Selection requires that the node is the edited scene or its descendant, and has an owner.
		if (node != edited_scene) {
			if (!node->get_owner() || !edited_scene->is_ancestor_of(node)) {
				continue;
			}
			node = edited_scene->get_deepest_editable_node(node);
			while (node != edited_scene) {
				Node *node_owner = node->get_owner();
				if (node_owner == edited_scene || (node_owner != nullptr && edited_scene->is_editable_instance(node_owner))) {
					break;
				}
				node = node->get_parent();
			}
		}

		// Replace the node by the group if grouped
		if (node->is_class("Node3D")) {
			Node3D *sel = Object::cast_to<Node3D>(node);
			while (node && node != get_edited_scene()->get_parent()) {
				Node3D *selected_tmp = Object::cast_to<Node3D>(node);
				if (selected_tmp && node->has_meta("_edit_group_")) {
					sel = selected_tmp;
				}
				node = node->get_parent();
			}
			node = sel;
		}

		if (_is_node_locked(node)) {
			continue;
		}

		Vector<Ref<Node3DGizmo>> gizmos = sp->get_gizmos();
		for (int j = 0; j < gizmos.size(); j++) {
			Ref<EditorNode3DGizmo> seg = gizmos[j];
			if (seg.is_null()) {
				continue;
			}

			if (seg->intersect_frustum(camera, frustum)) {
				selected.push_back(node);
			}
		}
	}

	for (int i = 0; i < selected.size(); i++) {
		if (!editor_selection->is_selected(selected[i])) {
			editor_selection->add_node(selected[i]);
		}
	}

	const List<Node *> &top_node_list = editor_selection->get_top_selected_node_list();
	if (top_node_list.size() == 1) {
		EditorNode::get_singleton()->edit_node(top_node_list.front()->get());
	}
}

void Node3DEditorViewport::_view_state_changed() {
	if (view_3d_controller->get_orthogonal_mode() == View3DController::ORTHOGONAL_AUTO) {
		_menu_option(VIEW_ORTHOGONAL);
		view_3d_controller->force_auto_orthogonal();
	} else if (view_3d_controller->get_orthogonal_mode() == View3DController::ORTHOGONAL_DISABLED) {
		_menu_option(VIEW_PERSPECTIVE);
	}
	_update_name();
}

void Node3DEditorViewport::_update_name() {
	view_display_menu->set_text(view_3d_controller->get_view_type_name());
	view_display_menu->reset_size();
}

void Node3DEditorViewport::_compute_edit(const Point2 &p_point) {
	_edit.original_local = spatial_editor->are_local_coords_enabled();
	_edit.click_ray = get_ray(p_point);
	_edit.click_ray_pos = get_ray_pos(p_point);
	_edit.plane = TRANSFORM_VIEW;
	spatial_editor->update_transform_gizmo();
	_edit.center = spatial_editor->get_gizmo_transform().origin;

	Node3D *selected = spatial_editor->get_single_selected_node();
	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;

	if (se && se->gizmo.is_valid()) {
		for (const KeyValue<int, Transform3D> &E : se->subgizmos) {
			int subgizmo_id = E.key;
			se->subgizmos[subgizmo_id] = se->gizmo->get_subgizmo_transform(subgizmo_id);
		}
		se->original_local = selected->get_transform();
		se->original = selected->get_global_transform();
	} else {
		const List<Node *> &selection = editor_selection->get_top_selected_node_list();

		for (Node *E : selection) {
			Node3D *sp = Object::cast_to<Node3D>(E);
			if (!sp) {
				continue;
			}

			Node3DEditorSelectedItem *sel_item = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);

			if (!sel_item) {
				continue;
			}

			sel_item->original_local = sel_item->sp->get_local_gizmo_transform();
			sel_item->original = sel_item->sp->get_global_gizmo_transform();
		}
	}

	if (spatial_editor->is_preserve_children_transform_enabled() && _edit.children_original_globals.is_empty()) {
		const List<Node *> &selection = editor_selection->get_top_selected_node_list();
		for (Node *E : selection) {
			Node3D *sp = Object::cast_to<Node3D>(E);
			if (!sp) {
				continue;
			}

			int child_count = sp->get_child_count();
			for (int i = 0; i < child_count; i++) {
				Node3D *child = Object::cast_to<Node3D>(sp->get_child(i));
				if (child) {
					_edit.children_original_globals[child] = child->get_global_transform();
				}
			}
		}
	}
}

static Key _get_key_modifier_setting(const String &p_property) {
	switch (EDITOR_GET(p_property).operator int()) {
		case 0:
			return Key::NONE;
		case 1:
			return Key::SHIFT;
		case 2:
			return Key::ALT;
		case 3:
			return Key::META;
		case 4:
			return Key::CTRL;
	}
	return Key::NONE;
}

static Key _get_key_modifier(Ref<InputEventWithModifiers> e) {
	if (e->is_shift_pressed()) {
		return Key::SHIFT;
	}
	if (e->is_alt_pressed()) {
		return Key::ALT;
	}
	if (e->is_ctrl_pressed()) {
		return Key::CTRL;
	}
	if (e->is_meta_pressed()) {
		return Key::META;
	}
	return Key::NONE;
}

bool Node3DEditorViewport::_transform_gizmo_select(const Vector2 &p_screenpos, bool p_highlight_only) {
	if (!spatial_editor->is_gizmo_visible()) {
		return false;
	}
	if (get_selected_count() == 0) {
		if (p_highlight_only) {
			spatial_editor->select_gizmo_highlight_axis(-1);
		}
		return false;
	}

	Vector3 ray_pos = get_ray_pos(p_screenpos);
	Vector3 ray = get_ray(p_screenpos);

	Transform3D gt = spatial_editor->get_gizmo_transform();

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_MOVE) {
		int col_axis = -1;
		real_t col_d = 1e20;

		for (int i = 0; i < 3; i++) {
			const Vector3 grabber_pos = gt.origin + gt.basis.get_column(i).normalized() * gizmo_scale * (GIZMO_ARROW_OFFSET + (GIZMO_ARROW_SIZE * 0.5));
			const real_t grabber_radius = gizmo_scale * GIZMO_ARROW_SIZE;

			Vector3 r;

			if (Geometry3D::segment_intersects_sphere(ray_pos, ray_pos + ray * MAX_Z, grabber_pos, grabber_radius, &r)) {
				const real_t d = r.distance_to(ray_pos);
				if (d < col_d) {
					col_d = d;
					col_axis = i;
				}
			}
		}

		bool is_plane_translate = false;
		// plane select
		if (col_axis == -1) {
			col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				Vector3 ivec2 = gt.basis.get_column((i + 1) % 3).normalized();
				Vector3 ivec3 = gt.basis.get_column((i + 2) % 3).normalized();

				// Allow some tolerance to make the plane easier to click,
				// even if the click is actually slightly outside the plane.
				const Vector3 grabber_pos = gt.origin + (ivec2 + ivec3) * gizmo_scale * (GIZMO_PLANE_SIZE + GIZMO_PLANE_DST * 0.6667);

				Vector3 r;
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);

				if (plane.intersects_ray(ray_pos, ray, &r)) {
					const real_t dist = r.distance_to(grabber_pos);
					// Allow some tolerance to make the plane easier to click,
					// even if the click is actually slightly outside the plane.
					if (dist < (gizmo_scale * GIZMO_PLANE_SIZE * 1.5)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;

							is_plane_translate = true;
						}
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(col_axis + (is_plane_translate ? 6 : 0));

			} else {
				//handle plane translate
				_edit.mode = TRANSFORM_TRANSLATE;
				_compute_edit(p_screenpos);
				_edit.plane = TransformPlane(TRANSFORM_X_AXIS + col_axis + (is_plane_translate ? 3 : 0));
			}
			return true;
		}
	}

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_ROTATE) {
		int col_axis = -1;
		bool view_rotation_selected = false;
		bool trackball_selected = false;

		Vector3 hit_position;
		Vector3 hit_normal;
		real_t ray_length = gt.origin.distance_to(ray_pos) + (GIZMO_CIRCLE_SIZE * gizmo_scale) * 4.0f;
		if (Geometry3D::segment_intersects_sphere(ray_pos, ray_pos + ray * ray_length, gt.origin, gizmo_scale * (GIZMO_CIRCLE_SIZE), &hit_position, &hit_normal)) {
			if (hit_normal.dot(_get_camera_normal()) < 0.05) {
				hit_position = gt.xform_inv(hit_position).abs();
				int min_axis = hit_position.min_axis_index();
				if (hit_position[min_axis] < gizmo_scale * GIZMO_RING_HALF_WIDTH) {
					col_axis = min_axis;
				}
			}
		}

		if (col_axis == -1) {
			float col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);
				Vector3 r;
				if (!plane.intersects_ray(ray_pos, ray, &r)) {
					continue;
				}

				const real_t dist = r.distance_to(gt.origin);
				const Vector3 r_dir = (r - gt.origin).normalized();

				if (_get_camera_normal().dot(r_dir) <= 0.005) {
					if (dist > gizmo_scale * (GIZMO_CIRCLE_SIZE - GIZMO_RING_HALF_WIDTH) && dist < gizmo_scale * (GIZMO_CIRCLE_SIZE + GIZMO_RING_HALF_WIDTH)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;
						}
					}
				}
			}
		}

		if (col_axis == -1) {
			Vector3 ray_to_center = gt.origin - ray_pos;
			real_t ray_length_to_center = ray_to_center.dot(ray);
			Vector3 closest_point_on_ray = ray_pos + ray * ray_length_to_center;
			real_t distance_ray_to_center = closest_point_on_ray.distance_to(gt.origin);

			real_t view_rotation_radius = gizmo_scale * spatial_editor->gizmo_view_rotation_scale;
			real_t circumference_tolerance = gizmo_scale * GIZMO_RING_HALF_WIDTH;

			if (Math::abs(distance_ray_to_center - view_rotation_radius) < circumference_tolerance &&
					ray_length_to_center > 0) {
				view_rotation_selected = true;
			} else if (spatial_editor->is_trackball_enabled() && distance_ray_to_center < gizmo_scale * (GIZMO_CIRCLE_SIZE - GIZMO_RING_HALF_WIDTH) && ray_length_to_center > 0) {
				trackball_selected = true;
			}
		}

		if (view_rotation_selected) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(GIZMO_HIGHLIGHT_AXIS_VIEW_ROTATION);
			} else {
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_VIEW;
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = _get_camera_normal();
				_edit.view_axis_local = spatial_editor->get_gizmo_transform().basis.xform_inv(_get_camera_normal()).normalized();
				_edit.gizmo_initiated = true;
			}
			return true;
		} else if (trackball_selected) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(GIZMO_HIGHLIGHT_AXIS_TRACKBALL);
			} else {
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_VIEW;
				_edit.is_trackball = true;
				_edit.show_rotation_line = false;
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = _get_camera_normal();
				_edit.gizmo_initiated = true;
				spatial_editor->select_gizmo_highlight_axis(-1);
			}
			return true;
		} else if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(col_axis + 3);
			} else {
				//handle axis-specific rotate
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TransformPlane(TRANSFORM_X_AXIS + col_axis);
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = gt.basis.get_column(col_axis).normalized();
				_edit.gizmo_initiated = true;
			}
			return true;
		}
	}

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_SCALE) {
		int col_axis = -1;
		float col_d = 1e20;

		for (int i = 0; i < 3; i++) {
			const Vector3 grabber_pos = gt.origin + gt.basis.get_column(i).normalized() * gizmo_scale * GIZMO_SCALE_OFFSET;
			const real_t grabber_radius = gizmo_scale * GIZMO_ARROW_SIZE;

			Vector3 r;

			if (Geometry3D::segment_intersects_sphere(ray_pos, ray_pos + ray * MAX_Z, grabber_pos, grabber_radius, &r)) {
				const real_t d = r.distance_to(ray_pos);
				if (d < col_d) {
					col_d = d;
					col_axis = i;
				}
			}
		}

		bool is_plane_scale = false;
		// plane select
		if (col_axis == -1) {
			col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				const Vector3 ivec2 = gt.basis.get_column((i + 1) % 3).normalized();
				const Vector3 ivec3 = gt.basis.get_column((i + 2) % 3).normalized();

				// Allow some tolerance to make the plane easier to click,
				// even if the click is actually slightly outside the plane.
				const Vector3 grabber_pos = gt.origin + (ivec2 + ivec3) * gizmo_scale * (GIZMO_PLANE_SIZE + GIZMO_PLANE_DST * 0.6667);

				Vector3 r;
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);

				if (plane.intersects_ray(ray_pos, ray, &r)) {
					const real_t dist = r.distance_to(grabber_pos);
					// Allow some tolerance to make the plane easier to click,
					// even if the click is actually slightly outside the plane.
					if (dist < (gizmo_scale * GIZMO_PLANE_SIZE * 1.5)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;

							is_plane_scale = true;
						}
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(col_axis + (is_plane_scale ? 12 : 9));

			} else {
				//handle scale
				_edit.mode = TRANSFORM_SCALE;
				_compute_edit(p_screenpos);
				_edit.plane = TransformPlane(TRANSFORM_X_AXIS + col_axis + (is_plane_scale ? 3 : 0));
			}
			return true;
		}
	}

	if (p_highlight_only) {
		spatial_editor->select_gizmo_highlight_axis(-1);
	}

	return false;
}

void Node3DEditorViewport::_transform_gizmo_apply(Node3D *p_node, const Transform3D &p_transform, bool p_local) {
	if (p_transform.basis.determinant() == 0) {
		return;
	}

	bool preserve_children = spatial_editor->is_preserve_children_transform_enabled();

	Vector<Transform3D> children_global_transforms;
	Vector<Node3D *> node3d_children;

	if (preserve_children) {
		int child_count = p_node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			Node3D *child = Object::cast_to<Node3D>(p_node->get_child(i));
			if (child) {
				children_global_transforms.push_back(child->get_global_transform());
				node3d_children.push_back(child);
			}
		}
	}

	if (p_local) {
		p_node->set_transform(p_transform);
	} else {
		p_node->set_global_transform(p_transform);
	}

	if (preserve_children) {
		for (int i = 0; i < node3d_children.size(); i++) {
			node3d_children[i]->set_global_transform(children_global_transforms[i]);
		}
	}
}

Transform3D Node3DEditorViewport::_compute_transform(TransformMode p_mode, const Transform3D &p_original, const Transform3D &p_original_local, Vector3 p_motion, double p_extra, bool p_local, bool p_orthogonal, bool p_view_axis) {
	switch (p_mode) {
		case TRANSFORM_SCALE: {
			if (spatial_editor->is_snap_enabled()) {
				p_motion.snapf(p_extra);
			}
			Transform3D s;
			if (p_local) {
				s.basis = p_original_local.basis.scaled_local(p_motion + Vector3(1, 1, 1));
				s.origin = p_original_local.origin;
			} else {
				s.basis.scale(p_motion + Vector3(1, 1, 1));
				Transform3D base = Transform3D(Basis(), _edit.center);
				s = base * (s * (base.inverse() * p_original));

				// Recalculate orthogonalized scale without moving origin.
				if (p_orthogonal) {
					s.basis = p_original.basis.scaled_orthogonal(p_motion + Vector3(1, 1, 1));
				}
			}

			return s;
		}
		case TRANSFORM_TRANSLATE: {
			if (spatial_editor->is_snap_enabled()) {
				p_motion.snapf(p_extra);
			}

			if (p_local) {
				return p_original_local.translated_local(p_motion);
			}

			return p_original.translated(p_motion);
		}
		case TRANSFORM_ROTATE: {
			Transform3D r;

			Basis parent_global_basis = p_original.basis * p_original_local.basis.inverse();

			Vector3 axis;
			if (p_local && !p_view_axis) {
				axis = p_original_local.basis.xform(p_motion);
			} else {
				axis = parent_global_basis.xform_inv(p_motion);
			}

			if (p_local) {
				r.basis = Basis(axis.normalized(), p_extra) * p_original_local.basis;
				r.origin = p_original_local.origin;
			} else {
				r.basis = parent_global_basis * Basis(axis.normalized(), p_extra) * p_original_local.basis;
				r.origin = Basis(p_motion, p_extra).xform(p_original.origin - _edit.center) + _edit.center;
			}

			return r;
		}
		default: {
			ERR_FAIL_V_MSG(Transform3D(), "Invalid mode in '_compute_transform'");
		}
	}
}

void Node3DEditorViewport::_reset_transform(TransformType p_type) {
	List<Node *> selection = editor_selection->get_full_selected_node_list();
	if (selection.is_empty()) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Reset Transform"));
	for (Node *node : selection) {
		Node3D *sp = Object::cast_to<Node3D>(node);
		if (!sp) {
			continue;
		}

		switch (p_type) {
			case TransformType::POSITION:
				undo_redo->add_undo_method(sp, "set_position", sp->get_position());
				undo_redo->add_do_method(sp, "set_position", Vector3());
				break;
			case TransformType::ROTATION:
				undo_redo->add_undo_method(sp, "set_rotation", sp->get_rotation());
				undo_redo->add_do_method(sp, "set_rotation", Vector3());
				break;
			case TransformType::SCALE:
				undo_redo->add_undo_method(sp, "set_scale", sp->get_scale());
				undo_redo->add_do_method(sp, "set_scale", Vector3(1, 1, 1));
				break;
		}
	}
	undo_redo->commit_action();
}

void Node3DEditorViewport::_surface_mouse_enter() {
	if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
		return;
	}

	if (!surface->has_focus() && (!get_viewport()->gui_get_focus_owner() || !get_viewport()->gui_get_focus_owner()->is_text_field())) {
		surface->grab_focus();
	}
}

void Node3DEditorViewport::_surface_mouse_exit() {
	_remove_preview_node();
	_reset_preview_material();
	_remove_preview_material();
}

void Node3DEditorViewport::_surface_focus_enter() {
	// Focus is what picks the active view among several: it follows the click
	// that the user made, where visibility only says a pane is on screen.
	spatial_editor->make_active();
	view_display_menu->set_disable_shortcuts(false);
}

void Node3DEditorViewport::_surface_focus_exit() {
	view_display_menu->set_disable_shortcuts(true);
}

bool Node3DEditorViewport::_is_node_locked(const Node *p_node) const {
	return p_node->get_meta("_edit_lock_", false);
}

void Node3DEditorViewport::_list_select(Ref<InputEventMouseButton> b) {
	Vector<_RayResult> potential_selection_results;
	_find_items_at_pos(b->get_position(), potential_selection_results, b->is_alt_pressed());

	Node *edited_scene = get_edited_scene();

	// Filter to a list of nodes which include either the edited scene or nodes directly owned by the edited scene.
	// If a node has an invalid owner, recursively check their parents until a valid node is found.
	for (int i = 0; i < potential_selection_results.size(); i++) {
		Node3D *node = potential_selection_results[i].item;
		while (true) {
			if (node == nullptr || node == edited_scene->get_parent()) {
				break;
			} else {
				Node *node_owner = node->get_owner();
				if (node == edited_scene || node_owner == edited_scene || (node_owner != nullptr && edited_scene->is_editable_instance(node_owner))) {
					if (!selection_results.has(node)) {
						selection_results.append(node);
					}
					break;
				}
			}
			node = Object::cast_to<Node3D>(node->get_parent());
		}
	}

	clicked_wants_append = b->is_shift_pressed();

	if (selection_results.size() == 1) {
		clicked = selection_results[0]->get_instance_id();
		selection_results.clear();

		if (clicked.is_valid()) {
			_select_clicked(b->is_alt_pressed());
		}
	} else if (!selection_results.is_empty()) {
		NodePath root_path = get_edited_scene()->get_path();
		StringName root_name = root_path.get_name(root_path.get_name_count() - 1);
		int icon_max_width = EditorNode::get_singleton()->get_editor_theme()->get_constant(SNAME("class_icon_size"), EditorStringName(Editor));

		for (int i = 0; i < selection_results.size(); i++) {
			Node3D *spat = selection_results[i];

			Ref<Texture2D> icon = EditorNode::get_singleton()->get_object_icon(spat);

			String node_path = "/" + root_name + "/" + String(root_path.rel_path_to(spat->get_path()));

			int locked = 0;
			if (_is_node_locked(spat)) {
				locked = 1;
			} else {
				Node *ed_scene = get_edited_scene();
				Node *node = spat;

				while (node && node != ed_scene->get_parent()) {
					Node3D *selected_tmp = Object::cast_to<Node3D>(node);
					if (selected_tmp && node->has_meta("_edit_group_")) {
						locked = 2;
					}
					node = node->get_parent();
				}
			}

			String suffix;
			if (locked == 1) {
				suffix = " (" + TTR("Locked") + ")";
			} else if (locked == 2) {
				suffix = " (" + TTR("Grouped") + ")";
			}
			selection_menu->add_item((String)spat->get_name() + suffix);
			selection_menu->set_item_icon(i, icon);
			selection_menu->set_item_icon_max_width(i, icon_max_width);
			selection_menu->set_item_metadata(i, node_path);
			selection_menu->set_item_tooltip(i, String(spat->get_name()) + "\nType: " + spat->get_class() + "\nPath: " + node_path);
		}

		selection_results_menu = selection_results;
		selection_menu->set_position(get_screen_position() + b->get_position());
		selection_menu->reset_size();
		selection_menu->popup();
	}
}

// Helper function to redirect mouse events to the active freelook viewport
static bool _redirect_freelook_input(const Ref<InputEvent> &p_event, Node3DEditorViewport *p_exclude_viewport = nullptr) {
	if (Input::get_singleton()->get_mouse_mode() != Input::MouseMode::MOUSE_MODE_CAPTURED) {
		return false;
	}

	// Whichever view holds freelook, not whichever is active: the mouse is
	// captured by one viewport at a time, and it need not be in the active one.
	Node3DEditorViewport *freelook_vp = nullptr;
	for (Node3DEditor *editor : Node3DEditor::get_instances()) {
		if (editor->get_freelook_viewport()) {
			freelook_vp = editor->get_freelook_viewport();
			break;
		}
	}
	if (!freelook_vp || freelook_vp == p_exclude_viewport) {
		return false;
	}

	Ref<InputEventMouse> mouse_event = p_event;
	if (!mouse_event.is_valid()) {
		return false;
	}

	Control *target_surface = freelook_vp->get_surface();

	target_surface->emit_signal(SceneStringName(gui_input), p_event);
	return true;
}

// This is only active during instant transforms,
// to capture and wrap mouse events outside the control.
void Node3DEditorViewport::input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(!_edit.instant);
	Ref<InputEventMouseMotion> m = p_event;

	if (m.is_valid()) {
		_edit.mouse_pos += view_3d_controller->get_warped_mouse_motion(p_event, surface->get_global_rect());
		update_transform(_get_key_modifier(m) == Key::SHIFT);
	}
}

void Node3DEditorViewport::_sinput(const Ref<InputEvent> &p_event) {
	const Ref<InputEventKey> k = p_event;

	if (k.is_valid() && k->is_pressed()) {
		const Key code = view_3d_controller->emulate_numpad_key(k->get_physical_keycode());
		if (code != k->get_physical_keycode()) {
			k->set_keycode(code);
		}
	}

	if (get_viewport()->gui_get_drag_data()) {
		// Disable all input actions during drag-and-drop.
		return;
	}

	if (k.is_valid()) {
		Ref<InputEventKey> k_no_shift = k->duplicate();
		k_no_shift->set_shift_pressed(false);
		if (!vertex_snap_mode && !vertex_snap_dragging && k->is_pressed() && _edit.mode == TRANSFORM_NONE && ED_IS_SHORTCUT("spatial_editor/vertex_snap", k_no_shift)) {
			vertex_snap_mode = true;
			vertex_snap_keycode = k->get_physical_keycode() != Key::NONE ? k->get_physical_keycode() : k->get_keycode();
			_disable_follow_mode();
			set_message(TTR("Vertex Snap"));
			_vertex_snap_update_source(_edit.mouse_pos);
			surface->queue_redraw();
		} else if (vertex_snap_mode && !k->is_pressed() && k->get_physical_keycode() == vertex_snap_keycode) {
			vertex_snap_mode = false;
			vertex_snap_keycode = Key::NONE;
			if (!vertex_snap_dragging) {
				vertex_snap_has_source = false;
				vertex_snap_has_target = false;
				set_message("");
				surface->queue_redraw();
			}
		} else if ((vertex_snap_mode || vertex_snap_dragging) && k->is_pressed() && k->get_keycode() == Key::ESCAPE) {
			if (vertex_snap_dragging) {
				_vertex_snap_cancel();
			} else {
				vertex_snap_has_source = false;
				surface->queue_redraw();
			}
			vertex_snap_mode = false;
			vertex_snap_keycode = Key::NONE;
			set_message("");
			return;
		}
	}

	if (_redirect_freelook_input(p_event, this)) {
		return;
	}

	{
		Ref<InputEventMouseButton> vb = p_event;
		if ((vertex_snap_mode || vertex_snap_dragging) && vb.is_valid()) {
			if (vb->get_button_index() == MouseButton::RIGHT && vb->is_pressed() && vertex_snap_dragging) {
				_vertex_snap_cancel();
				return;
			}
			if (vb->get_button_index() == MouseButton::LEFT) {
				if (vb->is_pressed()) {
					const List<Node *> &selection = editor_selection->get_top_selected_node_list();
					bool use_origin_snap = spatial_editor->is_vertex_snap_origin_mode();

					Node3D *selected = spatial_editor->get_single_selected_node();
					Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;
					bool has_subgizmos = se && se->gizmo.is_valid() && !se->subgizmos.is_empty();

					if (!use_origin_snap) {
						bool has_snap_target = false;
						for (Node *E : selection) {
							if (_node_has_snap_target(E, spatial_editor->is_vertex_snap_use_collision())) {
								has_snap_target = true;
								break;
							}
						}
						if (!has_snap_target) {
							use_origin_snap = true;
						}
					}

					if (use_origin_snap || !vertex_snap_has_source) {
						_vertex_snap_update_source(vb->get_position());
					}

					if (vertex_snap_has_source && !selection.is_empty()) {
						vertex_snap_original_positions.clear();
						for (Node *E : selection) {
							Node3D *sp = Object::cast_to<Node3D>(E);
							if (sp) {
								vertex_snap_original_positions[sp->get_instance_id()] = sp->get_global_position();
							}
						}

						if (!use_origin_snap) {
							vertex_snap_dragging = true;
							Vector3 cam_normal = camera->get_global_transform().basis.get_column(2);
							vertex_snap_drag_plane = Plane(cam_normal, vertex_snap_source);
						} else {
							if (has_subgizmos) {
								Transform3D gi = selected->get_global_transform().affine_inverse();
								Vector3 local_target = gi.xform(vertex_snap_source);

								Vector<int> ids;
								Vector<Transform3D> restores;
								for (const KeyValue<int, Transform3D> &GE : se->subgizmos) {
									Transform3D original_xform = se->gizmo->get_subgizmo_transform(GE.key);
									ids.push_back(GE.key);
									restores.push_back(original_xform);

									Transform3D snapped_xform = original_xform;
									snapped_xform.origin = local_target;
									se->gizmo->set_subgizmo_transform(GE.key, snapped_xform);
								}
								se->gizmo->commit_subgizmos(ids, restores, false);
							} else {
								EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
								undo_redo->create_action(TTR("Vertex Snap"));
								for (const KeyValue<ObjectID, Vector3> &E2 : vertex_snap_original_positions) {
									Node3D *node = ObjectDB::get_instance<Node3D>(E2.key);
									if (node) {
										node->set_global_position(vertex_snap_source);
										undo_redo->add_do_method(node, "set_global_position", vertex_snap_source);
										undo_redo->add_undo_method(node, "set_global_position", E2.value);
									}
								}
								undo_redo->commit_action(false);
							}
							vertex_snap_original_positions.clear();
							spatial_editor->update_transform_gizmo();
							_reset_follow_mode_count();
						}
						surface->queue_redraw();
					}
				} else {
					if (vertex_snap_dragging) {
						_vertex_snap_commit();
					}
				}
				return;
			}
		}
	}

	EditorPlugin::AfterGUIInput after = EditorPlugin::AFTER_GUI_INPUT_PASS;
	{
		EditorNode *en = EditorNode::get_singleton();
		Camera3D *input_camera = previewing ? previewing : camera;

		switch (en->get_editor_plugins_force_input_forwarding()->forward_3d_gui_input(input_camera, p_event, true)) {
			case EditorPlugin::AFTER_GUI_INPUT_PASS: {
				// Continue processing.
			} break;

			case EditorPlugin::AFTER_GUI_INPUT_STOP: {
				return; // Stop processing.
			} break;

			case EditorPlugin::AFTER_GUI_INPUT_CUSTOM: {
				after = EditorPlugin::AFTER_GUI_INPUT_CUSTOM;
			} break;
		}

		switch (en->get_editor_plugins_over()->forward_3d_gui_input(input_camera, p_event, false)) {
			case EditorPlugin::AFTER_GUI_INPUT_PASS: {
				// Continue processing.
			} break;

			case EditorPlugin::AFTER_GUI_INPUT_STOP: {
				return; // Stop processing.
			} break;

			case EditorPlugin::AFTER_GUI_INPUT_CUSTOM: {
				after = EditorPlugin::AFTER_GUI_INPUT_CUSTOM;
			} break;
		}
	}

	// Several parts of the 3D navigation are handled here.
	bool was_navigating = view_3d_controller->is_navigating();
	view_3d_controller->gui_input(p_event, surface->get_global_rect());
	if (was_navigating && !view_3d_controller->is_navigating()) {
		return;
	}

	Ref<InputEventMouseButton> b = p_event;

	if (b.is_valid()) {
		emit_signal(SNAME("clicked"));

		View3DController::NavigationMouseButton orbit_mouse_preference = (View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/orbit_mouse_button").operator int();
		View3DController::NavigationMouseButton pan_mouse_preference = (View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/pan_mouse_button").operator int();
		View3DController::NavigationMouseButton zoom_mouse_preference = (View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/zoom_mouse_button").operator int();

		switch (b->get_button_index()) {
			case MouseButton::RIGHT: {
				if (b->is_pressed()) {
					if (_edit.gizmo.is_valid()) {
						// Restore.
						_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_handle_secondary, _edit.gizmo_initial_value, true);
						_edit.gizmo = Ref<EditorNode3DGizmo>();
						set_message("");
					}

					if (_edit.mode == TRANSFORM_NONE) {
						if (orbit_mouse_preference == View3DController::NAV_MOUSE_BUTTON_RIGHT && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_2")) {
							break;
						} else if (pan_mouse_preference == View3DController::NAV_MOUSE_BUTTON_RIGHT && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_2")) {
							break;
						} else if (zoom_mouse_preference == View3DController::NAV_MOUSE_BUTTON_RIGHT && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_2")) {
							break;
						}
					}

					if (b->is_alt_pressed()) {
						_list_select(b);
						return;
					}

					if (_edit.mode != TRANSFORM_NONE) {
						cancel_transform();
						break;
					}

					const Key mod = _get_key_modifier(b);
					if (!view_3d_controller->is_orthogonal() && !(previewing && !pilot_preview_enabled)) {
						if (mod == _get_key_modifier_setting("editors/3d/freelook/freelook_activation_modifier")) {
							view_3d_controller->set_freelook_enabled(true);
						}
					}
				} else {
					view_3d_controller->set_freelook_enabled(false);
				}

				if (view_3d_controller->is_freelook_enabled() && !surface->has_focus()) {
					// Focus usually doesn't trigger on right-click, but in case of freelook it should,
					// otherwise using keyboard navigation would misbehave
					surface->grab_focus();
				}

			} break;
			case MouseButton::MIDDLE: {
				if (b->is_pressed() && _edit.mode != TRANSFORM_NONE) {
					if (orbit_mouse_preference == View3DController::NAV_MOUSE_BUTTON_MIDDLE && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_2")) {
						break;
					} else if (pan_mouse_preference == View3DController::NAV_MOUSE_BUTTON_MIDDLE && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_2")) {
						break;
					} else if (zoom_mouse_preference == View3DController::NAV_MOUSE_BUTTON_MIDDLE && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_2")) {
						break;
					}

					switch (_edit.plane) {
						case TRANSFORM_VIEW: {
							_edit.plane = TRANSFORM_X_AXIS;
							set_message(TTR("X-Axis Transform."), 2);
							view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
						} break;
						case TRANSFORM_X_AXIS: {
							_edit.plane = TRANSFORM_Y_AXIS;
							set_message(TTR("Y-Axis Transform."), 2);

						} break;
						case TRANSFORM_Y_AXIS: {
							_edit.plane = TRANSFORM_Z_AXIS;
							set_message(TTR("Z-Axis Transform."), 2);

						} break;
						case TRANSFORM_Z_AXIS: {
							_edit.plane = TRANSFORM_VIEW;
							// TRANSLATORS: This refers to the transform of the view plane.
							set_message(TTR("View Plane Transform."), 2);

						} break;
						case TRANSFORM_YZ:
						case TRANSFORM_XZ:
						case TRANSFORM_XY: {
						} break;
					}
				}
			} break;
			case MouseButton::LEFT: {
				if (b->is_pressed()) {
					clicked_wants_append = b->is_shift_pressed();

					if (_edit.mode != TRANSFORM_NONE && (_edit.instant || collision_reposition)) {
						commit_transform();
						break; // just commit the edit, stop processing the event so we don't deselect the object
					}
					if (orbit_mouse_preference == View3DController::NAV_MOUSE_BUTTON_LEFT && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_orbit_modifier_2")) {
						break;
					} else if (pan_mouse_preference == View3DController::NAV_MOUSE_BUTTON_LEFT && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_pan_modifier_2")) {
						break;
					} else if (zoom_mouse_preference == View3DController::NAV_MOUSE_BUTTON_LEFT && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_1") && _is_nav_modifier_pressed("spatial_editor/viewport_zoom_modifier_2")) {
						break;
					}

					if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_RULER) {
						get_scene_root()->add_child(ruler);
						collision_reposition = true;
						break;
					}

					if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_LIST_SELECT) {
						_list_select(b);
						break;
					}

					_edit.mouse_pos = b->get_position();
					_edit.original_mouse_pos = b->get_position();
					_edit.mode = TRANSFORM_NONE;
					_edit.original = spatial_editor->get_gizmo_transform(); // To prevent to break when flipping with scale.

					bool can_select_gizmos = spatial_editor->get_single_selected_node();

					{
						int idx = view_display_menu->get_popup()->get_item_index(VIEW_GIZMOS);
						int idx2 = view_display_menu->get_popup()->get_item_index(VIEW_TRANSFORM_GIZMO);
						can_select_gizmos = can_select_gizmos && view_display_menu->get_popup()->is_item_checked(idx);
						transform_gizmo_visible = view_display_menu->get_popup()->is_item_checked(idx2);
					}

					// Gizmo handles
					if (can_select_gizmos) {
						Vector<Ref<Node3DGizmo>> gizmos = spatial_editor->get_single_selected_node()->get_gizmos();

						bool intersected_handle = false;
						for (int i = 0; i < gizmos.size(); i++) {
							Ref<EditorNode3DGizmo> seg = gizmos[i];

							if (seg.is_null()) {
								continue;
							}

							int gizmo_handle = -1;
							bool gizmo_secondary = false;
							seg->handles_intersect_ray(camera, _edit.mouse_pos, b->is_shift_pressed(), gizmo_handle, gizmo_secondary);
							if (gizmo_handle != -1) {
								_edit.gizmo = seg;
								seg->begin_handle_action(gizmo_handle, gizmo_secondary);
								_edit.gizmo_handle = gizmo_handle;
								_edit.gizmo_handle_secondary = gizmo_secondary;
								_edit.gizmo_initial_value = seg->get_handle_value(gizmo_handle, gizmo_secondary);
								intersected_handle = true;
								break;
							}
						}

						if (intersected_handle) {
							break;
						}
					}

					// Transform gizmo
					if (transform_gizmo_visible && _transform_gizmo_select(_edit.mouse_pos)) {
						break;
					}

					// Subgizmos
					if (can_select_gizmos) {
						Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(spatial_editor->get_single_selected_node());
						Vector<Ref<Node3DGizmo>> gizmos = spatial_editor->get_single_selected_node()->get_gizmos();

						bool intersected_subgizmo = false;
						for (int i = 0; i < gizmos.size(); i++) {
							Ref<EditorNode3DGizmo> seg = gizmos[i];

							if (seg.is_null()) {
								continue;
							}

							int subgizmo_id = seg->subgizmos_intersect_ray(camera, _edit.mouse_pos);
							if (subgizmo_id != -1) {
								ERR_CONTINUE(!se);
								if (b->is_shift_pressed()) {
									if (se->subgizmos.has(subgizmo_id)) {
										se->subgizmos.erase(subgizmo_id);
									} else {
										se->subgizmos.insert(subgizmo_id, seg->get_subgizmo_transform(subgizmo_id));
									}
								} else {
									se->subgizmos.clear();
									se->subgizmos.insert(subgizmo_id, seg->get_subgizmo_transform(subgizmo_id));
								}

								if (se->subgizmos.is_empty()) {
									se->gizmo = Ref<EditorNode3DGizmo>();
								} else {
									se->gizmo = seg;
								}

								seg->redraw();
								spatial_editor->update_transform_gizmo();
								_reset_follow_mode_count();
								intersected_subgizmo = true;
								break;
							}
						}

						if (intersected_subgizmo) {
							break;
						}
					}

					clicked = ObjectID();

					bool node_selected = get_selected_count() > 0;

					if (after != EditorPlugin::AFTER_GUI_INPUT_CUSTOM) {
						// Single item selection.
						clicked = _select_ray(b->get_position());

						if (clicked.is_valid() && !editor_selection->is_selected(ObjectDB::get_instance<Node>(clicked))) {
							if (!node_selected || (!b->is_alt_pressed() && !(spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM && b->is_command_or_control_pressed()))) {
								selection_in_progress = true;
								break;
							}
						}

						if (clicked.is_null()) {
							if (node_selected) {
								TransformMode mode = TRANSFORM_NONE;

								if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM) {
									if (b->is_command_or_control_pressed()) {
										mode = TRANSFORM_ROTATE;
									} else if (b->is_alt_pressed()) {
										mode = TRANSFORM_TRANSLATE;
									}
								} else if (b->is_alt_pressed()) {
									switch (spatial_editor->get_tool_mode()) {
										case Node3DEditor::TOOL_MODE_ROTATE:
											mode = TRANSFORM_ROTATE;
											break;
										case Node3DEditor::TOOL_MODE_MOVE:
											mode = TRANSFORM_TRANSLATE;
											break;
										case Node3DEditor::TOOL_MODE_SCALE:
											mode = TRANSFORM_SCALE;
											break;
										default:
											break;
									}
								}

								if (mode != TRANSFORM_NONE) {
									begin_transform(mode, false);
									break;
								}
							}

							if (!previewing) {
								// Default to region select.
								view_3d_controller->cursor.region_select = true;
								view_3d_controller->cursor.region_begin = b->get_position();
								view_3d_controller->cursor.region_end = b->get_position();
							}
							break;
						}
					}

					if (clicked.is_valid() && !clicked_wants_append) {
						bool is_clicked_node_selected = editor_selection->is_selected(ObjectDB::get_instance<Node>(clicked));
						TransformMode mode = TRANSFORM_NONE;

						switch (spatial_editor->get_tool_mode()) {
							case Node3DEditor::TOOL_MODE_TRANSFORM:
								if (b->is_command_or_control_pressed() && node_selected) {
									mode = TRANSFORM_ROTATE;
								} else if (b->is_alt_pressed() && node_selected) {
									mode = TRANSFORM_TRANSLATE;
								} else if (is_clicked_node_selected) {
									mode = TRANSFORM_TRANSLATE;
								}
								break;
							case Node3DEditor::TOOL_MODE_ROTATE:
								if (is_clicked_node_selected || (b->is_alt_pressed() && node_selected)) {
									mode = TRANSFORM_ROTATE;
								}
								break;
							case Node3DEditor::TOOL_MODE_MOVE:
								if (is_clicked_node_selected || (b->is_alt_pressed() && node_selected)) {
									mode = TRANSFORM_TRANSLATE;
								}
								break;
							case Node3DEditor::TOOL_MODE_SCALE:
								if (is_clicked_node_selected || (b->is_alt_pressed() && node_selected)) {
									mode = TRANSFORM_SCALE;
								}
								break;
							default:
								break;
						}

						if (mode != TRANSFORM_NONE) {
							begin_transform(mode, false);
							break;
						}
					}

					surface->queue_redraw();
				} else {
					if (ruler->is_inside_tree()) {
						get_scene_root()->remove_child(ruler);
						ruler_start_point->set_visible(false);
						ruler_end_point->set_visible(false);
						ruler_label->set_visible(false);
						ruler_label_x->set_visible(false);
						ruler_label_y->set_visible(false);
						ruler_label_z->set_visible(false);
						collision_reposition = false;
						break;
					}

					if (_edit.gizmo.is_valid()) {
						// Certain gizmo plugins should be able to commit handles without dragging them.
						if (_edit.original_mouse_pos != _edit.mouse_pos || _edit.gizmo->get_plugin()->can_commit_handle_on_click()) {
							_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_handle_secondary, _edit.gizmo_initial_value, false);
						}
						Node3D *selected_node = spatial_editor->get_single_selected_node();
						if (selected_node) {
							selected_node->update_gizmos();
						}
						_edit.gizmo = Ref<EditorNode3DGizmo>();
						set_message("");
						break;
					}

					if (after != EditorPlugin::AFTER_GUI_INPUT_CUSTOM) {
						selection_in_progress = false;

						if (clicked.is_valid() && _edit.mode == TRANSFORM_NONE) {
							_select_clicked(false);
						} else if (view_3d_controller->cursor.region_select) {
							_select_region();
							surface->queue_redraw();
						}

						movement_threshold_passed = false;
						view_3d_controller->cursor.region_select = false;
					}

					if (!_edit.instant && _edit.mode != TRANSFORM_NONE) {
						Node3D *selected = spatial_editor->get_single_selected_node();
						Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;

						if (se && se->gizmo.is_valid()) {
							Vector<int> ids;
							Vector<Transform3D> restore;

							for (const KeyValue<int, Transform3D> &GE : se->subgizmos) {
								ids.push_back(GE.key);
								restore.push_back(GE.value);
							}

							se->gizmo->commit_subgizmos(ids, restore, false);
							finish_transform();
						} else {
							if (_edit.original_mouse_pos != _edit.mouse_pos) {
								commit_transform();
							}
						}
						_edit.mode = TRANSFORM_NONE;
						set_message("");
						spatial_editor->update_transform_gizmo();
					}
				}

			} break;
			default:
				break;
		}
	}

	Ref<InputEventMouseMotion> m = p_event;

	// Instant transforms process mouse motion in input() to handle wrapping.
	if (m.is_valid() && !_edit.instant) {
		_edit.mouse_pos = m->get_position();

		if (vertex_snap_mode || vertex_snap_dragging) {
			if (!vertex_snap_dragging) {
				_vertex_snap_update_source(m->get_position());
			} else {
				const float vertex_snap_threshold = VERTEX_SNAP_THRESHOLD * EDSCALE;
				bool has_displacement = false;
				Vector3 displacement;

				Vector3 target;
				if (_find_closest_vertex_in_scene(m->get_position(), vertex_snap_threshold, target, &vertex_snap_original_positions)) {
					vertex_snap_target = target;
					vertex_snap_has_target = true;
					displacement = target - vertex_snap_source;
					has_displacement = true;
					set_message(TTR("Vertex Snap (Snapped)"));
				} else {
					vertex_snap_has_target = false;
					Vector3 ray_pos = get_ray_pos(m->get_position());
					Vector3 ray_dir = get_ray(m->get_position());
					Vector3 intersection;
					if (vertex_snap_drag_plane.intersects_ray(ray_pos, ray_dir, &intersection)) {
						displacement = intersection - vertex_snap_source;
						has_displacement = true;
					}
					set_message(TTR("Vertex Snap"));
				}

				if (has_displacement) {
					for (const KeyValue<ObjectID, Vector3> &E : vertex_snap_original_positions) {
						Node3D *node = ObjectDB::get_instance<Node3D>(E.key);
						if (node) {
							node->set_global_position(E.value + displacement);
						}
					}
				}
			}
			surface->queue_redraw();
			return;
		}

		if (!view_3d_controller->is_freelook_enabled() && spatial_editor->get_single_selected_node()) {
			Vector<Ref<Node3DGizmo>> gizmos = spatial_editor->get_single_selected_node()->get_gizmos();

			Ref<EditorNode3DGizmo> found_gizmo;
			int found_handle = -1;
			bool found_handle_secondary = false;

			for (int i = 0; i < gizmos.size(); i++) {
				Ref<EditorNode3DGizmo> seg = gizmos[i];
				if (seg.is_null()) {
					continue;
				}

				seg->handles_intersect_ray(camera, _edit.mouse_pos, false, found_handle, found_handle_secondary);

				if (found_handle != -1) {
					found_gizmo = seg;
					break;
				}
			}

			if (found_gizmo.is_valid()) {
				spatial_editor->select_gizmo_highlight_axis(-1);
			}

			bool current_hover_handle_secondary = false;
			int current_hover_handle = spatial_editor->get_current_hover_gizmo_handle(current_hover_handle_secondary);
			if (found_gizmo != spatial_editor->get_current_hover_gizmo() || found_handle != current_hover_handle || found_handle_secondary != current_hover_handle_secondary) {
				spatial_editor->set_current_hover_gizmo(found_gizmo);
				spatial_editor->set_current_hover_gizmo_handle(found_handle, found_handle_secondary);
				spatial_editor->get_single_selected_node()->update_gizmos();
			}
		}

		if (!view_3d_controller->is_freelook_enabled() && transform_gizmo_visible && spatial_editor->get_current_hover_gizmo().is_null() && !m->get_button_mask().has_flag(MouseButtonMask::LEFT) && _edit.gizmo.is_null()) {
			_transform_gizmo_select(_edit.mouse_pos, true);
		}

		if (_edit.gizmo.is_valid()) {
			_edit.gizmo->set_handle(_edit.gizmo_handle, _edit.gizmo_handle_secondary, camera, m->get_position());
			Variant v = _edit.gizmo->get_handle_value(_edit.gizmo_handle, _edit.gizmo_handle_secondary);
			String n = _edit.gizmo->get_handle_name(_edit.gizmo_handle, _edit.gizmo_handle_secondary);
			set_message(n + ": " + String(v));

		} else if (m->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
			movement_threshold_passed = _edit.original_mouse_pos.distance_to(_edit.mouse_pos) > 8 * EDSCALE;

			if ((selection_in_progress || clicked_wants_append || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_SELECT) && movement_threshold_passed && clicked.is_valid() && !previewing) {
				view_3d_controller->cursor.region_select = true;
				view_3d_controller->cursor.region_begin = _edit.original_mouse_pos;
				clicked = ObjectID();
			}

			if (view_3d_controller->cursor.region_select) {
				view_3d_controller->cursor.region_end = m->get_position();
				surface->queue_redraw();
				return;
			}

			if (clicked.is_valid() && movement_threshold_passed && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_MOVE)) {
				bool is_select_mode = (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM);
				bool is_clicked_selected = editor_selection->is_selected(ObjectDB::get_instance<Node>(clicked));

				if (_edit.mode == TRANSFORM_NONE && (is_select_mode || is_clicked_selected)) {
					_compute_edit(_edit.original_mouse_pos);
					clicked = ObjectID();
					_edit.mode = TRANSFORM_TRANSLATE;
				}
			}

			if (_edit.mode == TRANSFORM_NONE || _edit.numeric_input != 0 || _edit.numeric_next_decimal != 0) {
				return;
			}

			if (!selection_in_progress) {
				update_transform(_get_key_modifier(m) == Key::SHIFT);
			}
		}
	}

	if (k.is_valid()) {
		if (!k->is_pressed()) {
			return;
		}

		if (_edit.instant) {
			// In a Blender-style transform, numbers set the magnitude of the transform.
			// E.g. pressing g4.5x means "translate 4.5 units along the X axis".
			bool processed = true;
			Key keycode = k->get_keycode();
			Key physical_keycode = k->get_physical_keycode();

			// Use physical keycode for main keyboard numbers (for non-QWERTY layouts like AZERTY)
			// but regular keycode for numpad numbers.
			if ((physical_keycode >= Key::KEY_0 && physical_keycode <= Key::KEY_9) || (keycode >= Key::KP_0 && keycode <= Key::KP_9)) {
				uint32_t value;
				if (physical_keycode >= Key::KEY_0 && physical_keycode <= Key::KEY_9) {
					value = uint32_t(physical_keycode - Key::KEY_0);
				} else {
					value = uint32_t(keycode - Key::KP_0);
				}

				if (_edit.numeric_next_decimal < 0) {
					_edit.numeric_input = _edit.numeric_input + value * Math::pow(10.0, _edit.numeric_next_decimal--);
				} else {
					_edit.numeric_input = _edit.numeric_input * 10 + value;
				}
				update_transform_numeric();
			} else if (keycode == Key::MINUS || keycode == Key::KP_SUBTRACT) {
				_edit.numeric_negate = !_edit.numeric_negate;
				update_transform_numeric();
			} else if (keycode == Key::PERIOD || physical_keycode == Key::KP_PERIOD) {
				// Use physical keycode for KP_PERIOD to ensure numpad period works consistently
				// across different keyboard layouts (like nordic keyboards).
				if (_edit.numeric_next_decimal == 0) {
					_edit.numeric_next_decimal = -1;
				}
			} else if (keycode == Key::ENTER || keycode == Key::KP_ENTER || keycode == Key::SPACE) {
				commit_transform();
			} else {
				processed = false;
			}

			if (processed) {
				// Ignore mouse inputs once we receive a numeric input.
				set_process_input(false);
				accept_event();
				return;
			}
		}

		Ref<InputEvent> event_mod = p_event;
		const Key code = view_3d_controller->emulate_numpad_key(k->get_physical_keycode());
		if (code != k->get_physical_keycode()) {
			event_mod = p_event->duplicate();
			Ref<InputEventKey> k_mod = event_mod;
			k_mod->set_keycode(code);
		}

		if (_edit.mode == TRANSFORM_NONE) {
			if (_edit.gizmo.is_null() && view_3d_controller->is_freelook_enabled() && k->get_keycode() == Key::ESCAPE) {
				view_3d_controller->set_freelook_enabled(false);
				return;
			}

			if (_edit.gizmo.is_valid() && (k->get_keycode() == Key::ESCAPE || k->get_keycode() == Key::BACKSPACE)) {
				// Restore.
				_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_handle_secondary, _edit.gizmo_initial_value, true);
				_edit.gizmo = Ref<EditorNode3DGizmo>();
				set_message("");
			}
			if (k->get_keycode() == Key::ESCAPE && !view_3d_controller->cursor.region_select && !k->is_echo()) {
				_clear_selected();
				return;
			}
		} else {
			// We're actively transforming, handle keys specially
			TransformPlane new_plane = TRANSFORM_VIEW;
			if (ED_IS_SHORTCUT("spatial_editor/lock_transform_x", event_mod)) {
				new_plane = TRANSFORM_X_AXIS;
			} else if (ED_IS_SHORTCUT("spatial_editor/lock_transform_y", event_mod)) {
				new_plane = TRANSFORM_Y_AXIS;
			} else if (ED_IS_SHORTCUT("spatial_editor/lock_transform_z", event_mod)) {
				new_plane = TRANSFORM_Z_AXIS;
			} else if (_edit.mode != TRANSFORM_ROTATE) { // rotating on a plane doesn't make sense
				if (ED_IS_SHORTCUT("spatial_editor/lock_transform_yz", event_mod)) {
					new_plane = TRANSFORM_YZ;
				} else if (ED_IS_SHORTCUT("spatial_editor/lock_transform_xz", event_mod)) {
					new_plane = TRANSFORM_XZ;
				} else if (ED_IS_SHORTCUT("spatial_editor/lock_transform_xy", event_mod)) {
					new_plane = TRANSFORM_XY;
				}
			}

			if (new_plane != TRANSFORM_VIEW) {
				if (new_plane != _edit.plane) {
					// lock me once and get a global constraint
					_edit.plane = new_plane;
					spatial_editor->set_local_coords_enabled(false);
				} else if (!spatial_editor->are_local_coords_enabled()) {
					// lock me twice and get a local constraint
					spatial_editor->set_local_coords_enabled(true);
				} else {
					// lock me thrice and we're back where we started
					_edit.plane = TRANSFORM_VIEW;
					spatial_editor->set_local_coords_enabled(false);
				}
				if (_edit.numeric_input != 0 || _edit.numeric_next_decimal != 0) {
					update_transform_numeric();
				} else {
					update_transform(Input::get_singleton()->is_key_pressed(Key::SHIFT));
				}
				accept_event();
				return;
			}
		}
		if (_edit.mode == TRANSFORM_NONE && k->is_pressed() && !k->is_echo() && _open_pie_for(event_mod, k->get_keycode())) {
			accept_event();
			return;
		}
		if (ED_IS_SHORTCUT("spatial_editor/bottom_view", event_mod)) {
			_menu_option(VIEW_BOTTOM);
		}
		if (ED_IS_SHORTCUT("spatial_editor/top_view", event_mod)) {
			_menu_option(VIEW_TOP);
		}
		if (ED_IS_SHORTCUT("spatial_editor/rear_view", event_mod)) {
			_menu_option(VIEW_REAR);
		}
		if (ED_IS_SHORTCUT("spatial_editor/front_view", event_mod)) {
			_menu_option(VIEW_FRONT);
		}
		if (ED_IS_SHORTCUT("spatial_editor/left_view", event_mod)) {
			_menu_option(VIEW_LEFT);
		}
		if (ED_IS_SHORTCUT("spatial_editor/right_view", event_mod)) {
			_menu_option(VIEW_RIGHT);
		}
		if (ED_IS_SHORTCUT("spatial_editor/orbit_view_down", event_mod)) {
			// Clamp rotation to roughly -90..90 degrees so the user can't look upside-down and end up disoriented.
			view_3d_controller->cursor.x_rot = CLAMP(view_3d_controller->cursor.x_rot - Math::PI / 12.0, -1.57, 1.57);
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
		}
		if (ED_IS_SHORTCUT("spatial_editor/orbit_view_up", event_mod)) {
			// Clamp rotation to roughly -90..90 degrees so the user can't look upside-down and end up disoriented.
			view_3d_controller->cursor.x_rot = CLAMP(view_3d_controller->cursor.x_rot + Math::PI / 12.0, -1.57, 1.57);
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
		}
		if (ED_IS_SHORTCUT("spatial_editor/orbit_view_right", event_mod)) {
			view_3d_controller->cursor.y_rot -= Math::PI / 12.0;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
		}
		if (ED_IS_SHORTCUT("spatial_editor/orbit_view_left", event_mod)) {
			view_3d_controller->cursor.y_rot += Math::PI / 12.0;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
		}
		if (ED_IS_SHORTCUT("spatial_editor/orbit_view_180", event_mod)) {
			view_3d_controller->cursor.y_rot += Math::PI;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
		}
		if (ED_IS_SHORTCUT("spatial_editor/focus_origin", event_mod)) {
			_menu_option(VIEW_CENTER_TO_ORIGIN);
		}
		if (ED_IS_SHORTCUT("spatial_editor/focus_selection", event_mod)) {
			_menu_option(VIEW_CENTER_TO_SELECTION);
			times_focused_consecutively += 1;
		}
		if (ED_IS_SHORTCUT("spatial_editor/align_transform_with_view", event_mod)) {
			_menu_option(VIEW_ALIGN_TRANSFORM_WITH_VIEW);
		}
		if (ED_IS_SHORTCUT("spatial_editor/align_rotation_with_view", event_mod)) {
			_menu_option(VIEW_ALIGN_ROTATION_WITH_VIEW);
		}
		if (ED_IS_SHORTCUT("spatial_editor/insert_anim_key", event_mod)) {
			if (!get_selected_count() || _edit.mode != TRANSFORM_NONE) {
				return;
			}

			if (!AnimationPlayerEditor::get_singleton()->get_track_editor()->has_keying()) {
				set_message(TTR("Keying is disabled (no key inserted)."));
				return;
			}

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			for (Node *E : selection) {
				Node3D *sp = Object::cast_to<Node3D>(E);
				if (!sp) {
					continue;
				}

				spatial_editor->emit_signal(SNAME("transform_key_request"), sp, "", sp->get_transform());
			}

			set_message(TTR("Animation Key Inserted."));
		}
		if (ED_IS_SHORTCUT("spatial_editor/cancel_transform", event_mod) && _edit.mode != TRANSFORM_NONE) {
			cancel_transform();
		}
		if (!view_3d_controller->is_freelook_enabled() && !k->is_echo()) {
			if (ED_IS_SHORTCUT("spatial_editor/reset_transform_position", event_mod)) {
				_reset_transform(TransformType::POSITION);
			}
			if (ED_IS_SHORTCUT("spatial_editor/reset_transform_rotation", event_mod)) {
				_reset_transform(TransformType::ROTATION);
			}
			if (ED_IS_SHORTCUT("spatial_editor/reset_transform_scale", event_mod)) {
				_reset_transform(TransformType::SCALE);
			}
			if (ED_IS_SHORTCUT("spatial_editor/instant_translate", event_mod) && (_edit.mode != TRANSFORM_TRANSLATE || collision_reposition)) {
				if (_edit.mode == TRANSFORM_NONE) {
					begin_transform(TRANSFORM_TRANSLATE, true);
				} else if (_edit.instant || collision_reposition) {
					commit_transform();
					begin_transform(TRANSFORM_TRANSLATE, true);
				}
			}
			if (ED_IS_SHORTCUT("spatial_editor/instant_rotate", event_mod)) {
				if (_edit.mode == TRANSFORM_ROTATE && _edit.instant) {
					_edit.is_trackball = !_edit.is_trackball;
					_edit.show_rotation_line = !_edit.is_trackball;
					_edit.plane = TRANSFORM_VIEW;
					_edit.original_mouse_pos = _edit.mouse_pos;
					if (_edit.is_trackball) {
						set_message(TTR("Trackball Rotation"));
					} else {
						_edit.initial_click_vector = Vector3();
						_edit.previous_rotation_vector = Vector3();
						_edit.accumulated_rotation_angle = 0.0;
						_edit.rotation_angle = 0.0;
						set_message(vformat(TTR("Rotating %s degrees."), String::num(0, 0)));
					}
					surface->queue_redraw();
				} else if (_edit.mode != TRANSFORM_ROTATE) {
					if (_edit.mode == TRANSFORM_NONE) {
						begin_transform(TRANSFORM_ROTATE, true);
					} else if (_edit.instant || collision_reposition) {
						commit_transform();
						begin_transform(TRANSFORM_ROTATE, true);
					}
				}
			}
			if (ED_IS_SHORTCUT("spatial_editor/instant_scale", event_mod) && _edit.mode != TRANSFORM_SCALE) {
				if (_edit.mode == TRANSFORM_NONE) {
					begin_transform(TRANSFORM_SCALE, true);
				} else if (_edit.instant || collision_reposition) {
					commit_transform();
					begin_transform(TRANSFORM_SCALE, true);
				}
			}
			if (ED_IS_SHORTCUT("spatial_editor/collision_reposition", event_mod) && editor_selection->get_top_selected_node_list().size() == 1 && !collision_reposition) {
				if (_edit.mode == TRANSFORM_NONE || _edit.instant) {
					if (_edit.mode == TRANSFORM_NONE) {
						_compute_edit(_edit.mouse_pos);
					} else {
						commit_transform();
						_compute_edit(_edit.mouse_pos);
					}
					_edit.mode = TRANSFORM_TRANSLATE;
					collision_reposition = true;
				}
			}
		}

		// Freelook doesn't work in orthogonal mode or when previewing without pilot mode.
		if (!view_3d_controller->is_orthogonal() && !(previewing && !pilot_preview_enabled) && ED_IS_SHORTCUT("spatial_editor/freelook_toggle", event_mod)) {
			view_3d_controller->set_freelook_enabled(!view_3d_controller->is_freelook_enabled());
		} else if (k->get_keycode() == Key::ESCAPE) {
			view_3d_controller->set_freelook_enabled(false);
		}

		if (k->get_keycode() == Key::SPACE) {
			if (!k->is_pressed()) {
				emit_signal(SNAME("toggle_maximize_view"), this);
			}
		}
	}

	// Freelook uses most of the useful shortcuts, like save, so its OK
	// to consider freelook active as end of the line for future events.
	if (view_3d_controller->is_freelook_enabled()) {
		accept_event();
	}
}

void Node3DEditorViewport::_cursor_interpolated() {
	last_camera_transform = view_3d_controller->interp_to_camera_transform();

	if (previewing_camera && previewing && pilot_preview_enabled) {
		_pilot_ensure_undo_session();
		previewing->set_global_transform(last_camera_transform);
		pilot_undo_idle_time = 0.0;
	} else if (!previewing_camera) {
		camera->set_global_transform(last_camera_transform);
	}

	if (view_3d_controller->is_orthogonal()) {
		float half_fov = Math::deg_to_rad(get_fov()) / 2.0;
		float height = 2.0 * view_3d_controller->cursor.distance * Math::tan(half_fov);
		camera->set_orthogonal(height, get_znear(), get_zfar());
	} else {
		camera->set_perspective(get_fov(), get_znear(), get_zfar());
	}

	view_3d_controller->set_z_near(get_znear());
	view_3d_controller->set_z_far(get_zfar());

	update_transform_gizmo_view();
	rotation_control->queue_redraw();
	position_control->queue_redraw();
	look_control->queue_redraw();
	surface->queue_redraw();
	spatial_editor->update_grid();
}

void Node3DEditorViewport::_cursor_distance_scaled() {
	zoom_indicator_delay = ZOOM_FREELOOK_INDICATOR_DELAY_S;
	surface->queue_redraw();
}

void Node3DEditorViewport::_freelook_changed() {
	spatial_editor->set_freelook_viewport(view_3d_controller->is_freelook_enabled() ? this : nullptr);
}

void Node3DEditorViewport::_pilot_ensure_undo_session() {
	if (pilot_undo_session_active || !previewing) {
		return;
	}
	pilot_undo_initial_transform = previewing->get_global_transform();
	pilot_undo_session_active = true;
	pilot_undo_idle_time = 0.0;
}

void Node3DEditorViewport::_pilot_commit_undo_session() {
	if (!pilot_undo_session_active) {
		return;
	}
	pilot_undo_session_active = false;
	pilot_undo_idle_time = 0.0;
	if (!previewing) {
		return;
	}
	const Transform3D current_transform = previewing->get_global_transform();
	if (current_transform.is_equal_approx(pilot_undo_initial_transform)) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(vformat(TTR("Move Camera \"%s\""), previewing->get_name()), UndoRedo::MERGE_ENDS);
	undo_redo->add_do_method(previewing, "set_global_transform", current_transform);
	undo_redo->add_undo_method(previewing, "set_global_transform", pilot_undo_initial_transform);
	undo_redo->commit_action(false);
}

void Node3DEditorViewport::_pilot_tick_undo_session(real_t p_delta) {
	if (!pilot_undo_session_active) {
		return;
	}
	pilot_undo_idle_time += p_delta;
	if (pilot_undo_idle_time > 0.15) {
		_pilot_commit_undo_session();
	}
}

void Node3DEditorViewport::_freelook_speed_scaled() {
	zoom_indicator_delay = ZOOM_FREELOOK_INDICATOR_DELAY_S;
	surface->queue_redraw();
}

bool Node3DEditorViewport::_is_nav_modifier_pressed(const String &p_name) {
	return _is_shortcut_empty(p_name) || Input::get_singleton()->is_action_pressed(p_name);
}

bool Node3DEditorViewport::_is_shortcut_empty(const String &p_name) {
	Ref<Shortcut> check_shortcut = ED_GET_SHORTCUT(p_name);

	ERR_FAIL_COND_V_MSG(check_shortcut.is_null(), true, "The Shortcut was null, possible name mismatch.");

	return check_shortcut->get_events().is_empty();
}

void Node3DEditorViewport::set_message(const String &p_message, float p_time) {
	message = p_message;
	message_time = p_time;
}

void Node3DEditorPlugin::edited_scene_changed() {
	// The document changed, so its world did too: follow it before anything
	// tries to draw into the world of the scene that was open before. Every
	// open view has to be told, not just the one in the first pane - a view
	// bound to a document of its own still follows it when tabs close and the
	// scene it was pointed at is gone.
	for (Node3DEditor *editor : Node3DEditor::get_instances()) {
		editor->update_editing_world();

		for (uint32_t i = 0; i < Node3DEditor::VIEWPORTS_COUNT; i++) {
			Node3DEditorViewport *viewport = editor->get_editor_viewport(i);
			if (viewport->is_visible()) {
				viewport->notification(Control::NOTIFICATION_VISIBILITY_CHANGED);
			}
		}
	}
}

void Node3DEditorViewport::_project_settings_changed() {
	// Update shadow atlas if changed.
	int shadowmap_size = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_size");
	bool shadowmap_16_bits = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_16_bits");
	int atlas_q0 = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_quadrant_0_subdiv");
	int atlas_q1 = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_quadrant_1_subdiv");
	int atlas_q2 = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_quadrant_2_subdiv");
	int atlas_q3 = GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/atlas_quadrant_3_subdiv");

	viewport->set_positional_shadow_atlas_size(shadowmap_size);
	viewport->set_positional_shadow_atlas_16_bits(shadowmap_16_bits);
	viewport->set_positional_shadow_atlas_quadrant_subdiv(0, Viewport::PositionalShadowAtlasQuadrantSubdiv(atlas_q0));
	viewport->set_positional_shadow_atlas_quadrant_subdiv(1, Viewport::PositionalShadowAtlasQuadrantSubdiv(atlas_q1));
	viewport->set_positional_shadow_atlas_quadrant_subdiv(2, Viewport::PositionalShadowAtlasQuadrantSubdiv(atlas_q2));
	viewport->set_positional_shadow_atlas_quadrant_subdiv(3, Viewport::PositionalShadowAtlasQuadrantSubdiv(atlas_q3));

	// Update MSAA, screen-space AA and debanding if changed

	const int msaa_mode = GLOBAL_GET("rendering/anti_aliasing/quality/msaa_3d");
	viewport->set_msaa_3d(Viewport::MSAA(msaa_mode));
	const int ssaa_mode = GLOBAL_GET("rendering/anti_aliasing/quality/screen_space_aa");
	viewport->set_screen_space_aa(Viewport::ScreenSpaceAA(ssaa_mode));
	const bool use_taa = GLOBAL_GET("rendering/anti_aliasing/quality/use_taa");
	viewport->set_use_taa(use_taa);

	const bool transparent_background = GLOBAL_GET("rendering/viewport/transparent_background");
	viewport->set_transparent_background(transparent_background);

	const bool use_debanding = GLOBAL_GET("rendering/anti_aliasing/quality/use_debanding");
	viewport->set_use_debanding(use_debanding);

	const bool use_occlusion_culling = GLOBAL_GET("rendering/occlusion_culling/use_occlusion_culling");
	viewport->set_use_occlusion_culling(use_occlusion_culling);

	const float mesh_lod_threshold = GLOBAL_GET("rendering/mesh_lod/lod_change/threshold_pixels");
	viewport->set_mesh_lod_threshold(mesh_lod_threshold);

	const Viewport::Scaling3DMode scaling_3d_mode = Viewport::Scaling3DMode(int(GLOBAL_GET("rendering/scaling_3d/mode")));
	viewport->set_scaling_3d_mode(scaling_3d_mode);

	_update_shrink();

	const float fsr_sharpness = GLOBAL_GET("rendering/scaling_3d/fsr_sharpness");
	viewport->set_fsr_sharpness(fsr_sharpness);

	const float texture_mipmap_bias = GLOBAL_GET("rendering/textures/default_filters/texture_mipmap_bias");
	viewport->set_texture_mipmap_bias(texture_mipmap_bias);

	const Viewport::AnisotropicFiltering anisotropic_filtering_level = Viewport::AnisotropicFiltering(int(GLOBAL_GET("rendering/textures/default_filters/anisotropic_filtering_level")));
	viewport->set_anisotropic_filtering_level(anisotropic_filtering_level);
}

static void override_label_colors(Control *p_control) {
	// Read from the editor's own base rather than from p_control. These colours
	// become overrides, and an override keeps whatever it was given: a control
	// that is between parents - which a pane being split makes it - cannot reach
	// the editor theme, so it would be handed a default and keep it. That is
	// what turned the white text on these buttons dark after a split.
	Control *theme_source = EditorNode::get_singleton()->get_gui_base();
	p_control->begin_bulk_theme_override();
	p_control->add_theme_color_override(SceneStringName(font_color), theme_source->get_theme_color(SNAME("font_dark_background_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_hover_color", theme_source->get_theme_color(SNAME("font_dark_background_hover_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_focus_color", theme_source->get_theme_color(SNAME("font_dark_background_focus_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_pressed_color", theme_source->get_theme_color(SNAME("font_dark_background_pressed_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_hover_pressed_color", theme_source->get_theme_color(SNAME("font_dark_background_hover_pressed_color"), EditorStringName(Editor)));
	p_control->end_bulk_theme_override();
}

static void override_button_stylebox(Button *p_button, const Ref<StyleBox> p_stylebox) {
	p_button->begin_bulk_theme_override();
	p_button->add_theme_style_override(CoreStringName(normal), p_stylebox);
	p_button->add_theme_style_override("normal_mirrored", p_stylebox);
	p_button->add_theme_style_override(SceneStringName(hover), p_stylebox);
	p_button->add_theme_style_override("hover_mirrored", p_stylebox);
	p_button->add_theme_style_override("hover_pressed", p_stylebox);
	p_button->add_theme_style_override("hover_pressed_mirrored", p_stylebox);
	p_button->add_theme_style_override(SceneStringName(pressed), p_stylebox);
	p_button->add_theme_style_override("pressed_mirrored", p_stylebox);
	p_button->add_theme_style_override("focus", p_stylebox);
	p_button->add_theme_style_override("focus_mirrored", p_stylebox);
	p_button->add_theme_style_override("disabled", p_stylebox);
	p_button->add_theme_style_override("disabled_mirrored", p_stylebox);
	p_button->end_bulk_theme_override();
}

void Node3DEditorViewport::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_name();
			_update_centered_labels();
			message_time = MIN(message_time, 0.001); // Make it disappear.

			const int item_count = display_submenu->get_item_count();
			for (int i = 0; i < item_count; i++) {
				const Array item_data = display_submenu->get_item_metadata(i);
				if (item_data.is_empty()) {
					continue;
				}

				SupportedRenderingMethods rendering_methods = item_data[0];
				String base_tooltip = item_data[1];

				bool disabled = false;
				String disabled_tooltip;
				switch (rendering_methods) {
					case SupportedRenderingMethods::ALL:
						break;
					case SupportedRenderingMethods::FORWARD_PLUS_MOBILE:
						disabled = OS::get_singleton()->get_current_rendering_method() == "gl_compatibility";
						disabled_tooltip = TTR("This debug draw mode is only supported when using the Forward+ or Mobile renderer.");
						break;
					case SupportedRenderingMethods::FORWARD_PLUS:
						disabled = OS::get_singleton()->get_current_rendering_method() == "gl_compatibility" || OS::get_singleton()->get_current_rendering_method() == "mobile";
						disabled_tooltip = TTR("This debug draw mode is only supported when using the Forward+ renderer.");
						break;
				}

				display_submenu->set_item_disabled(i, disabled);
				String tooltip = TTR(base_tooltip);
				if (disabled) {
					if (tooltip.is_empty()) {
						tooltip = disabled_tooltip;
					} else {
						tooltip += "\n\n" + disabled_tooltip;
					}
				}
				display_submenu->set_item_tooltip(i, tooltip);
			}
		} break;

		case NOTIFICATION_READY: {
			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &Node3DEditorViewport::_project_settings_changed));
			_update_navigation_controls_visibility();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			bool vp_visible = is_visible_in_tree();

			set_process(vp_visible);
			set_physics_process(vp_visible);

			if (vp_visible) {
				view_3d_controller->set_orthogonal(view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_ORTHOGONAL)));
				view_3d_controller->update_camera();
				_update_name();
			} else {
				view_3d_controller->set_freelook_enabled(false);
				_pilot_commit_undo_session();
			}
			callable_mp(this, &Node3DEditorViewport::update_transform_gizmo_view).call_deferred();
		} break;

		case NOTIFICATION_RESIZED: {
			callable_mp(this, &Node3DEditorViewport::update_transform_gizmo_view).call_deferred();
		} break;

		case NOTIFICATION_PROCESS: {
			if (ruler->is_inside_tree()) {
				Vector3 start_pos = ruler_start_point->get_global_position();
				Vector3 end_pos = ruler_end_point->get_global_position();

				geometry->clear_surfaces();
				geometry->surface_begin(Mesh::PRIMITIVE_LINES);

				Vector3 center = (start_pos + end_pos) / 2;
				Vector3 camera_dir = (camera->get_transform().origin - center).normalized();
				real_t offset_distance = 0.01;

				geometry->surface_add_vertex(start_pos + camera_dir * offset_distance);
				geometry->surface_add_vertex(end_pos + camera_dir * offset_distance);
				geometry->surface_end();

				geometry_xray->clear_surfaces();
				geometry_xray->surface_begin(Mesh::PRIMITIVE_LINES);
				geometry_xray->surface_add_vertex(start_pos);
				geometry_xray->surface_add_vertex(end_pos);
				geometry_xray->surface_end();

				float distance = start_pos.distance_to(end_pos);
				if (distance < 0.001) {
					distance = 0.0;
				}
				ruler_label->set_text(TranslationServer::get_singleton()->format_number(vformat("%.3f m", distance), _get_locale()));

				Vector2 screen_position = camera->unproject_position(center) - (ruler_label->get_custom_minimum_size() / 2);
				ruler_label->set_position(screen_position);

				bool show_components = Input::get_singleton()->is_key_pressed(Key::SHIFT);

				if (show_components) {
					Ref<ImmediateMesh> triangle_mesh = ruler_triangle_lines->get_mesh();
					Ref<ImmediateMesh> triangle_mesh_xray = ruler_triangle_lines_xray->get_mesh();

					if (triangle_mesh.is_valid() && triangle_mesh_xray.is_valid()) {
						Vector3 delta = end_pos - start_pos;
						delta = delta.abs();
						const real_t threshold = 0.001;
						if (delta.x < threshold) {
							delta.x = 0.0;
						}
						if (delta.y < threshold) {
							delta.y = 0.0;
						}
						if (delta.z < threshold) {
							delta.z = 0.0;
						}

						Vector3 corner_point;
						Color first_line_color;
						Color second_line_color;

						Color axis_x_color = get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor));
						Color axis_y_color = get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor));
						Color axis_z_color = get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor));

						if (delta.x > 0.0 && delta.y > 0.0 && delta.z == 0.0) {
							// XY plane
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);
							first_line_color = axis_x_color;
							second_line_color = axis_y_color;
						} else if (delta.x > 0.0 && delta.z > 0.0 && delta.y == 0.0) {
							// XZ plane
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);
							first_line_color = axis_x_color;
							second_line_color = axis_z_color;
						} else if (delta.y > 0.0 && delta.z > 0.0 && delta.x == 0.0) {
							// YZ plane
							corner_point = Vector3(start_pos.x, start_pos.y, end_pos.z);
							first_line_color = axis_z_color;
							second_line_color = axis_y_color;
						} else if (delta.x > 0.0 && delta.y > 0.0 && delta.z > 0.0) {
							// All three axes
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);
							first_line_color = axis_x_color;
							second_line_color = axis_y_color;
						} else {
							corner_point = end_pos;
							first_line_color = axis_x_color;
							second_line_color = axis_x_color;
						}

						triangle_mesh->clear_surfaces();
						triangle_mesh->surface_begin(Mesh::PRIMITIVE_LINES);

						Vector3 triangle_camera_dir = (camera->get_transform().origin - center).normalized();
						real_t triangle_offset_distance = 0.01;

						triangle_mesh->surface_set_color(first_line_color);
						triangle_mesh->surface_add_vertex(start_pos + triangle_camera_dir * triangle_offset_distance);
						triangle_mesh->surface_set_color(first_line_color);
						triangle_mesh->surface_add_vertex(corner_point + triangle_camera_dir * triangle_offset_distance);
						triangle_mesh->surface_set_color(second_line_color);
						triangle_mesh->surface_add_vertex(corner_point + triangle_camera_dir * triangle_offset_distance);
						triangle_mesh->surface_set_color(second_line_color);
						triangle_mesh->surface_add_vertex(end_pos + triangle_camera_dir * triangle_offset_distance);

						triangle_mesh->surface_end();

						Color first_line_color_xray = first_line_color;
						Color second_line_color_xray = second_line_color;
						first_line_color_xray.a = 0.15;
						second_line_color_xray.a = 0.15;

						triangle_mesh_xray->clear_surfaces();
						triangle_mesh_xray->surface_begin(Mesh::PRIMITIVE_LINES);

						triangle_mesh_xray->surface_set_color(first_line_color_xray);
						triangle_mesh_xray->surface_add_vertex(start_pos);
						triangle_mesh_xray->surface_set_color(first_line_color_xray);
						triangle_mesh_xray->surface_add_vertex(corner_point);
						triangle_mesh_xray->surface_set_color(second_line_color_xray);
						triangle_mesh_xray->surface_add_vertex(corner_point);
						triangle_mesh_xray->surface_set_color(second_line_color_xray);
						triangle_mesh_xray->surface_add_vertex(end_pos);

						triangle_mesh_xray->surface_end();
					}
				} else {
					Ref<ImmediateMesh> triangle_mesh = ruler_triangle_lines->get_mesh();
					Ref<ImmediateMesh> triangle_mesh_xray = ruler_triangle_lines_xray->get_mesh();

					if (triangle_mesh.is_valid()) {
						triangle_mesh->clear_surfaces();
					}
					if (triangle_mesh_xray.is_valid()) {
						triangle_mesh_xray->clear_surfaces();
					}
				}

				if (show_components) {
					Vector3 delta = end_pos - start_pos;
					delta = delta.abs();
					const real_t threshold = 0.001;
					if (delta.x < threshold) {
						delta.x = 0.0;
					}
					if (delta.y < threshold) {
						delta.y = 0.0;
					}
					if (delta.z < threshold) {
						delta.z = 0.0;
					}

					int active_axes = 0;
					if (delta.x > 0.0) {
						active_axes++;
					}
					if (delta.y > 0.0) {
						active_axes++;
					}
					if (delta.z > 0.0) {
						active_axes++;
					}

					String x_text = delta.x > 0.0 ? TranslationServer::get_singleton()->format_number(vformat("X: %.3f m", delta.x), _get_locale()) : "";
					String y_text = delta.y > 0.0 ? TranslationServer::get_singleton()->format_number(vformat("Y: %.3f m", delta.y), _get_locale()) : "";
					String z_text = delta.z > 0.0 ? TranslationServer::get_singleton()->format_number(vformat("Z: %.3f m", delta.z), _get_locale()) : "";

					Color axis_x_color = get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor));
					Color axis_y_color = get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor));
					Color axis_z_color = get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor));

					Vector3 corner_point;

					if (active_axes >= 2) {
						if (delta.z == 0.0) {
							// XY plane
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);

							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_x_color);
							ruler_label_x->set_text(x_text);
							ruler_label_x->set_visible(true);
							Vector2 x_pos = camera->unproject_position((start_pos + corner_point) / 2) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(x_pos);

							ruler_label_y->add_theme_color_override(SceneStringName(font_color), axis_y_color);
							ruler_label_y->set_text(y_text);
							ruler_label_y->set_visible(true);
							Vector2 y_pos = camera->unproject_position((corner_point + end_pos) / 2) - (ruler_label_y->get_custom_minimum_size() / 2);
							ruler_label_y->set_position(y_pos);

							ruler_label_z->set_visible(false);
						} else if (delta.y == 0.0) {
							// XZ plane
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);

							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_x_color);
							ruler_label_x->set_text(x_text);
							ruler_label_x->set_visible(true);
							Vector2 x_pos = camera->unproject_position((start_pos + corner_point) / 2) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(x_pos);

							ruler_label_y->add_theme_color_override(SceneStringName(font_color), axis_z_color);
							ruler_label_y->set_text(z_text);
							ruler_label_y->set_visible(true);
							Vector2 z_pos = camera->unproject_position((corner_point + end_pos) / 2) - (ruler_label_y->get_custom_minimum_size() / 2);
							ruler_label_y->set_position(z_pos);

							ruler_label_z->set_visible(false);
						} else if (delta.x == 0.0) {
							// YZ plane
							corner_point = Vector3(start_pos.x, start_pos.y, end_pos.z);

							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_z_color);
							ruler_label_x->set_text(z_text);
							ruler_label_x->set_visible(true);
							Vector2 z_pos = camera->unproject_position((start_pos + corner_point) / 2) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(z_pos);

							ruler_label_y->add_theme_color_override(SceneStringName(font_color), axis_y_color);
							ruler_label_y->set_text(y_text);
							ruler_label_y->set_visible(true);
							Vector2 y_pos = camera->unproject_position((corner_point + end_pos) / 2) - (ruler_label_y->get_custom_minimum_size() / 2);
							ruler_label_y->set_position(y_pos);

							ruler_label_z->set_visible(false);
						} else {
							// All three axes
							corner_point = Vector3(end_pos.x, start_pos.y, start_pos.z);

							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_x_color);
							ruler_label_x->set_text(x_text);
							ruler_label_x->set_visible(true);
							Vector2 x_pos = camera->unproject_position((start_pos + corner_point) / 2) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(x_pos);

							ruler_label_y->add_theme_color_override(SceneStringName(font_color), axis_y_color);
							ruler_label_y->set_text(y_text);
							ruler_label_y->set_visible(true);
							Vector2 y_pos = camera->unproject_position((corner_point + end_pos) / 2) - (ruler_label_y->get_custom_minimum_size() / 2);
							ruler_label_y->set_position(y_pos);

							ruler_label_z->add_theme_color_override(SceneStringName(font_color), axis_z_color);
							ruler_label_z->set_text(z_text);
							ruler_label_z->set_visible(true);
							Vector2 z_pos = camera->unproject_position(center + Vector3(0, 0, -0.5)) - (ruler_label_z->get_custom_minimum_size() / 2);
							ruler_label_z->set_position(z_pos);
						}
					} else if (active_axes == 1) {
						corner_point = end_pos;

						if (delta.x > 0.0) {
							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_x_color);
							ruler_label_x->set_text(x_text);
							ruler_label_x->set_visible(true);
							Vector2 pos = camera->unproject_position(center) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(pos);
							ruler_label_y->set_visible(false);
							ruler_label_z->set_visible(false);
						} else if (delta.y > 0.0) {
							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_y_color);
							ruler_label_x->set_text(y_text);
							ruler_label_x->set_visible(true);
							Vector2 pos = camera->unproject_position(center) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(pos);
							ruler_label_y->set_visible(false);
							ruler_label_z->set_visible(false);
						} else {
							ruler_label_x->add_theme_color_override(SceneStringName(font_color), axis_z_color);
							ruler_label_x->set_text(z_text);
							ruler_label_x->set_visible(true);
							Vector2 pos = camera->unproject_position(center) - (ruler_label_x->get_custom_minimum_size() / 2);
							ruler_label_x->set_position(pos);
							ruler_label_y->set_visible(false);
							ruler_label_z->set_visible(false);
						}
					} else {
						ruler_label_x->set_visible(false);
						ruler_label_y->set_visible(false);
						ruler_label_z->set_visible(false);
					}

				} else {
					ruler_label_x->set_visible(false);
					ruler_label_y->set_visible(false);
					ruler_label_z->set_visible(false);
				}
			}

			real_t delta = get_process_delta_time();

			if (zoom_indicator_delay > 0) {
				zoom_indicator_delay -= delta;
				if (zoom_indicator_delay <= 0) {
					surface->queue_redraw();
					zoom_limit_label->hide();
				}
			}

			if (view_3d_controller->is_freelook_enabled()) {
				view_3d_controller->update_freelook(delta);
				_disable_follow_mode();
			}

			if (focused_node_id.is_valid() && get_selected_count() > 0 && times_focused_consecutively >= 2 && times_focused_consecutively % 2 == 0) {
				Node *focused_node = ObjectDB::get_instance<Node>(focused_node_id);
				if (focused_node) {
					follow_mode->set_text(vformat(TTR("Following %s"), focused_node->get_name()));
					follow_mode->set_button_icon(get_editor_theme_icon(focused_node->get_class()));
					follow_mode->show();
					focus_selection();
				} else {
					_disable_follow_mode();
				}
			} else {
				follow_mode->hide();
			}

			Node *scene_root = SceneTreeDock::get_singleton()->get_editor_data()->get_edited_scene_root();
			if (previewing_cinema && scene_root != nullptr) {
				Camera3D *cam = scene_root->get_viewport()->get_camera_3d();
				if (cam != nullptr && cam != previewing) {
					//then switch the viewport's camera to the scene's viewport camera
					if (previewing != nullptr) {
						_pilot_commit_undo_session();
						previewing->disconnect(SceneStringName(tree_exited), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
						previewing->disconnect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
					}
					previewing = cam;
					previewing->connect(SceneStringName(tree_exited), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
					previewing->connect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
					RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), cam->get_camera());
					surface->queue_redraw();
				}
			}

			if (_camera_moved_externally()) {
				// If camera moved after this plugin last set it, presumably a tool script has moved it, accept the new camera transform as the cursor position.
				pilot_undo_session_active = false;
				pilot_undo_idle_time = 0.0;
				_apply_camera_transform_to_cursor();
				view_3d_controller->update_camera();
			} else {
				view_3d_controller->update_camera(delta);
			}

			_pilot_tick_undo_session(delta);

			const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();

			bool changed = false;
			bool exist = false;

			for (const KeyValue<ObjectID, Object *> &E : selection) {
				Node3D *sp = ObjectDB::get_instance<Node3D>(E.key);
				if (!sp) {
					continue;
				}

				Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
				if (!se) {
					continue;
				}

				Transform3D t = sp->get_global_gizmo_transform();
				if (!t.is_finite()) {
					continue;
				}
				AABB new_aabb = _calculate_spatial_bounds(sp);

				exist = true;
				if (se->last_xform == t && se->aabb == new_aabb && !se->last_xform_dirty) {
					continue;
				}
				changed = true;
				se->last_xform_dirty = false;
				se->last_xform = t;

				se->aabb = new_aabb;

				Transform3D t_offset = t;

				// apply AABB scaling before item's global transform
				{
					const Vector3 offset(0.005, 0.005, 0.005);
					Basis aabb_s;
					aabb_s.scale(se->aabb.size + offset);
					t.translate_local(se->aabb.position - offset / 2);
					t.basis = t.basis * aabb_s;
				}
				{
					const Vector3 offset(0.01, 0.01, 0.01);
					Basis aabb_s;
					aabb_s.scale(se->aabb.size + offset);
					t_offset.translate_local(se->aabb.position - offset / 2);
					t_offset.basis = t_offset.basis * aabb_s;
				}

				RenderingServer::get_singleton()->instance_set_transform(se->sbox_instance, t);
				RenderingServer::get_singleton()->instance_set_transform(se->sbox_instance_offset, t_offset);
				RenderingServer::get_singleton()->instance_set_transform(se->sbox_instance_xray, t);
				RenderingServer::get_singleton()->instance_set_transform(se->sbox_instance_xray_offset, t_offset);
			}

			if (changed || (spatial_editor->is_gizmo_visible() && !exist)) {
				spatial_editor->update_transform_gizmo();
			}

			if (message_time > 0) {
				if (message != last_message) {
					surface->queue_redraw();
					last_message = message;
				}

				message_time -= get_process_delta_time();
				if (message_time < 0) {
					surface->queue_redraw();
				}
			}

			bool show_info = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_INFORMATION));
			if (show_info != info_panel->is_visible()) {
				info_panel->set_visible(show_info);
			}

			Camera3D *current_camera;

			if (previewing) {
				current_camera = previewing;
			} else {
				current_camera = camera;
			}

			if (show_info) {
				const String viewport_size = vformat(U"%d × %d", viewport->get_size().x * viewport->get_scaling_3d_scale(), viewport->get_size().y * viewport->get_scaling_3d_scale());
				String text;
				text += vformat(TTR("X: %s"), rtos(current_camera->get_position().x).pad_decimals(1)) + "\n";
				text += vformat(TTR("Y: %s"), rtos(current_camera->get_position().y).pad_decimals(1)) + "\n";
				text += vformat(TTR("Z: %s"), rtos(current_camera->get_position().z).pad_decimals(1)) + "\n";
				text += "\n";
				text += vformat(
						TTR("Size: %s (%.1fMP)") + "\n",
						viewport_size,
						viewport->get_size().x * viewport->get_size().y * Math::pow(viewport->get_scaling_3d_scale(), 2) * 0.000001);

				text += "\n";
				text += vformat(TTR("Objects: %d"), viewport->get_render_info(Viewport::RENDER_INFO_TYPE_VISIBLE, Viewport::RENDER_INFO_OBJECTS_IN_FRAME)) + "\n";
				text += vformat(TTR("Primitives: %d"), viewport->get_render_info(Viewport::RENDER_INFO_TYPE_VISIBLE, Viewport::RENDER_INFO_PRIMITIVES_IN_FRAME)) + "\n";
				text += vformat(TTR("Draw Calls: %d"), viewport->get_render_info(Viewport::RENDER_INFO_TYPE_VISIBLE, Viewport::RENDER_INFO_DRAW_CALLS_IN_FRAME));

				info_label->set_text(text);
			}

			// FPS Counter.
			bool show_fps = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_FRAME_TIME));

			if (show_fps != frame_time_panel->is_visible()) {
				frame_time_panel->set_visible(show_fps);
				RS::get_singleton()->viewport_set_measure_render_time(viewport->get_viewport_rid(), show_fps);
				for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
					// Initialize to 120 FPS, so that the initial estimation until we get enough data is always reasonable.
					cpu_time_history[i] = 8.333333;
					gpu_time_history[i] = 8.333333;
				}
				cpu_time_history_index = 0;
				gpu_time_history_index = 0;
			}
			if (show_fps) {
				cpu_time_history[cpu_time_history_index] = RS::get_singleton()->viewport_get_measured_render_time_cpu(viewport->get_viewport_rid());
				cpu_time_history_index = (cpu_time_history_index + 1) % FRAME_TIME_HISTORY;
				double cpu_time = 0.0;
				for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
					cpu_time += cpu_time_history[i];
				}
				cpu_time /= FRAME_TIME_HISTORY;
				// Prevent unrealistically low values.
				cpu_time = MAX(0.01, cpu_time);

				gpu_time_history[gpu_time_history_index] = RS::get_singleton()->viewport_get_measured_render_time_gpu(viewport->get_viewport_rid());
				gpu_time_history_index = (gpu_time_history_index + 1) % FRAME_TIME_HISTORY;
				double gpu_time = 0.0;
				for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
					gpu_time += gpu_time_history[i];
				}
				gpu_time /= FRAME_TIME_HISTORY;
				// Prevent division by zero for the FPS counter (and unrealistically low values).
				// This limits the reported FPS to 100000.
				gpu_time = MAX(0.01, gpu_time);

				// Color labels depending on performance level ("good" = green, "OK" = yellow, "bad" = red).
				// Middle point is at 15 ms.
				cpu_time_label->set_text(vformat(TTR("CPU Time: %s ms"), rtos(cpu_time).pad_decimals(2)));
				cpu_time_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(cpu_time, 0, 30, 0, 1)));

				gpu_time_label->set_text(vformat(TTR("GPU Time: %s ms"), rtos(gpu_time).pad_decimals(2)));
				// Middle point is at 15 ms.
				gpu_time_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(gpu_time, 0, 30, 0, 1)));

				const double fps = 1000.0 / gpu_time;
				fps_label->set_text(vformat(TTR("FPS: %d"), fps));
				// Middle point is at 60 FPS.
				fps_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(fps, 110, 10, 0, 1)));
			}
		} break;

		case NOTIFICATION_PHYSICS_PROCESS: {
			if (collision_reposition) {
				Node3D *selected_node = nullptr;

				if (ruler->is_inside_tree()) {
					if (ruler_start_point->is_visible()) {
						selected_node = ruler_end_point;
					} else {
						selected_node = ruler_start_point;
					}
				} else {
					const List<Node *> &selection = editor_selection->get_top_selected_node_list();
					if (selection.size() == 1) {
						selected_node = Object::cast_to<Node3D>(selection.front()->get());
					}
				}

				if (selected_node) {
					if (!ruler->is_inside_tree()) {
						double snap = EDITOR_GET("interface/inspector/default_float_step");
						int snap_step_decimals = Math::range_step_decimals(snap);
						set_message(vformat(TTR("Translating: %s"), vformat("%.*v", snap_step_decimals, selected_node->get_global_position())));
					}

					selected_node->set_global_position(spatial_editor->snap_point(_get_instance_position(_edit.mouse_pos, selected_node)));

					if (ruler->is_inside_tree() && !ruler_start_point->is_visible()) {
						ruler_end_point->set_global_position(ruler_start_point->get_global_position());
						ruler_start_point->set_visible(true);
						ruler_end_point->set_visible(true);
						ruler_label->set_visible(true);
						ruler_label_x->set_visible(false);
						ruler_label_y->set_visible(false);
						ruler_label_z->set_visible(false);
					}
				}
			}

			if (!update_preview_node) {
				return;
			}
			if (preview_node->is_inside_tree()) {
				preview_node_pos = spatial_editor->snap_point(_get_instance_position(preview_node_viewport_pos, preview_node));
				double snap = EDITOR_GET("interface/inspector/default_float_step");
				int snap_step_decimals = Math::range_step_decimals(snap);
				set_message(vformat(TTR("Instantiating: %s"), vformat("%.*v", snap_step_decimals, preview_node_pos)));
				Transform3D preview_gl_transform = Transform3D(Basis(), preview_node_pos);
				preview_node->set_global_transform(preview_gl_transform);
				if (!preview_node->is_visible()) {
					preview_node->show();
				}
			}
			update_preview_node = false;
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_OUT:
		case NOTIFICATION_WM_WINDOW_FOCUS_OUT: {
			view_3d_controller->set_freelook_enabled(false);
			view_3d_controller->cursor.region_select = false;
			surface->queue_redraw();

			// Commit the drag if the window is focused out.
			if (_edit.mode != TRANSFORM_NONE) {
				commit_transform();
				return;
			}

			if (_edit.gizmo.is_valid()) {
				// Certain gizmo plugins should be able to commit handles without dragging them.
				if (_edit.original_mouse_pos != _edit.mouse_pos || _edit.gizmo->get_plugin()->can_commit_handle_on_click()) {
					_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_handle_secondary, _edit.gizmo_initial_value, false);
				}

				spatial_editor->get_single_selected_node()->update_gizmos();
				_edit.gizmo = Ref<EditorNode3DGizmo>();
				set_message("");
			}
		} break;

		case NOTIFICATION_ENTER_TREE: {
			// Wiring this view's own children, which never change. Entering the
			// tree used to happen exactly once, because a view was parented
			// where it was built and never moved; a pane that is split moves it,
			// so this has to say out loud that it is done once.
			if (!wired) {
				wired = true;
				surface->connect(SceneStringName(draw), callable_mp(this, &Node3DEditorViewport::_draw));
				surface->connect(SceneStringName(gui_input), callable_mp(this, &Node3DEditorViewport::_sinput));
				surface->connect(SceneStringName(mouse_entered), callable_mp(this, &Node3DEditorViewport::_surface_mouse_enter));
				surface->connect(SceneStringName(mouse_exited), callable_mp(this, &Node3DEditorViewport::_surface_mouse_exit));
				surface->connect(SceneStringName(focus_entered), callable_mp(this, &Node3DEditorViewport::_surface_focus_enter));
				surface->connect(SceneStringName(focus_exited), callable_mp(this, &Node3DEditorViewport::_surface_focus_exit));
			}

			// Render the world the edited scene lives in. Without this the camera
			// looks into the editor window's world, which the scene is not in.
			viewport->set_world_3d(get_editing_world());

			_init_gizmo_instance(index);
			_apply_gizmo_layer();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_finish_gizmo_instances();
			_release_gizmo_layer();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_centered_labels();

			view_display_menu->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHlDarkBackground")));
			preview_camera->set_button_icon(get_editor_theme_icon(SNAME("Camera3DDarkBackground")));
			pilot_camera->set_button_icon(get_editor_theme_icon(SNAME("3D")));
			Control *gui_base = EditorNode::get_singleton()->get_gui_base();

			const Ref<StyleBox> &information_3d_stylebox = gui_base->get_theme_stylebox(SNAME("Information3dViewport"), EditorStringName(EditorStyles));

			override_button_stylebox(view_display_menu, information_3d_stylebox);
			override_label_colors(view_display_menu);
			override_button_stylebox(translation_preview_button, information_3d_stylebox);
			override_label_colors(translation_preview_button);
			override_button_stylebox(follow_mode, information_3d_stylebox);
			override_label_colors(follow_mode);
			override_button_stylebox(preview_camera, information_3d_stylebox);
			override_label_colors(preview_camera);

			frame_time_gradient->set_color(0, gui_base->get_theme_color(SNAME("success_color_dark_background"), EditorStringName(Editor)));
			frame_time_gradient->set_color(1, gui_base->get_theme_color(SNAME("warning_color_dark_background"), EditorStringName(Editor)));
			frame_time_gradient->set_color(2, gui_base->get_theme_color(SNAME("error_color_dark_background"), EditorStringName(Editor)));

			override_button_stylebox(pilot_camera, information_3d_stylebox);
			override_label_colors(pilot_camera);

			info_panel->add_theme_style_override(SceneStringName(panel), information_3d_stylebox);
			override_label_colors(info_label);
			tooltip_panel->add_theme_style_override(CoreStringName(normal), information_3d_stylebox);

			frame_time_panel->add_theme_style_override(SceneStringName(panel), information_3d_stylebox);
			// Set a minimum width to prevent the width from changing all the time
			// when numbers vary rapidly. This minimum width is set based on a
			// GPU time of 999.99 ms in the current editor language.
			const float min_width = get_theme_font(SNAME("main"), EditorStringName(EditorFonts))->get_string_size(vformat(TTR("GPU Time: %s ms"), 999.99)).x;
			frame_time_panel->set_custom_minimum_size(Size2(min_width, 0) * EDSCALE);
			frame_time_vbox->add_theme_constant_override("separation", Math::round(-1 * EDSCALE));

			cinema_label->add_theme_style_override(CoreStringName(normal), information_3d_stylebox);
			locked_label->add_theme_style_override(CoreStringName(normal), information_3d_stylebox);

			ruler_label->add_theme_color_override(SceneStringName(font_color), Color(1.0, 0.9, 0.0, 1.0));
			ruler_label->add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 1.0));
			ruler_label->add_theme_constant_override("outline_size", 4 * EDSCALE);
			ruler_label->add_theme_font_size_override(SceneStringName(font_size), 15 * EDSCALE);
			ruler_label->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));

			ruler_label_x->add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 1.0));
			ruler_label_x->add_theme_constant_override("outline_size", 4 * EDSCALE);
			ruler_label_x->add_theme_font_size_override(SceneStringName(font_size), 15 * EDSCALE);
			ruler_label_x->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));

			ruler_label_y->add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 1.0));
			ruler_label_y->add_theme_constant_override("outline_size", 4 * EDSCALE);
			ruler_label_y->add_theme_font_size_override(SceneStringName(font_size), 15 * EDSCALE);
			ruler_label_y->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));

			ruler_label_z->add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 1.0));
			ruler_label_z->add_theme_constant_override("outline_size", 4 * EDSCALE);
			ruler_label_z->add_theme_font_size_override(SceneStringName(font_size), 15 * EDSCALE);
			ruler_label_z->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
		} break;

		case NOTIFICATION_DRAG_END: {
			// Clear preview material when dropped outside applicable object.
			if (spatial_editor->get_preview_material().is_valid() && !is_drag_successful()) {
				_reset_preview_material();
				_remove_preview_material();
			} else {
				_remove_preview_node();
			}
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			_update_view_3d_controller();

			if (EditorSettings::get_singleton()->check_changed_settings_in_group("editors/3d")) {
				_update_navigation_controls_visibility();
			}
		} break;
	}
}

void Node3DEditorViewport::_update_view_3d_controller(bool p_update_all) {
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FOV_DECREASE, ED_GET_SHORTCUT("spatial_editor/decrease_fov"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FOV_INCREASE, ED_GET_SHORTCUT("spatial_editor/increase_fov"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FOV_RESET, ED_GET_SHORTCUT("spatial_editor/reset_fov"));

	view_3d_controller->set_shortcut(View3DController::SHORTCUT_PAN_MOD_1, ED_GET_SHORTCUT("spatial_editor/viewport_pan_modifier_1"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_PAN_MOD_2, ED_GET_SHORTCUT("spatial_editor/viewport_pan_modifier_2"));

	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ORBIT_MOD_1, ED_GET_SHORTCUT("spatial_editor/viewport_orbit_modifier_1"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ORBIT_MOD_2, ED_GET_SHORTCUT("spatial_editor/viewport_orbit_modifier_2"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ORBIT_SNAP_MOD_1, ED_GET_SHORTCUT("spatial_editor/viewport_orbit_snap_modifier_1"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ORBIT_SNAP_MOD_2, ED_GET_SHORTCUT("spatial_editor/viewport_orbit_snap_modifier_2"));

	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_FORWARD, ED_GET_SHORTCUT("spatial_editor/freelook_forward"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_BACKWARDS, ED_GET_SHORTCUT("spatial_editor/freelook_backwards"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_LEFT, ED_GET_SHORTCUT("spatial_editor/freelook_left"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_RIGHT, ED_GET_SHORTCUT("spatial_editor/freelook_right"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_UP, ED_GET_SHORTCUT("spatial_editor/freelook_up"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_DOWN, ED_GET_SHORTCUT("spatial_editor/freelook_down"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_SPEED_MOD, ED_GET_SHORTCUT("spatial_editor/freelook_speed_modifier"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_FREELOOK_SLOW_MOD, ED_GET_SHORTCUT("spatial_editor/freelook_slow_modifier"));

	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ZOOM_MOD_1, ED_GET_SHORTCUT("spatial_editor/viewport_zoom_modifier_1"));
	view_3d_controller->set_shortcut(View3DController::SHORTCUT_ZOOM_MOD_2, ED_GET_SHORTCUT("spatial_editor/viewport_zoom_modifier_2"));

	if (p_update_all || EditorSettings::get_singleton()->check_changed_settings_in_group("editors/3d")) {
		view_3d_controller->set_pan_mouse_button((View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/pan_mouse_button").operator int());

		view_3d_controller->set_orbit_sensitivity(EDITOR_GET("editors/3d/navigation_feel/orbit_sensitivity"));
		view_3d_controller->set_orbit_inertia(EDITOR_GET("editors/3d/navigation_feel/orbit_inertia"));
		view_3d_controller->set_orbit_mouse_button((View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/orbit_mouse_button").operator int());

		view_3d_controller->set_zoom_style((View3DController::ZoomStyle)EDITOR_GET("editors/3d/navigation/zoom_style").operator int());
		view_3d_controller->set_zoom_inertia(EDITOR_GET("editors/3d/navigation_feel/zoom_inertia"));
		view_3d_controller->set_zoom_mouse_button((View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/zoom_mouse_button").operator int());

		view_3d_controller->set_freelook_scheme((View3DController::FreelookScheme)EDITOR_GET("editors/3d/freelook/freelook_navigation_scheme").operator int());
		view_3d_controller->set_freelook_base_speed(EDITOR_GET("editors/3d/freelook/freelook_base_speed"));
		view_3d_controller->set_freelook_sensitivity(EDITOR_GET("editors/3d/freelook/freelook_sensitivity"));
		view_3d_controller->set_freelook_inertia(EDITOR_GET("editors/3d/freelook/freelook_inertia"));
		view_3d_controller->set_freelook_speed_zoom_link(EDITOR_GET("editors/3d/freelook/freelook_speed_zoom_link"));
		view_3d_controller->set_freelook_invert_y_axis(EDITOR_GET("editors/3d/freelook/freelook_invert_y_axis"));

		view_3d_controller->set_translation_sensitivity(EDITOR_GET("editors/3d/navigation_feel/translation_sensitivity"));
		view_3d_controller->set_translation_inertia(EDITOR_GET("editors/3d/navigation_feel/translation_inertia"));

		view_3d_controller->set_angle_snap_threshold(EDITOR_GET("editors/3d/navigation_feel/angle_snap_threshold"));

		view_3d_controller->set_emulate_3_button_mouse(EDITOR_GET("editors/3d/navigation/emulate_3_button_mouse"));
		view_3d_controller->set_emulate_numpad(EDITOR_GET("editors/3d/navigation/emulate_numpad"));

		view_3d_controller->set_invert_x_axis(EDITOR_GET("editors/3d/navigation/invert_x_axis"));
		view_3d_controller->set_invert_y_axis(EDITOR_GET("editors/3d/navigation/invert_y_axis"));

		view_3d_controller->set_warped_mouse_panning((View3DController::NavigationMouseButton)EDITOR_GET("editors/3d/navigation/warped_mouse_panning").operator int());
	}
}

static void draw_indicator_bar(Control &p_surface, real_t p_fill, const Ref<Texture2D> p_icon, const Ref<Font> p_font, int p_font_size, const String &p_text, const Color &p_color) {
	// Adjust bar size from control height
	const Vector2 surface_size = p_surface.get_size();
	const real_t h = surface_size.y / 2.0;
	const real_t y = (surface_size.y - h) / 2.0;

	const Rect2 r(10 * EDSCALE, y, 6 * EDSCALE, h);
	const real_t sy = r.size.y * p_fill;

	// Note: because this bar appears over the viewport, it has to stay readable for any background color
	// Draw both neutral dark and bright colors to account this
	p_surface.draw_rect(r, p_color * Color(1, 1, 1, 0.2));
	p_surface.draw_rect(Rect2(r.position.x, r.position.y + r.size.y - sy, r.size.x, sy), p_color * Color(1, 1, 1, 0.6));
	p_surface.draw_rect(r.grow(1), Color(0, 0, 0, 0.7), false, Math::round(EDSCALE));

	const Vector2 icon_size = p_icon->get_size();
	const Vector2 icon_pos = Vector2(r.position.x - (icon_size.x - r.size.x) / 2, r.position.y + r.size.y + 2 * EDSCALE);
	p_surface.draw_texture(p_icon, icon_pos, p_color);

	// Draw text below the bar (for speed/zoom information).
	p_surface.draw_string_outline(p_font, Vector2(icon_pos.x, icon_pos.y + icon_size.y + 16 * EDSCALE), p_text, HORIZONTAL_ALIGNMENT_LEFT, -1.f, p_font_size, Math::round(4 * EDSCALE), Color(0, 0, 0));
	p_surface.draw_string(p_font, Vector2(icon_pos.x, icon_pos.y + icon_size.y + 16 * EDSCALE), p_text, HORIZONTAL_ALIGNMENT_LEFT, -1.f, p_font_size, p_color);
}

void Node3DEditorViewport::_draw() {
	EditorNode::get_singleton()->get_editor_plugins_over()->forward_3d_draw_over_viewport(surface);
	EditorNode::get_singleton()->get_editor_plugins_force_over()->forward_3d_force_draw_over_viewport(surface);

	if (surface->has_focus() || rotation_control->has_focus()) {
		Size2 size = surface->get_size();
		Rect2 r = Rect2(Point2(), size);
		get_theme_stylebox(SNAME("FocusViewport"), EditorStringName(EditorStyles))->draw(surface->get_canvas_item(), r);
	}

	View3DController::Cursor cursor = view_3d_controller->cursor;

	if (cursor.region_select && movement_threshold_passed) {
		const Rect2 selection_rect = Rect2(cursor.region_begin, cursor.region_end - cursor.region_begin);

		surface->draw_rect(
				selection_rect,
				get_theme_color(SNAME("box_selection_fill_color"), EditorStringName(Editor)));

		surface->draw_rect(
				selection_rect,
				get_theme_color(SNAME("box_selection_stroke_color"), EditorStringName(Editor)),
				false,
				Math::round(EDSCALE));
	}

	RID ci = surface->get_canvas_item();

	if (message_time > 0) {
		Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
		Point2 msgpos = Point2(10 * EDSCALE, get_size().y - 14 * EDSCALE);
		font->draw_string(ci, msgpos + Point2(1, 1), message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0, 0, 0, 0.8));
		font->draw_string(ci, msgpos + Point2(-1, -1), message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0, 0, 0, 0.8));
		font->draw_string(ci, msgpos, message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1, 1));
	}

	if ((vertex_snap_mode || vertex_snap_dragging) && vertex_snap_has_source) {
		const float circle_radius = 6.0f * EDSCALE;
		const float outline_width = 2.0f * EDSCALE;
		const float occluded_alpha = 0.30f;

		Vector3 source_display = vertex_snap_source;
		if (vertex_snap_dragging) {
			for (const KeyValue<ObjectID, Vector3> &E : vertex_snap_original_positions) {
				Node3D *node = ObjectDB::get_instance<Node3D>(E.key);
				if (node) {
					source_display = vertex_snap_source + (node->get_global_position() - E.value);
					break;
				}
			}
		}

		if (!camera->is_position_behind(source_display)) {
			Vector2 screen_pos = camera->unproject_position(source_display);
			bool occluded = _is_vertex_occluded(source_display, screen_pos);
			float alpha = occluded ? occluded_alpha : 1.0f;
			surface->draw_circle(screen_pos, circle_radius + outline_width, Color(0, 0, 0, 0.6 * alpha), true, -1.0, true);
			surface->draw_circle(screen_pos, circle_radius, Color(1, 1, 0, alpha), true, -1.0, true);
		}

		if (vertex_snap_dragging && vertex_snap_has_target && !camera->is_position_behind(vertex_snap_target)) {
			Vector2 screen_pos = camera->unproject_position(vertex_snap_target);
			bool occluded = _is_vertex_occluded(vertex_snap_target, screen_pos);
			float alpha = occluded ? occluded_alpha : 1.0f;
			surface->draw_circle(screen_pos, circle_radius + outline_width, Color(0, 0, 0, 0.6 * alpha), true, -1.0, true);
			surface->draw_circle(screen_pos, circle_radius, Color(0, 1, 0, alpha), true, -1.0, true);
		}
	}

	if (_edit.mode == TRANSFORM_ROTATE) {
		Point2 center = point_to_screen(_edit.center);

		Color handle_color;
		switch (_edit.plane) {
			case TRANSFORM_VIEW:
				handle_color = get_theme_color(SNAME("axis_view_plane_color"), EditorStringName(Editor));
				break;
			case TRANSFORM_X_AXIS:
				handle_color = get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor));
				break;
			case TRANSFORM_Y_AXIS:
				handle_color = get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor));
				break;
			case TRANSFORM_Z_AXIS:
				handle_color = get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor));
				break;
			default:
				handle_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
				break;
		}

		if (!_edit.is_trackball && _is_rotation_arc_visible() && !_edit.initial_click_vector.is_zero_approx()) {
			Vector3 up = _edit.rotation_axis;
			Vector3 right = _edit.initial_click_vector;

			right = right - up * up.dot(right);
			right.normalize();
			Vector3 forward = up.cross(right);

			real_t rotation_radius = (_edit.plane == TRANSFORM_VIEW) ? spatial_editor->gizmo_view_rotation_scale : GIZMO_CIRCLE_SIZE;

			const int circle_segments = 64;
			Vector<Point2> circle_points;
			for (int i = 0; i <= circle_segments; i++) {
				float angle = (float(i) / float(circle_segments)) * Math::TAU;
				Vector3 point_3d = _edit.center + gizmo_scale * rotation_radius * (right * Math::cos(angle) + forward * Math::sin(angle));
				Point2 point_2d = point_to_screen(point_3d);
				circle_points.push_back(point_2d);
			}

			Color circle_color = handle_color.from_hsv(handle_color.get_h(), handle_color.get_s() * 0.6, 1.0, 0.8);
			Vector<Color> circle_colors;
			circle_colors.resize(circle_points.size());
			circle_colors.fill(circle_color);
			RenderingServer::get_singleton()->canvas_item_add_polyline(ci, circle_points, circle_colors, Math::round(2 * EDSCALE), true);

			const int segments = 64;
			float display_angle = _edit.rotation_angle;

			float abs_angle = Math::abs(display_angle);
			if (abs_angle > Math::TAU) {
				float remainder = Math::fmod((double)abs_angle, Math::TAU);
				remainder = remainder < 0.01 ? Math::TAU : remainder;
				display_angle = SIGN(display_angle) * remainder;
				abs_angle = remainder;
			}

			int num_segments = MAX(8, int(abs_angle / (Math::TAU / segments) * segments));
			num_segments = MIN(num_segments, segments);

			Color fill_color = Color(1.0, 1.0, 1.0, 0.2);

			bool is_counterclockwise = display_angle > 0;
			float start_angle = is_counterclockwise ? 0.0f : display_angle;
			float end_angle = is_counterclockwise ? display_angle : 0.0f;

			for (int i = 0; i < num_segments; i++) {
				float t1 = float(i) / float(num_segments);
				float t2 = float(i + 1) / float(num_segments);
				float angle1 = Math::lerp(start_angle, end_angle, t1);
				float angle2 = Math::lerp(start_angle, end_angle, t2);

				Vector3 point1_3d = _edit.center + gizmo_scale * rotation_radius * (right * Math::cos(angle1) + forward * Math::sin(angle1));
				Vector3 point2_3d = _edit.center + gizmo_scale * rotation_radius * (right * Math::cos(angle2) + forward * Math::sin(angle2));

				Point2 point1_2d = point_to_screen(point1_3d);
				Point2 point2_2d = point_to_screen(point2_3d);

				Vector<Point2> triangle_points;
				triangle_points.push_back(center);
				triangle_points.push_back(point1_2d);
				triangle_points.push_back(point2_2d);

				Vector<Color> triangle_colors;
				triangle_colors.push_back(fill_color);
				triangle_colors.push_back(fill_color);
				triangle_colors.push_back(fill_color);

				RenderingServer::get_singleton()->canvas_item_add_polygon(ci, triangle_points, triangle_colors);
			}

			Color edge_color = handle_color.from_hsv(handle_color.get_h(), handle_color.get_s() * 0.8, 1.0, 0.7);

			Vector3 start_point_3d = _edit.center + gizmo_scale * rotation_radius * right;
			Point2 start_point_2d = point_to_screen(start_point_3d);
			RenderingServer::get_singleton()->canvas_item_add_line(
					ci,
					center,
					start_point_2d,
					edge_color,
					Math::round(2 * EDSCALE),
					true);

			Vector3 end_point_3d = _edit.center + gizmo_scale * rotation_radius * (right * Math::cos(display_angle) + forward * Math::sin(display_angle));
			Point2 end_point_2d = point_to_screen(end_point_3d);
			RenderingServer::get_singleton()->canvas_item_add_line(
					ci,
					center,
					end_point_2d,
					edge_color,
					Math::round(2 * EDSCALE),
					true);
		}

		if (_edit.show_rotation_line) {
			handle_color = handle_color.from_hsv(handle_color.get_h(), handle_color.get_s() * 0.25, 1.0, handle_color.a);
			RenderingServer::get_singleton()->canvas_item_add_line(
					ci,
					_edit.mouse_pos,
					center,
					handle_color,
					Math::round(2 * EDSCALE));
		}
	}
	if (previewing) {
		Size2 ss = Size2(GLOBAL_GET("display/window/size/viewport_width"), GLOBAL_GET("display/window/size/viewport_height"));
		float aspect = ss.aspect();
		Size2 s = get_size();

		Rect2 draw_rect;

		switch (previewing->get_keep_aspect_mode()) {
			case Camera3D::KEEP_WIDTH: {
				draw_rect.size = Size2(s.width, s.width / aspect);
				draw_rect.position.x = 0;
				draw_rect.position.y = (s.height - draw_rect.size.y) * 0.5;

			} break;
			case Camera3D::KEEP_HEIGHT: {
				draw_rect.size = Size2(s.height * aspect, s.height);
				draw_rect.position.y = 0;
				draw_rect.position.x = (s.width - draw_rect.size.x) * 0.5;

			} break;
		}

		draw_rect = Rect2(Vector2(), s).intersection(draw_rect);

		surface->draw_rect(draw_rect, Color(0.6, 0.6, 0.1, 0.5), false, Math::round(2 * EDSCALE));

	} else {
		if (zoom_indicator_delay > 0.0) {
			if (view_3d_controller->is_freelook_enabled()) {
				// Show speed.

				real_t min_speed = MAX(camera->get_near() * 4, View3DControllerConsts::ZOOM_FREELOOK_MIN);
				real_t max_speed = MIN(camera->get_far() / 4, View3DControllerConsts::ZOOM_FREELOOK_MAX);
				real_t scale_length = (max_speed - min_speed);

				if (!Math::is_zero_approx(scale_length)) {
					float freelook_speed = view_3d_controller->get_freelook_speed();
					real_t logscale_t = 1.0 - Math::log1p(freelook_speed - min_speed) / Math::log1p(scale_length);

					// Display the freelook speed to help the user get a better sense of scale.
					const int precision = freelook_speed < 1.0 ? 2 : 1;
					draw_indicator_bar(
							*surface,
							1.0 - logscale_t,
							get_editor_theme_icon(SNAME("ViewportSpeed")),
							get_theme_font("bold", EditorStringName(EditorFonts)),
							get_theme_font_size(SceneStringName(font_size), SNAME("Label")),
							vformat("%s m/s", String::num(freelook_speed).pad_decimals(precision)),
							Color(1.0, 0.95, 0.7));
				}
			} else {
				// Show zoom
				zoom_limit_label->set_visible(zoom_failed_attempts_count > 15);

				real_t min_distance = MAX(camera->get_near() * 4, View3DControllerConsts::ZOOM_FREELOOK_MIN);
				real_t max_distance = MIN(camera->get_far() / 4, View3DControllerConsts::ZOOM_FREELOOK_MAX);
				real_t scale_length = (max_distance - min_distance);

				if (!Math::is_zero_approx(scale_length)) {
					real_t logscale_t = 1.0 - Math::log1p(cursor.distance - min_distance) / Math::log1p(scale_length);

					// Display the zoom center distance to help the user get a better sense of scale.
					const int precision = cursor.distance < 1.0 ? 2 : 1;
					draw_indicator_bar(
							*surface,
							logscale_t,
							get_editor_theme_icon(SNAME("ViewportZoom")),
							get_theme_font("bold", EditorStringName(EditorFonts)),
							get_theme_font_size(SceneStringName(font_size), SNAME("Label")),
							vformat("%s m", String::num(cursor.distance).pad_decimals(precision)),
							Color(0.7, 0.95, 1.0));
				}
			}
		}
	}
}

bool Node3DEditorViewport::_camera_moved_externally() {
	if (previewing_camera && previewing) {
		if (pilot_preview_enabled) {
			Transform3D t = previewing->get_global_transform();
			return !t.is_equal_approx(last_camera_transform);
		}
		return false;
	}
	Transform3D t = camera->get_global_transform();
	return !t.is_equal_approx(last_camera_transform);
}

void Node3DEditorViewport::_apply_camera_transform_to_cursor() {
	if (previewing_camera && previewing) {
		if (pilot_preview_enabled) {
			_sync_cursor_from_transform(previewing->get_global_transform());
		}
		return;
	}
	_sync_cursor_from_transform(camera->get_camera_transform());
}

void Node3DEditorViewport::_menu_option(int p_option) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	switch (p_option) {
		case VIEW_TOP: {
			view_3d_controller->cursor.y_rot = 0;
			view_3d_controller->cursor.x_rot = Math::PI / 2.0;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			set_message(TTR("Top View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_TOP);

		} break;
		case VIEW_BOTTOM: {
			view_3d_controller->cursor.y_rot = 0;
			view_3d_controller->cursor.x_rot = -Math::PI / 2.0;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			set_message(TTR("Bottom View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_BOTTOM);

		} break;
		case VIEW_LEFT: {
			view_3d_controller->cursor.x_rot = 0;
			view_3d_controller->cursor.y_rot = Math::PI / 2.0;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			set_message(TTR("Left View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_LEFT);

		} break;
		case VIEW_RIGHT: {
			view_3d_controller->cursor.x_rot = 0;
			view_3d_controller->cursor.y_rot = -Math::PI / 2.0;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			set_message(TTR("Right View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_RIGHT);

		} break;
		case VIEW_FRONT: {
			view_3d_controller->cursor.x_rot = 0;
			view_3d_controller->cursor.y_rot = 0;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			set_message(TTR("Front View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_FRONT);

		} break;
		case VIEW_REAR: {
			view_3d_controller->cursor.x_rot = 0;
			view_3d_controller->cursor.y_rot = Math::PI;
			view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
			view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
			set_message(TTR("Rear View."), 2);
			view_3d_controller->set_view_type(View3DController::VIEW_TYPE_REAR);

		} break;
		case VIEW_CENTER_TO_ORIGIN: {
			view_3d_controller->cursor.pos = Vector3(0, 0, 0);
			_disable_follow_mode();

		} break;
		case VIEW_CENTER_TO_SELECTION: {
			focus_selection();

		} break;
		case VIEW_ALIGN_TRANSFORM_WITH_VIEW: {
			if (!get_selected_count()) {
				break;
			}

			Transform3D camera_transform = camera->get_global_transform();

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			undo_redo->create_action(TTR("Align Transform with View"));
			for (Node *E : selection) {
				Node3D *sp = Object::cast_to<Node3D>(E);
				if (!sp) {
					continue;
				}

				Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
				if (!se) {
					continue;
				}

				Transform3D xform = camera_transform;
				if (view_3d_controller->is_orthogonal()) {
					Vector3 offset = camera_transform.basis.xform(Vector3(0, 0, view_3d_controller->cursor.distance));
					xform.origin = view_3d_controller->cursor.pos + offset;
				} else {
					xform.scale_basis(sp->get_scale());
				}

				if (Object::cast_to<Decal>(E)) {
					// Adjust rotation to match Decal's default orientation.
					// This makes the decal "look" in the same direction as the camera,
					// rather than pointing down relative to the camera orientation.
					xform.basis.rotate_local(Vector3(1, 0, 0), Math::TAU * 0.25);
				}

				Node3D *parent = sp->get_parent_node_3d();
				Transform3D local_xform = parent ? parent->get_global_transform().affine_inverse() * xform : xform;
				undo_redo->add_do_method(sp, "set_transform", local_xform);
				undo_redo->add_undo_method(sp, "set_transform", sp->get_local_gizmo_transform());
			}
			undo_redo->commit_action();

		} break;
		case VIEW_ALIGN_ROTATION_WITH_VIEW: {
			if (!get_selected_count()) {
				break;
			}

			Transform3D camera_transform = camera->get_global_transform();

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			undo_redo->create_action(TTR("Align Rotation with View"));
			for (Node *E : selection) {
				Node3D *sp = Object::cast_to<Node3D>(E);
				if (!sp) {
					continue;
				}

				Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
				if (!se) {
					continue;
				}

				Basis basis = camera_transform.basis;

				if (Object::cast_to<Decal>(E)) {
					// Adjust rotation to match Decal's default orientation.
					// This makes the decal "look" in the same direction as the camera,
					// rather than pointing down relative to the camera orientation.
					basis.rotate_local(Vector3(1, 0, 0), Math::TAU * 0.25);
				}

				undo_redo->add_do_method(sp, "set_rotation", basis.get_euler_normalized());
				undo_redo->add_undo_method(sp, "set_rotation", sp->get_rotation());
			}
			undo_redo->commit_action();

		} break;
		case VIEW_ENVIRONMENT: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_ENVIRONMENT);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			if (current) {
				camera->set_environment(Ref<Resource>());
			} else {
				camera->set_environment(spatial_editor->get_viewport_environment());
			}

			view_display_menu->get_popup()->set_item_checked(idx, current);

		} break;
		case VIEW_PERSPECTIVE: {
			view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_PERSPECTIVE), true);
			view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_ORTHOGONAL), false);
			view_3d_controller->set_orthogonal(false);
			callable_mp(this, &Node3DEditorViewport::update_transform_gizmo_view).call_deferred();
			view_3d_controller->update_camera();
			_update_name();

		} break;
		case VIEW_ORTHOGONAL: {
			view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_PERSPECTIVE), false);
			view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_ORTHOGONAL), true);
			view_3d_controller->set_orthogonal(true);
			callable_mp(this, &Node3DEditorViewport::update_transform_gizmo_view).call_deferred();
			view_3d_controller->update_camera();
			_update_name();
		} break;
		case VIEW_SWITCH_PERSPECTIVE_ORTHOGONAL: {
			_menu_option(view_3d_controller->is_orthogonal() ? VIEW_PERSPECTIVE : VIEW_ORTHOGONAL);

		} break;
		case VIEW_AUTO_ORTHOGONAL: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_AUTO_ORTHOGONAL);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			view_display_menu->get_popup()->set_item_checked(idx, current);
			view_3d_controller->set_auto_orthogonal_allowed(current);
			_update_name();
		} break;
		case VIEW_LOCK_ROTATION: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_LOCK_ROTATION);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			_set_lock_view_rotation(!current);

		} break;
		case VIEW_AUDIO_LISTENER: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_LISTENER);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			viewport->set_as_audio_listener_3d(current);
			view_display_menu->get_popup()->set_item_checked(idx, current);

		} break;
		case VIEW_AUDIO_DOPPLER: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_DOPPLER);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			camera->set_doppler_tracking(current ? Camera3D::DOPPLER_TRACKING_IDLE_STEP : Camera3D::DOPPLER_TRACKING_DISABLED);
			view_display_menu->get_popup()->set_item_checked(idx, current);

		} break;
		case VIEW_CINEMATIC_PREVIEW: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_CINEMATIC_PREVIEW);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			view_display_menu->get_popup()->set_item_checked(idx, current);
			_toggle_cinema_preview(current);

			cinema_label->set_visible(current);
			_update_centered_labels();
			surface->queue_redraw();

			if (current) {
				preview_camera->hide();
			} else {
				if (previewing != nullptr) {
					preview_camera->show();
				}
			}
		} break;
		case VIEW_GIZMOS: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_GIZMOS);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			uint32_t layers = camera->get_cull_mask();
			layers &= ~(1 << GIZMO_EDIT_LAYER);
			if (current) {
				layers |= (1 << GIZMO_EDIT_LAYER);
			}
			camera->set_cull_mask(layers);
			view_display_menu->get_popup()->set_item_checked(idx, current);

		} break;
		case VIEW_TRANSFORM_GIZMO: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_TRANSFORM_GIZMO);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			transform_gizmo_visible = current;

			spatial_editor->update_transform_gizmo();
			view_display_menu->get_popup()->set_item_checked(idx, current);
		} break;
		case VIEW_HALF_RESOLUTION: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_HALF_RESOLUTION);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			view_display_menu->get_popup()->set_item_checked(idx, !current);
			_update_shrink();
		} break;
		case VIEW_INFORMATION: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_INFORMATION);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			view_display_menu->get_popup()->set_item_checked(idx, !current);

		} break;
		case VIEW_FRAME_TIME: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_FRAME_TIME);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			view_display_menu->get_popup()->set_item_checked(idx, !current);
		} break;
		case VIEW_GRID: {
			int idx = view_display_menu->get_popup()->get_item_index(VIEW_GRID);
			bool current = view_display_menu->get_popup()->is_item_checked(idx);
			current = !current;
			uint32_t layers = camera->get_cull_mask();
			layers &= ~(1 << GIZMO_GRID_LAYER);
			if (current) {
				layers |= (1 << GIZMO_GRID_LAYER);
			}
			camera->set_cull_mask(layers);
			view_display_menu->get_popup()->set_item_checked(idx, current);
		} break;
		case VIEW_DISPLAY_NORMAL:
		case VIEW_DISPLAY_WIREFRAME:
		case VIEW_DISPLAY_OVERDRAW:
		case VIEW_DISPLAY_UNSHADED:
		case VIEW_DISPLAY_LIGHTING:
		case VIEW_DISPLAY_NORMAL_BUFFER:
		case VIEW_DISPLAY_DEBUG_SHADOW_ATLAS:
		case VIEW_DISPLAY_DEBUG_DIRECTIONAL_SHADOW_ATLAS:
		case VIEW_DISPLAY_DEBUG_VOXEL_GI_ALBEDO:
		case VIEW_DISPLAY_DEBUG_VOXEL_GI_LIGHTING:
		case VIEW_DISPLAY_DEBUG_VOXEL_GI_EMISSION:
		case VIEW_DISPLAY_DEBUG_SCENE_LUMINANCE:
		case VIEW_DISPLAY_DEBUG_SSAO:
		case VIEW_DISPLAY_DEBUG_SSIL:
		case VIEW_DISPLAY_DEBUG_PSSM_SPLITS:
		case VIEW_DISPLAY_DEBUG_DECAL_ATLAS:
		case VIEW_DISPLAY_DEBUG_AREA_LIGHT_ATLAS:
		case VIEW_DISPLAY_DEBUG_SDFGI:
		case VIEW_DISPLAY_DEBUG_SDFGI_PROBES:
		case VIEW_DISPLAY_DEBUG_GI_BUFFER:
		case VIEW_DISPLAY_DEBUG_DISABLE_LOD:
		case VIEW_DISPLAY_DEBUG_CLUSTER_OMNI_LIGHTS:
		case VIEW_DISPLAY_DEBUG_CLUSTER_SPOT_LIGHTS:
		case VIEW_DISPLAY_DEBUG_CLUSTER_AREA_LIGHTS:
		case VIEW_DISPLAY_DEBUG_CLUSTER_DECALS:
		case VIEW_DISPLAY_DEBUG_CLUSTER_REFLECTION_PROBES:
		case VIEW_DISPLAY_DEBUG_OCCLUDERS:
		case VIEW_DISPLAY_MOTION_VECTORS:
		case VIEW_DISPLAY_INTERNAL_BUFFER: {
			static const int display_options[] = {
				VIEW_DISPLAY_NORMAL,
				VIEW_DISPLAY_WIREFRAME,
				VIEW_DISPLAY_OVERDRAW,
				VIEW_DISPLAY_UNSHADED,
				VIEW_DISPLAY_LIGHTING,
				VIEW_DISPLAY_NORMAL_BUFFER,
				VIEW_DISPLAY_DEBUG_SHADOW_ATLAS,
				VIEW_DISPLAY_DEBUG_DIRECTIONAL_SHADOW_ATLAS,
				VIEW_DISPLAY_DEBUG_VOXEL_GI_ALBEDO,
				VIEW_DISPLAY_DEBUG_VOXEL_GI_LIGHTING,
				VIEW_DISPLAY_DEBUG_VOXEL_GI_EMISSION,
				VIEW_DISPLAY_DEBUG_SCENE_LUMINANCE,
				VIEW_DISPLAY_DEBUG_SSAO,
				VIEW_DISPLAY_DEBUG_SSIL,
				VIEW_DISPLAY_DEBUG_GI_BUFFER,
				VIEW_DISPLAY_DEBUG_DISABLE_LOD,
				VIEW_DISPLAY_DEBUG_PSSM_SPLITS,
				VIEW_DISPLAY_DEBUG_DECAL_ATLAS,
				VIEW_DISPLAY_DEBUG_AREA_LIGHT_ATLAS,
				VIEW_DISPLAY_DEBUG_SDFGI,
				VIEW_DISPLAY_DEBUG_SDFGI_PROBES,
				VIEW_DISPLAY_DEBUG_CLUSTER_OMNI_LIGHTS,
				VIEW_DISPLAY_DEBUG_CLUSTER_SPOT_LIGHTS,
				VIEW_DISPLAY_DEBUG_CLUSTER_AREA_LIGHTS,
				VIEW_DISPLAY_DEBUG_CLUSTER_DECALS,
				VIEW_DISPLAY_DEBUG_CLUSTER_REFLECTION_PROBES,
				VIEW_DISPLAY_DEBUG_OCCLUDERS,
				VIEW_DISPLAY_MOTION_VECTORS,
				VIEW_DISPLAY_INTERNAL_BUFFER,
				VIEW_MAX
			};
			static const Viewport::DebugDraw debug_draw_modes[] = {
				Viewport::DEBUG_DRAW_DISABLED,
				Viewport::DEBUG_DRAW_WIREFRAME,
				Viewport::DEBUG_DRAW_OVERDRAW,
				Viewport::DEBUG_DRAW_UNSHADED,
				Viewport::DEBUG_DRAW_LIGHTING,
				Viewport::DEBUG_DRAW_NORMAL_BUFFER,
				Viewport::DEBUG_DRAW_SHADOW_ATLAS,
				Viewport::DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS,
				Viewport::DEBUG_DRAW_VOXEL_GI_ALBEDO,
				Viewport::DEBUG_DRAW_VOXEL_GI_LIGHTING,
				Viewport::DEBUG_DRAW_VOXEL_GI_EMISSION,
				Viewport::DEBUG_DRAW_SCENE_LUMINANCE,
				Viewport::DEBUG_DRAW_SSAO,
				Viewport::DEBUG_DRAW_SSIL,
				Viewport::DEBUG_DRAW_GI_BUFFER,
				Viewport::DEBUG_DRAW_DISABLE_LOD,
				Viewport::DEBUG_DRAW_PSSM_SPLITS,
				Viewport::DEBUG_DRAW_DECAL_ATLAS,
				Viewport::DEBUG_DRAW_AREA_LIGHT_ATLAS,
				Viewport::DEBUG_DRAW_SDFGI,
				Viewport::DEBUG_DRAW_SDFGI_PROBES,
				Viewport::DEBUG_DRAW_CLUSTER_OMNI_LIGHTS,
				Viewport::DEBUG_DRAW_CLUSTER_SPOT_LIGHTS,
				Viewport::DEBUG_DRAW_CLUSTER_AREA_LIGHTS,
				Viewport::DEBUG_DRAW_CLUSTER_DECALS,
				Viewport::DEBUG_DRAW_CLUSTER_REFLECTION_PROBES,
				Viewport::DEBUG_DRAW_OCCLUDERS,
				Viewport::DEBUG_DRAW_MOTION_VECTORS,
				Viewport::DEBUG_DRAW_INTERNAL_BUFFER,
			};

			for (int idx = 0; display_options[idx] != VIEW_MAX; idx++) {
				int id = display_options[idx];
				int item_idx = view_display_menu->get_popup()->get_item_index(id);
				if (item_idx != -1) {
					view_display_menu->get_popup()->set_item_checked(item_idx, id == p_option);
				}
				item_idx = display_submenu->get_item_index(id);
				if (item_idx != -1) {
					display_submenu->set_item_checked(item_idx, id == p_option);
				}

				if (id == p_option) {
					viewport->set_debug_draw(debug_draw_modes[idx]);
				}
			}
			spatial_editor->update_shading_buttons();
		} break;
	}
}

void Node3DEditorViewport::_preview_exited_scene() {
	preview_camera->disconnect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview));
	preview_camera->set_pressed(false);
	_toggle_camera_preview(false);
	preview_camera->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview));
	view_display_menu->show();
}

void Node3DEditorViewport::_preview_camera_property_changed() {
	if (previewing) {
		surface->queue_redraw();
	}
}

void Node3DEditorViewport::_sync_cursor_from_transform(const Transform3D &p_transform) {
	const Basis basis = p_transform.basis;

	view_3d_controller->cursor.eye_pos = p_transform.origin;
	view_3d_controller->cursor.x_rot = -basis.get_euler().x;
	view_3d_controller->cursor.y_rot = -basis.get_euler().y;
	view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
	view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;

	real_t distance = view_3d_controller->cursor.distance;
	if (view_3d_controller->is_orthogonal()) {
		distance = (get_zfar() - get_znear()) / 2.0;
	}
	view_3d_controller->cursor.pos = p_transform.origin - basis.get_column(2) * distance;
}

void Node3DEditorViewport::_update_centered_labels() {
	if (cinema_label->is_visible()) {
		cinema_label->reset_size();
		float cinema_half_width = cinema_label->get_size().width / 2.0f;
		cinema_label->set_anchor_and_offset(SIDE_LEFT, 0.5f, -cinema_half_width);
	}

	if (locked_label->is_visible()) {
		locked_label->reset_size();
		float locked_half_width = locked_label->get_size().width / 2.0f;
		locked_label->set_anchor_and_offset(SIDE_LEFT, 0.5f, -locked_half_width);
	}
}

void Node3DEditorViewport::_acquire_gizmo_layer() {
	const Ref<World3D> world = get_editing_world();
	const RID scenario = world.is_valid() ? world->get_scenario() : RID();
	if (scenario == gizmo_layer_scenario) {
		return;
	}
	_release_gizmo_layer();
	gizmo_layer_scenario = scenario;
	gizmo_layer = Node3DEditor::acquire_gizmo_layer(scenario);
}

void Node3DEditorViewport::_release_gizmo_layer() {
	if (gizmo_layer_scenario.is_null()) {
		return;
	}
	Node3DEditor::release_gizmo_layer(gizmo_layer_scenario, gizmo_layer);
	gizmo_layer_scenario = RID();
	gizmo_layer = GIZMO_BASE_LAYER;
}

void Node3DEditorViewport::_apply_gizmo_layer() {
	const uint32_t layer = 1 << gizmo_layer;
	// The manipulator instances only exist while the view is in the tree; the
	// camera's mask is worth setting either way.
	if (move_gizmo_instance[0].is_valid()) {
		for (int i = 0; i < 3; i++) {
			RS::get_singleton()->instance_set_layer_mask(move_gizmo_instance[i], layer);
			RS::get_singleton()->instance_set_layer_mask(move_plane_gizmo_instance[i], layer);
			RS::get_singleton()->instance_set_layer_mask(scale_gizmo_instance[i], layer);
			RS::get_singleton()->instance_set_layer_mask(scale_plane_gizmo_instance[i], layer);
			RS::get_singleton()->instance_set_layer_mask(axis_gizmo_instance[i], layer);
		}
		for (int i = 0; i < 4; i++) {
			RS::get_singleton()->instance_set_layer_mask(rotate_gizmo_instance[i], layer);
		}
		if (trackball_sphere_instance.is_valid()) {
			RS::get_singleton()->instance_set_layer_mask(trackball_sphere_instance, layer);
		}
	}
	// The camera sees the scene, the shared editor visuals, and its own
	// manipulator - never another view's.
	camera->set_cull_mask(((1 << 20) - 1) | layer | (1 << GIZMO_EDIT_LAYER) | (1 << GIZMO_GRID_LAYER) | (1 << MISC_TOOL_LAYER));
}

void Node3DEditorViewport::_init_gizmo_instance(int p_idx) {
	_acquire_gizmo_layer();
	uint32_t layer = 1 << gizmo_layer;

	for (int i = 0; i < 3; i++) {
		move_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(move_gizmo_instance[i], spatial_editor->get_move_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(move_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(move_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(move_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(move_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(move_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		move_plane_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(move_plane_gizmo_instance[i], spatial_editor->get_move_plane_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(move_plane_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(move_plane_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(move_plane_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(move_plane_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(move_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(move_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		scale_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(scale_gizmo_instance[i], spatial_editor->get_scale_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(scale_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(scale_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(scale_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(scale_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(scale_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(scale_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		scale_plane_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(scale_plane_gizmo_instance[i], spatial_editor->get_scale_plane_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(scale_plane_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(scale_plane_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(scale_plane_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(scale_plane_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(scale_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(scale_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		axis_gizmo_instance[i] = RS::get_singleton()->instance_create();
	}

	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->instance_set_base(axis_gizmo_instance[i], spatial_editor->get_axis_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(axis_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(axis_gizmo_instance[i], true);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(axis_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(axis_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(axis_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(axis_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	}

	for (int i = 0; i < 4; i++) {
		rotate_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(rotate_gizmo_instance[i], spatial_editor->get_rotate_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(rotate_gizmo_instance[i], get_editing_world()->get_scenario());
		RS::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(rotate_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(rotate_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(rotate_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(rotate_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	}

	// Create trackball sphere instance
	trackball_sphere_instance = RS::get_singleton()->instance_create();
	RS::get_singleton()->instance_set_base(trackball_sphere_instance, spatial_editor->get_trackball_sphere_gizmo()->get_rid());
	RS::get_singleton()->instance_set_scenario(trackball_sphere_instance, get_editing_world()->get_scenario());
	RS::get_singleton()->instance_set_visible(trackball_sphere_instance, false);
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(trackball_sphere_instance, RSE::SHADOW_CASTING_SETTING_OFF);
	RS::get_singleton()->instance_set_layer_mask(trackball_sphere_instance, layer);
	RS::get_singleton()->instance_geometry_set_flag(trackball_sphere_instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(trackball_sphere_instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
}

void Node3DEditorViewport::_finish_gizmo_instances() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	// Cleared, not just freed: a view leaves and re-enters the tree now, and a
	// freed RID still reads as valid, so code that asks whether the instances
	// are there would be answered wrongly.
	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->free_rid(move_gizmo_instance[i]);
		RS::get_singleton()->free_rid(move_plane_gizmo_instance[i]);
		RS::get_singleton()->free_rid(rotate_gizmo_instance[i]);
		RS::get_singleton()->free_rid(scale_gizmo_instance[i]);
		RS::get_singleton()->free_rid(scale_plane_gizmo_instance[i]);
		RS::get_singleton()->free_rid(axis_gizmo_instance[i]);
		move_gizmo_instance[i] = RID();
		move_plane_gizmo_instance[i] = RID();
		rotate_gizmo_instance[i] = RID();
		scale_gizmo_instance[i] = RID();
		scale_plane_gizmo_instance[i] = RID();
		axis_gizmo_instance[i] = RID();
	}
	// Rotation white outline
	RS::get_singleton()->free_rid(rotate_gizmo_instance[3]);
	rotate_gizmo_instance[3] = RID();

	RS::get_singleton()->free_rid(trackball_sphere_instance);
	trackball_sphere_instance = RID();
}

void Node3DEditorViewport::_disable_follow_mode() {
	// Exit follow mode by resetting the number of times the follow shortcut was used consecutively.
	times_focused_consecutively = 0;
}

void Node3DEditorViewport::_reset_follow_mode_count() {
	bool is_in_follow_mode = times_focused_consecutively >= 2 && times_focused_consecutively % 2 == 0;
	if (!is_in_follow_mode) {
		times_focused_consecutively = 0;
	}
}

void Node3DEditorViewport::_toggle_camera_preview(bool p_activate) {
	ERR_FAIL_COND(p_activate && !preview);
	ERR_FAIL_COND(!p_activate && !previewing);

	emit_signal(SNAME("clicked"));
	previewing_camera = p_activate;
	_update_navigation_controls_visibility();

	if (!p_activate) {
		_pilot_commit_undo_session();
		previewing->disconnect(SceneStringName(tree_exiting), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
		previewing->disconnect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
		previewing = nullptr;
		RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), camera->get_camera()); //restore
		if (!preview) {
			preview_camera->hide();
		}
		pilot_camera->hide();
		pilot_preview_enabled = false;
		pilot_camera->set_pressed(false);

		_apply_camera_transform_to_cursor();
		view_3d_controller->update_camera(0);
		last_camera_transform = camera->get_global_transform();

		surface->queue_redraw();

	} else {
		previewing = preview;
		previewing->connect(SceneStringName(tree_exiting), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
		previewing->connect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
		RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), preview->get_camera()); //replace

		_sync_cursor_from_transform(preview->get_global_transform());
		view_3d_controller->update_camera(0);

		pilot_camera->show();

		if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
			pilot_camera->set_pressed(true);
		}

		surface->queue_redraw();
	}
}

void Node3DEditorViewport::_toggle_pilot_preview(bool p_activate) {
	if (!p_activate) {
		_pilot_commit_undo_session();
	}
	pilot_preview_enabled = p_activate;
	if (p_activate && previewing) {
		_sync_cursor_from_transform(previewing->get_global_transform());
		view_3d_controller->update_camera(0);
	}
}

void Node3DEditorViewport::_toggle_cinema_preview(bool p_activate) {
	previewing_cinema = p_activate;
	_update_navigation_controls_visibility();

	if (!previewing_cinema) {
		if (previewing != nullptr) {
			previewing->disconnect(SceneStringName(tree_exited), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
			previewing->disconnect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
		}

		previewing = nullptr;
		RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), camera->get_camera()); //restore
		preview_camera->set_pressed(false);
		if (!preview) {
			preview_camera->hide();
		} else {
			preview_camera->show();
		}
		view_display_menu->show();
		surface->queue_redraw();
	}
}

void Node3DEditorViewport::_selection_result_pressed(int p_result) {
	if (selection_results_menu.size() <= p_result) {
		return;
	}

	clicked = selection_results_menu[p_result]->get_instance_id();

	if (clicked.is_valid()) {
		_select_clicked(true);
	}

	selection_results_menu.clear();
}

void Node3DEditorViewport::_selection_menu_hide() {
	selection_results.clear();
	selection_menu->clear();
	selection_menu->reset_size();
}

void Node3DEditorViewport::set_can_preview(Camera3D *p_preview) {
	preview = p_preview;

	if (!preview_camera->is_pressed() && !previewing_cinema) {
		preview_camera->set_visible(p_preview);
	}
}

void Node3DEditorViewport::switch_preview_camera(Camera3D *p_new_camera) {
	if (!previewing_camera || !previewing || !p_new_camera || p_new_camera == previewing) {
		return;
	}

	_pilot_commit_undo_session();

	previewing->disconnect(SceneStringName(tree_exiting), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
	previewing->disconnect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));

	previewing = p_new_camera;
	previewing->connect(SceneStringName(tree_exiting), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
	previewing->connect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
	RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), p_new_camera->get_camera());

	_sync_cursor_from_transform(p_new_camera->get_global_transform());
	view_3d_controller->update_camera(0);

	surface->queue_redraw();
}

void Node3DEditorViewport::update_transform_gizmo_view() {
	if (!is_visible_in_tree()) {
		return;
	}
	// A view built this frame has not finished entering the tree: its camera
	// cannot unproject yet and its manipulator instances do not exist. Panels
	// are built on demand now, so this is reachable rather than theoretical.
	if (!camera->is_inside_tree() || !move_gizmo_instance[0].is_valid()) {
		return;
	}

	Transform3D xform = spatial_editor->get_gizmo_transform();

	Transform3D camera_xform = camera->get_transform();

	if (xform.origin.is_equal_approx(camera_xform.origin)) {
		for (int i = 0; i < 3; i++) {
			RenderingServer::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(move_plane_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(scale_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(scale_plane_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(axis_gizmo_instance[i], false);
		}
		RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], false);
		return;
	}

	const Vector3 camz = -camera_xform.get_basis().get_column(2).normalized();
	const Vector3 camy = -camera_xform.get_basis().get_column(1).normalized();
	const Plane p = Plane(camz, camera_xform.origin);
	const real_t gizmo_d = MAX(Math::abs(p.distance_to(xform.origin)), CMP_EPSILON);
	const real_t d0 = camera->unproject_position(camera_xform.origin + camz * gizmo_d).y;
	const real_t d1 = camera->unproject_position(camera_xform.origin + camz * gizmo_d + camy).y;
	const real_t dd = MAX(Math::abs(d0 - d1), CMP_EPSILON);

	const real_t gizmo_size = EDITOR_GET("editors/3d/manipulator_gizmo_size");
	// At low viewport heights, multiply the gizmo scale based on the viewport height.
	// This prevents the gizmo from growing very large and going outside the viewport.
	const int viewport_base_height = 400 * MAX(1, EDSCALE);
	gizmo_scale =
			(gizmo_size / Math::abs(dd)) * MAX(1, EDSCALE) *
			MIN(viewport_base_height, subviewport_container->get_size().height) / viewport_base_height;
	Vector3 scale = Vector3(1, 1, 1) * gizmo_scale;

	// If the determinant is zero, we should disable the gizmo from being rendered,
	// this prevents supplying bad values to the renderer and then having to filter it out again.
	if (xform.basis.determinant() == 0) {
		for (int i = 0; i < 3; i++) {
			RenderingServer::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(move_plane_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(scale_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(scale_plane_gizmo_instance[i], false);
			RenderingServer::get_singleton()->instance_set_visible(axis_gizmo_instance[i], false);
		}
		RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], false);
		return;
	}

	bool local_coords = spatial_editor->are_local_coords_enabled();
	bool arc_visible = _is_rotation_arc_visible();
	int show_gizmo_flags = EDITOR_GET("editors/3d/show_gizmo_during_rotation");

	bool keep_gizmo_visible = arc_visible && ((local_coords && (show_gizmo_flags & Node3DEditor::TRANSFORM_MODE_LOCAL)) || (!local_coords && (show_gizmo_flags & Node3DEditor::TRANSFORM_MODE_GLOBAL)));
	bool hide_gizmo_during_rotation = arc_visible && !keep_gizmo_visible;
	bool hide_gizmo_during_trackball = (_edit.mode == TRANSFORM_ROTATE && _edit.is_trackball);

	int arc_replaces_ring = -1;
	if (keep_gizmo_visible) {
		switch (_edit.plane) {
			case TRANSFORM_X_AXIS:
				arc_replaces_ring = 0;
				break;
			case TRANSFORM_Y_AXIS:
				arc_replaces_ring = 1;
				break;
			case TRANSFORM_Z_AXIS:
				arc_replaces_ring = 2;
				break;
			case TRANSFORM_VIEW:
				arc_replaces_ring = 3;
				break;
			default:
				break;
		}
	}

	bool show_gizmo = spatial_editor->is_gizmo_visible() && !_edit.instant && transform_gizmo_visible && !collision_reposition && !hide_gizmo_during_rotation && !hide_gizmo_during_trackball;
	bool show_rotate_gizmo = show_gizmo && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_ROTATE);

	for (int i = 0; i < 3; i++) {
		Transform3D axis_angle;
		if (xform.basis.get_column(i).normalized().dot(xform.basis.get_column((i + 1) % 3).normalized()) < 1.0) {
			axis_angle = axis_angle.looking_at(xform.basis.get_column(i).normalized(), xform.basis.get_column((i + 1) % 3).normalized());
		}
		axis_angle.basis.scale(scale);
		axis_angle.origin = xform.origin;
		RenderingServer::get_singleton()->instance_set_transform(move_gizmo_instance[i], axis_angle);
		RenderingServer::get_singleton()->instance_set_visible(move_gizmo_instance[i], show_gizmo && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_MOVE));
		RenderingServer::get_singleton()->instance_set_transform(move_plane_gizmo_instance[i], axis_angle);
		RenderingServer::get_singleton()->instance_set_visible(move_plane_gizmo_instance[i], show_gizmo && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_MOVE));
		RenderingServer::get_singleton()->instance_set_transform(rotate_gizmo_instance[i], axis_angle);
		RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], show_rotate_gizmo && i != arc_replaces_ring);
		RenderingServer::get_singleton()->instance_set_transform(scale_gizmo_instance[i], axis_angle);
		RenderingServer::get_singleton()->instance_set_visible(scale_gizmo_instance[i], show_gizmo && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_SCALE));
		RenderingServer::get_singleton()->instance_set_transform(scale_plane_gizmo_instance[i], axis_angle);
		RenderingServer::get_singleton()->instance_set_visible(scale_plane_gizmo_instance[i], show_gizmo && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_SCALE));
		RenderingServer::get_singleton()->instance_set_transform(axis_gizmo_instance[i], xform);
	}

	Transform3D view_rotation_xform = xform;
	view_rotation_xform.orthonormalize();

	bool can_show_trackball = spatial_editor->is_gizmo_visible() && !_edit.instant && transform_gizmo_visible && !collision_reposition && !hide_gizmo_during_rotation;
	bool show_trackball_sphere = can_show_trackball && (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM || spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_ROTATE) && !hide_gizmo_during_trackball;
	Transform3D trackball_xform = view_rotation_xform;
	trackball_xform.basis.scale(scale);
	RenderingServer::get_singleton()->instance_set_transform(trackball_sphere_instance, trackball_xform);
	RenderingServer::get_singleton()->instance_set_visible(trackball_sphere_instance, show_trackball_sphere);

	bool shrink_view_ring = arc_replaces_ring >= 0 && arc_replaces_ring < 3;
	Vector3 view_ring_scale = shrink_view_ring ? scale : scale * (spatial_editor->gizmo_view_rotation_scale / GIZMO_CIRCLE_SIZE);
	view_rotation_xform.basis.scale(view_ring_scale);
	RenderingServer::get_singleton()->instance_set_transform(rotate_gizmo_instance[3], view_rotation_xform);
	RenderingServer::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], show_rotate_gizmo && arc_replaces_ring != 3);

	bool show_axes = spatial_editor->is_gizmo_visible() && _edit.mode != TRANSFORM_NONE && !hide_gizmo_during_trackball;
	RenderingServer *rs = RenderingServer::get_singleton();
	rs->instance_set_visible(axis_gizmo_instance[0], show_axes && (_edit.plane == TRANSFORM_X_AXIS || _edit.plane == TRANSFORM_XY || _edit.plane == TRANSFORM_XZ));
	rs->instance_set_visible(axis_gizmo_instance[1], show_axes && (_edit.plane == TRANSFORM_Y_AXIS || _edit.plane == TRANSFORM_XY || _edit.plane == TRANSFORM_YZ));
	rs->instance_set_visible(axis_gizmo_instance[2], show_axes && (_edit.plane == TRANSFORM_Z_AXIS || _edit.plane == TRANSFORM_XZ || _edit.plane == TRANSFORM_YZ));
}

void Node3DEditorViewport::update_transform_gizmo_highlight() {
	if (!is_visible_in_tree() || !Rect2(Vector2(), surface->get_size()).has_point(surface->get_local_mouse_position())) {
		return;
	}
	_transform_gizmo_select(surface->get_local_mouse_position(), true);
}

void Node3DEditorViewport::set_state(const Dictionary &p_state) {
	if (p_state.has("position")) {
		view_3d_controller->cursor.pos = p_state["position"];
	}
	if (p_state.has("x_rotation")) {
		view_3d_controller->cursor.x_rot = p_state["x_rotation"];
		view_3d_controller->cursor.unsnapped_x_rot = view_3d_controller->cursor.x_rot;
	}
	if (p_state.has("y_rotation")) {
		view_3d_controller->cursor.y_rot = p_state["y_rotation"];
		view_3d_controller->cursor.unsnapped_y_rot = view_3d_controller->cursor.y_rot;
	}
	if (p_state.has("distance")) {
		view_3d_controller->cursor.distance = p_state["distance"];
	}
	if (p_state.has("orthogonal")) {
		bool orth = p_state["orthogonal"];
		_menu_option(orth ? VIEW_ORTHOGONAL : VIEW_PERSPECTIVE);
	}
	if (p_state.has("view_type")) {
		view_3d_controller->set_view_type(View3DController::ViewType(p_state["view_type"].operator int()));
	}
	if (p_state.has("auto_orthogonal_enabled")) {
		bool enabled = p_state["auto_orthogonal_enabled"];
		view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_AUTO_ORTHOGONAL), enabled);
		view_3d_controller->set_auto_orthogonal_allowed(enabled);
	}
	if (p_state.has("auto_orthogonal")) {
		if (p_state["auto_orthogonal"]) {
			view_3d_controller->force_auto_orthogonal();
			_update_name();
		}
	}
	if (p_state.has("display_mode")) {
		int display = p_state["display_mode"];

		int idx = view_display_menu->get_popup()->get_item_index(display);
		if (idx != -1 && !view_display_menu->get_popup()->is_item_checked(idx)) {
			_menu_option(display);
		} else {
			idx = display_submenu->get_item_index(display);
			if (idx != -1 && !display_submenu->is_item_checked(idx)) {
				_menu_option(display);
			}
		}
	}
	if (p_state.has("lock_rotation")) {
		_set_lock_view_rotation(p_state["lock_rotation"]);
	}
	if (p_state.has("use_environment")) {
		bool env = p_state["use_environment"];

		if (env != camera->get_environment().is_valid()) {
			_menu_option(VIEW_ENVIRONMENT);
		}
	}
	if (p_state.has("listener")) {
		bool listener = p_state["listener"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_LISTENER);
		viewport->set_as_audio_listener_3d(listener);
		view_display_menu->get_popup()->set_item_checked(idx, listener);
	}
	if (p_state.has("doppler")) {
		bool doppler = p_state["doppler"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_DOPPLER);
		camera->set_doppler_tracking(doppler ? Camera3D::DOPPLER_TRACKING_IDLE_STEP : Camera3D::DOPPLER_TRACKING_DISABLED);
		view_display_menu->get_popup()->set_item_checked(idx, doppler);
	}
	if (p_state.has("gizmos")) {
		bool gizmos = p_state["gizmos"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_GIZMOS);
		if (view_display_menu->get_popup()->is_item_checked(idx) != gizmos) {
			_menu_option(VIEW_GIZMOS);
		}
	}
	if (p_state.has("transform_gizmo")) {
		bool transform_gizmo = p_state["transform_gizmo"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_TRANSFORM_GIZMO);
		if (view_display_menu->get_popup()->is_item_checked(idx) != transform_gizmo) {
			_menu_option(VIEW_TRANSFORM_GIZMO);
		}
	}
	if (p_state.has("grid")) {
		bool grid = p_state["grid"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_GRID);
		if (view_display_menu->get_popup()->is_item_checked(idx) != grid) {
			_menu_option(VIEW_GRID);
		}
	}
	if (p_state.has("information")) {
		bool information = p_state["information"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_INFORMATION);
		if (view_display_menu->get_popup()->is_item_checked(idx) != information) {
			_menu_option(VIEW_INFORMATION);
		}
	}
	if (p_state.has("frame_time")) {
		bool fps = p_state["frame_time"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_FRAME_TIME);
		if (view_display_menu->get_popup()->is_item_checked(idx) != fps) {
			_menu_option(VIEW_FRAME_TIME);
		}
	}
	if (p_state.has("half_res")) {
		bool half_res = p_state["half_res"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_HALF_RESOLUTION);
		view_display_menu->get_popup()->set_item_checked(idx, half_res);
		_update_shrink();
	}
	if (p_state.has("cinematic_preview")) {
		previewing_cinema = p_state["cinematic_preview"];

		int idx = view_display_menu->get_popup()->get_item_index(VIEW_CINEMATIC_PREVIEW);
		view_display_menu->get_popup()->set_item_checked(idx, previewing_cinema);

		cinema_label->set_visible(previewing_cinema);
		if (previewing_cinema) {
			_update_centered_labels();
			surface->queue_redraw();
		}
	}

	if (preview_camera->is_connected(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview))) {
		preview_camera->disconnect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview));
	}
	if (p_state.has("previewing")) {
		Node *pv = get_edited_scene()->get_node(p_state["previewing"]);
		if (Object::cast_to<Camera3D>(pv)) {
			previewing = Object::cast_to<Camera3D>(pv);
			previewing->connect(SceneStringName(tree_exiting), callable_mp(this, &Node3DEditorViewport::_preview_exited_scene));
			previewing->connect(CoreStringName(property_list_changed), callable_mp(this, &Node3DEditorViewport::_preview_camera_property_changed));
			RS::get_singleton()->viewport_attach_camera(viewport->get_viewport_rid(), previewing->get_camera()); //replace
			surface->queue_redraw();
			previewing_camera = true;
			_update_navigation_controls_visibility();
			preview_camera->set_pressed(true);
			preview_camera->show();
			pilot_camera->show();

			camera->set_global_transform(view_3d_controller->to_camera_transform());

			_sync_cursor_from_transform(previewing->get_global_transform());
			view_3d_controller->update_camera(0);
		}
	}
	preview_camera->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview));
}

Dictionary Node3DEditorViewport::get_state() const {
	Dictionary d;
	d["position"] = view_3d_controller->cursor.pos;
	d["x_rotation"] = view_3d_controller->cursor.x_rot;
	d["y_rotation"] = view_3d_controller->cursor.y_rot;
	d["distance"] = view_3d_controller->cursor.distance;
	d["use_environment"] = camera->get_environment().is_valid();
	d["orthogonal"] = camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL;
	d["view_type"] = view_3d_controller->get_view_type();
	d["auto_orthogonal"] = view_3d_controller->get_orthogonal_mode() == View3DController::ORTHOGONAL_AUTO;
	d["auto_orthogonal_enabled"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_AUTO_ORTHOGONAL));

	// Find selected display mode.
	int display_mode = VIEW_DISPLAY_NORMAL;
	for (int i = VIEW_DISPLAY_NORMAL; i < VIEW_DISPLAY_ADVANCED; i++) {
		if (view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(i))) {
			display_mode = i;
			break;
		}
	}
	for (int i = VIEW_DISPLAY_ADVANCED + 1; i < VIEW_DISPLAY_MAX; i++) {
		if (display_submenu->is_item_checked(display_submenu->get_item_index(i))) {
			display_mode = i;
			break;
		}
	}
	d["display_mode"] = display_mode;

	d["listener"] = viewport->is_audio_listener_3d();
	d["doppler"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_DOPPLER));
	d["gizmos"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_GIZMOS));
	d["transform_gizmo"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_TRANSFORM_GIZMO));
	d["grid"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_GRID));
	d["information"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_INFORMATION));
	d["frame_time"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_FRAME_TIME));
	d["half_res"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_HALF_RESOLUTION));
	d["cinematic_preview"] = view_display_menu->get_popup()->is_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_CINEMATIC_PREVIEW));
	if (previewing) {
		d["previewing"] = get_edited_scene()->get_path_to(previewing);
	}
	d["lock_rotation"] = view_3d_controller->is_locking_rotation();

	return d;
}

void Node3DEditorViewport::_bind_methods() {
	ADD_SIGNAL(MethodInfo("toggle_maximize_view", PropertyInfo(Variant::OBJECT, "viewport")));
	ADD_SIGNAL(MethodInfo("clicked"));
}

void Node3DEditorViewport::reset() {
	view_3d_controller->set_orthogonal(false);
	view_3d_controller->set_view_type(View3DController::VIEW_TYPE_USER);
	message_time = 0;
	message = "";
	last_message = "";

	view_3d_controller->cursor = View3DController::Cursor();
}

void Node3DEditorViewport::focus_selection() {
	Vector3 center;
	int count = 0;

	const List<Node *> &selection = editor_selection->get_top_selected_node_list();
	focused_node_id = ObjectID();
	if (!selection.is_empty()) {
		focused_node_id = selection.front()->get()->get_instance_id();
	}

	for (Node *node : selection) {
		Node3D *node_3d = Object::cast_to<Node3D>(node);
		if (!node_3d) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(node_3d);
		if (!se) {
			continue;
		}

		if (se->gizmo.is_valid()) {
			for (const KeyValue<int, Transform3D> &GE : se->subgizmos) {
				const Vector3 pos = node_3d->get_global_gizmo_transform().xform(se->gizmo->get_subgizmo_transform(GE.key).origin);
				if (pos.is_finite()) {
					center += pos;
					count++;
				}
			}
		} else {
			const Vector3 pos = node_3d->get_global_gizmo_transform().origin;
			if (pos.is_finite()) {
				center += pos;
				count++;
			}
		}
	}

	if (count > 1) {
		center /= count;
	}

	view_3d_controller->cursor.pos = center;
}

void Node3DEditorViewport::assign_pending_data_pointers(Node3D *p_preview_node, AABB *p_preview_bounds, AcceptDialog *p_accept) {
	preview_node = p_preview_node;
	preview_bounds = p_preview_bounds;
	accept = p_accept;
}

void _insert_rid_recursive(Node *node, HashSet<RID> &rids) {
	CollisionObject3D *co = Object::cast_to<CollisionObject3D>(node);

	if (co) {
		rids.insert(co->get_rid());
	} else if (node->is_class("CSGShape3D")) { // HACK: We should avoid referencing module logic.
		rids.insert(node->call("_get_root_collision_instance"));
	}

	for (int i = 0; i < node->get_child_count(); i++) {
		Node *child = node->get_child(i);
		_insert_rid_recursive(child, rids);
	}
}

Vector3 Node3DEditorViewport::_get_instance_position(const Point2 &p_pos, Node3D *p_node) const {
	const float MAX_DISTANCE = 50.0;
	const float FALLBACK_DISTANCE = 5.0;

	Vector3 world_ray = get_ray(p_pos);
	Vector3 world_pos = get_ray_pos(p_pos);

	PhysicsDirectSpaceState3D *ss = get_editing_world()->get_direct_space_state();

	HashSet<RID> rids;

	if (preview_node && preview_node->get_child_count() > 0) {
		_insert_rid_recursive(preview_node, rids);
	} else if (!preview_node->is_inside_tree() && !ruler->is_inside_tree()) {
		const List<Node *> &selection = editor_selection->get_top_selected_node_list();

		Node3D *first_selected_node = Object::cast_to<Node3D>(selection.front()->get());

		if (first_selected_node) {
			_insert_rid_recursive(first_selected_node, rids);
		}
	}

	PhysicsDirectSpaceState3D::RayParameters ray_params;
	ray_params.exclude = rids;
	ray_params.from = world_pos;
	ray_params.to = world_pos + world_ray * camera->get_far();

	PhysicsDirectSpaceState3D::RayResult result;
	if (ss->intersect_ray(ray_params, result) && (preview_node->get_child_count() > 0 || !preview_node->is_inside_tree())) {
		// Calculate an offset for the `p_node` such that the its bounding box is on top of and touching the contact surface's plane.

		// Use the Gram-Schmidt process to get an orthonormal Basis aligned with the surface normal.
		const Vector3 bb_basis_x = result.normal;
		Vector3 bb_basis_y = Vector3(0, 1, 0);
		bb_basis_y = bb_basis_y - bb_basis_y.project(bb_basis_x);
		if (bb_basis_y.is_zero_approx()) {
			bb_basis_y = Vector3(0, 0, 1);
			bb_basis_y = bb_basis_y - bb_basis_y.project(bb_basis_x);
		}
		bb_basis_y = bb_basis_y.normalized();
		const Vector3 bb_basis_z = bb_basis_x.cross(bb_basis_y);
		const Basis bb_basis = Basis(bb_basis_x, bb_basis_y, bb_basis_z);

		// This normal-aligned Basis allows us to create an AABB that can fit on the surface plane as snugly as possible.
		const Transform3D bb_transform = Transform3D(bb_basis, p_node->get_global_transform().origin);
		const AABB p_node_bb = _calculate_spatial_bounds(p_node, true, &bb_transform);
		// The x-axis's alignment with the surface normal also makes it trivial to get the distance from `p_node`'s origin at (0, 0, 0) to the correct AABB face.
		const float offset_distance = -p_node_bb.position.x;

		// `result_offset` is in global space.
		const Vector3 result_offset = result.position + result.normal * offset_distance;

		return result_offset;
	}

	const bool is_orthogonal = camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL;

	// The XZ plane.
	Vector3 intersection;
	Plane plane(Vector3(0, 1, 0));
	if (plane.intersects_ray(world_pos, world_ray, &intersection)) {
		if (is_orthogonal || world_pos.distance_to(intersection) <= MAX_DISTANCE) {
			return intersection;
		}
	}

	// Plane facing the camera using fallback distance.
	if (is_orthogonal) {
		plane = Plane(world_ray, view_3d_controller->cursor.pos - world_ray * (view_3d_controller->cursor.distance - FALLBACK_DISTANCE));
	} else {
		plane = Plane(world_ray, world_pos + world_ray * FALLBACK_DISTANCE);
	}
	if (plane.intersects_ray(world_pos, world_ray, &intersection)) {
		return intersection;
	}

	// Not likely, but just in case...
	return world_pos + world_ray * FALLBACK_DISTANCE;
}

AABB Node3DEditorViewport::_calculate_spatial_bounds(const Node3D *p_parent, bool p_omit_top_level, const Transform3D *p_bounds_orientation) {
	if (!p_parent) {
		return AABB(Vector3(-0.2, -0.2, -0.2), Vector3(0.4, 0.4, 0.4));
	}
	const Transform3D parent_transform = p_parent->get_global_transform();
	if (!parent_transform.is_finite()) {
		return AABB();
	}
	AABB bounds;

	Transform3D bounds_orientation;
	Transform3D xform_to_top_level_parent_space;
	if (p_bounds_orientation) {
		bounds_orientation = *p_bounds_orientation;
		xform_to_top_level_parent_space = bounds_orientation.affine_inverse() * parent_transform;
	} else {
		bounds_orientation = parent_transform;
	}

	const VisualInstance3D *visual_instance = Object::cast_to<VisualInstance3D>(p_parent);
	if (visual_instance) {
		bounds = visual_instance->get_aabb();
	} else {
		bounds = AABB();
	}
	bounds = xform_to_top_level_parent_space.xform(bounds);

	for (int i = 0; i < p_parent->get_child_count(); i++) {
		const Node3D *child = Object::cast_to<Node3D>(p_parent->get_child(i));
		if (child && !(p_omit_top_level && child->is_set_as_top_level())) {
			const AABB child_bounds = _calculate_spatial_bounds(child, p_omit_top_level, &bounds_orientation);
			bounds.merge_with(child_bounds);
		}
	}

	return bounds;
}

Node *Node3DEditorViewport::_sanitize_preview_node(Node *p_node) const {
	Node3D *node_3d = Object::cast_to<Node3D>(p_node);
	if (node_3d == nullptr) {
		Node3D *replacement_node = memnew(Node3D);
		replacement_node->set_name(p_node->get_name());
		p_node->replace_by(replacement_node);
		memdelete(p_node);
		p_node = replacement_node;
	} else {
		VisualInstance3D *visual_instance = Object::cast_to<VisualInstance3D>(node_3d);
		if (visual_instance == nullptr) {
			Node3D *replacement_node = memnew(Node3D);
			replacement_node->set_name(node_3d->get_name());
			replacement_node->set_visible(node_3d->is_visible());
			replacement_node->set_transform(node_3d->get_transform());
			replacement_node->set_rotation_edit_mode(node_3d->get_rotation_edit_mode());
			replacement_node->set_rotation_order(node_3d->get_rotation_order());
			replacement_node->set_as_top_level(node_3d->is_set_as_top_level());
			p_node->replace_by(replacement_node);
			memdelete(p_node);
			p_node = replacement_node;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_sanitize_preview_node(p_node->get_child(i));
	}

	return p_node;
}

void Node3DEditorViewport::_create_preview_node(const Vector<String> &files) const {
	bool add_preview = false;
	for (const String &path : files) {
		Ref<Resource> res = ResourceLoader::load(path);
		ERR_CONTINUE(res.is_null());

		Ref<PackedScene> scene = res;
		if (scene.is_valid()) {
			Node *instance = scene->instantiate();
			if (instance) {
				instance = _sanitize_preview_node(instance);
				preview_node->add_child(instance);
				Node3D *node_3d = Object::cast_to<Node3D>(instance);
				if (node_3d) {
					node_3d->set_as_top_level(false);
				}
			}
			add_preview = true;
		}

		Ref<Mesh> mesh = res;
		if (mesh.is_valid()) {
			MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
			mesh_instance->set_mesh(mesh);
			preview_node->add_child(mesh_instance);
			add_preview = true;
		}

		Ref<AudioStream> audio = res;
		if (audio.is_valid()) {
			Sprite3D *sprite = memnew(Sprite3D);
			sprite->set_texture(get_editor_theme_icon(SNAME("Gizmo3DSamplePlayer")));
			sprite->set_billboard_mode(StandardMaterial3D::BILLBOARD_ENABLED);
			sprite->set_pixel_size(0.005);
			preview_node->add_child(sprite);
			add_preview = true;
		}
	}
	if (add_preview) {
		get_scene_root()->add_child(preview_node);
		*preview_bounds = _calculate_spatial_bounds(preview_node);
	}
}

void Node3DEditorViewport::_remove_preview_node() {
	tooltip_panel->hide();

	set_message("");
	if (preview_node->get_parent()) {
		for (int i = preview_node->get_child_count() - 1; i >= 0; i--) {
			Node *node = preview_node->get_child(i);
			node->queue_free();
			preview_node->remove_child(node);
		}
		get_scene_root()->remove_child(preview_node);
	}
}

bool Node3DEditorViewport::_apply_preview_material(ObjectID p_target, const Point2 &p_point) const {
	_reset_preview_material();

	if (p_target.is_null()) {
		return false;
	}

	spatial_editor->set_preview_material_target(p_target);

	Object *target_inst = ObjectDB::get_instance(p_target);

	bool is_ctrl = Input::get_singleton()->is_key_pressed(Key::CTRL);

	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(target_inst);
	if (is_ctrl && mesh_instance) {
		Ref<Mesh> mesh = mesh_instance->get_mesh();
		int surface_count = mesh->get_surface_count();

		Vector3 world_ray = get_ray(p_point);
		Vector3 world_pos = get_ray_pos(p_point);

		int closest_surface = -1;
		float closest_dist = 1e20;

		Transform3D gt = mesh_instance->get_global_transform();

		Transform3D ai = gt.affine_inverse();
		Vector3 xform_ray = ai.basis.xform(world_ray).normalized();
		Vector3 xform_pos = ai.xform(world_pos);

		for (int surface_idx = 0; surface_idx < surface_count; surface_idx++) {
			Ref<TriangleMesh> surface_mesh = mesh->generate_surface_triangle_mesh(surface_idx);

			Vector3 rpos, rnorm;
			if (surface_mesh->intersect_ray(xform_pos, xform_ray, rpos, rnorm)) {
				Vector3 hitpos = gt.xform(rpos);

				const real_t dist = world_pos.distance_to(hitpos);

				if (dist < 0) {
					continue;
				}

				if (dist < closest_dist) {
					closest_surface = surface_idx;
					closest_dist = dist;
				}
			}
		}

		if (closest_surface == -1) {
			return false;
		}

		spatial_editor->set_preview_material_surface(closest_surface);
		spatial_editor->set_preview_reset_material(mesh_instance->get_surface_override_material(closest_surface));
		mesh_instance->set_surface_override_material(closest_surface, spatial_editor->get_preview_material());

		return true;
	}

	GeometryInstance3D *geometry_instance = Object::cast_to<GeometryInstance3D>(target_inst);
	if (geometry_instance) {
		spatial_editor->set_preview_material_surface(-1);
		spatial_editor->set_preview_reset_material(geometry_instance->get_material_override());
		geometry_instance->set_material_override(spatial_editor->get_preview_material());
		return true;
	}

	return false;
}

void Node3DEditorViewport::_reset_preview_material() const {
	ObjectID last_target = spatial_editor->get_preview_material_target();
	if (last_target.is_null()) {
		return;
	}
	Object *last_target_inst = ObjectDB::get_instance(last_target);

	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(last_target_inst);
	GeometryInstance3D *geometry_instance = Object::cast_to<GeometryInstance3D>(last_target_inst);
	if (mesh_instance && spatial_editor->get_preview_material_surface() != -1) {
		mesh_instance->set_surface_override_material(spatial_editor->get_preview_material_surface(), spatial_editor->get_preview_reset_material());
	} else if (geometry_instance) {
		geometry_instance->set_material_override(spatial_editor->get_preview_reset_material());
	}
}

void Node3DEditorViewport::_remove_preview_material() {
	tooltip_panel->hide();

	spatial_editor->set_preview_material(Ref<Material>());
	spatial_editor->set_preview_reset_material(Ref<Material>());
	spatial_editor->set_preview_material_target(ObjectID());
	spatial_editor->set_preview_material_surface(-1);
}

bool Node3DEditorViewport::_cyclical_dependency_exists(const String &p_target_scene_path, Node *p_desired_node) const {
	if (p_desired_node->get_scene_file_path() == p_target_scene_path) {
		return true;
	}

	int childCount = p_desired_node->get_child_count();
	for (int i = 0; i < childCount; i++) {
		Node *child = p_desired_node->get_child(i);
		if (_cyclical_dependency_exists(p_target_scene_path, child)) {
			return true;
		}
	}
	return false;
}

bool Node3DEditorViewport::_create_instance(Node *p_parent, const String &p_path, const Point2 &p_point) {
	Ref<Resource> res = ResourceLoader::load(p_path);
	ERR_FAIL_COND_V(res.is_null(), false);

	Ref<PackedScene> scene = res;
	Ref<Mesh> mesh = res;

	Node *instantiated_scene = nullptr;

	if (mesh.is_valid() || scene.is_valid()) {
		if (mesh.is_valid()) {
			MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
			mesh_instance->set_mesh(mesh);

			// Adjust casing according to project setting. The file name is expected to be in snake_case, but will work for others.
			const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
			if (!node_name.is_empty()) {
				mesh_instance->set_name(node_name);
			}

			instantiated_scene = mesh_instance;
		} else {
			if (scene.is_null()) { // invalid scene
				return false;
			} else {
				instantiated_scene = scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
			}
		}
	}

	if (instantiated_scene == nullptr) {
		return false;
	}

	if (!get_edited_scene()->get_scene_file_path().is_empty()) { // Cyclic instantiation.
		if (_cyclical_dependency_exists(get_edited_scene()->get_scene_file_path(), instantiated_scene)) {
			memdelete(instantiated_scene);
			return false;
		}
	}

	if (scene.is_valid()) {
		instantiated_scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_path));
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->add_do_method(p_parent, "add_child", instantiated_scene, true);
	undo_redo->add_do_method(instantiated_scene, "set_owner", get_edited_scene());
	undo_redo->add_do_reference(instantiated_scene);
	undo_redo->add_undo_method(p_parent, "remove_child", instantiated_scene);
	undo_redo->add_do_method(editor_selection, "add_node", instantiated_scene);

	String new_name = p_parent->validate_child_name(instantiated_scene);
	EditorDebuggerNode *ed = EditorDebuggerNode::get_singleton();
	undo_redo->add_do_method(ed, "live_debug_instantiate_node", get_edited_scene()->get_path_to(p_parent), p_path, new_name);
	undo_redo->add_undo_method(ed, "live_debug_remove_node", NodePath(String(get_edited_scene()->get_path_to(p_parent)) + "/" + new_name));

	Node3D *node3d = Object::cast_to<Node3D>(instantiated_scene);
	if (node3d) {
		Transform3D parent_tf;
		Node3D *parent_node3d = Object::cast_to<Node3D>(p_parent);
		if (parent_node3d) {
			parent_tf = parent_node3d->get_global_gizmo_transform();
		}

		Transform3D new_tf = node3d->get_transform();
		if (node3d->is_set_as_top_level()) {
			new_tf.origin += preview_node_pos;
		} else {
			new_tf.origin = parent_tf.affine_inverse().xform(preview_node_pos + node3d->get_position());
			new_tf.basis = parent_tf.affine_inverse().basis * new_tf.basis;
		}

		undo_redo->add_do_method(instantiated_scene, "set_transform", new_tf);
	}

	return true;
}

bool Node3DEditorViewport::_create_audio_node(Node *p_parent, const String &p_path, const Point2 &p_point) {
	Ref<AudioStream> audio = ResourceLoader::load(p_path);
	ERR_FAIL_COND_V(audio.is_null(), false);

	AudioStreamPlayer3D *audio_player = memnew(AudioStreamPlayer3D);
	audio_player->set_stream(audio);

	// Adjust casing according to project setting. The file name is expected to be in snake_case, but will work for others.
	const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
	if (!node_name.is_empty()) {
		audio_player->set_name(node_name);
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->add_do_method(p_parent, "add_child", audio_player, true);
	undo_redo->add_do_method(audio_player, "set_owner", get_edited_scene());
	undo_redo->add_do_reference(audio_player);
	undo_redo->add_undo_method(p_parent, "remove_child", audio_player);
	undo_redo->add_do_method(editor_selection, "add_node", audio_player);

	const String new_name = p_parent->validate_child_name(audio_player);
	EditorDebuggerNode *ed = EditorDebuggerNode::get_singleton();
	undo_redo->add_do_method(ed, "live_debug_create_node", get_edited_scene()->get_path_to(p_parent), audio_player->get_class(), new_name);
	undo_redo->add_undo_method(ed, "live_debug_remove_node", NodePath(String(get_edited_scene()->get_path_to(p_parent)) + "/" + new_name));

	Transform3D parent_tf;
	Node3D *parent_node3d = Object::cast_to<Node3D>(p_parent);
	if (parent_node3d) {
		parent_tf = parent_node3d->get_global_gizmo_transform();
	}

	Transform3D new_tf = audio_player->get_transform();
	new_tf.origin = parent_tf.affine_inverse().xform(preview_node_pos + audio_player->get_position());
	new_tf.basis = parent_tf.affine_inverse().basis * new_tf.basis;

	undo_redo->add_do_method(audio_player, "set_transform", new_tf);

	return true;
}

void Node3DEditorViewport::_perform_drop_data() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (spatial_editor->get_preview_material_target().is_valid()) {
		GeometryInstance3D *geometry_instance = ObjectDB::get_instance<GeometryInstance3D>(spatial_editor->get_preview_material_target());
		MeshInstance3D *mesh_instance = ObjectDB::get_instance<MeshInstance3D>(spatial_editor->get_preview_material_target());
		if (mesh_instance && spatial_editor->get_preview_material_surface() != -1) {
			undo_redo->create_action(vformat(TTR("Set Surface %d Override Material"), spatial_editor->get_preview_material_surface()));
			undo_redo->add_do_method(geometry_instance, "set_surface_override_material", spatial_editor->get_preview_material_surface(), spatial_editor->get_preview_material());
			undo_redo->add_undo_method(geometry_instance, "set_surface_override_material", spatial_editor->get_preview_material_surface(), spatial_editor->get_preview_reset_material());
			undo_redo->commit_action();
		} else if (geometry_instance) {
			undo_redo->create_action(TTR("Set Material Override"));
			undo_redo->add_do_method(geometry_instance, "set_material_override", spatial_editor->get_preview_material());
			undo_redo->add_undo_method(geometry_instance, "set_material_override", spatial_editor->get_preview_reset_material());
			undo_redo->commit_action();
		}

		_remove_preview_material();
		return;
	}

	_remove_preview_node();

	PackedStringArray error_files;

	undo_redo->create_action(TTR("Create Node"), UndoRedo::MERGE_DISABLE, target_node);
	undo_redo->add_do_method(editor_selection, "clear");

	for (int i = 0; i < selected_files.size(); i++) {
		String path = selected_files[i];
		Ref<Resource> res = ResourceLoader::load(path);
		if (res.is_null()) {
			continue;
		}

		Ref<PackedScene> scene = res;
		Ref<Mesh> mesh = res;
		if (mesh.is_valid() || scene.is_valid()) {
			if (!_create_instance(target_node, path, drop_pos)) {
				error_files.push_back(path.get_file());
			}
		}

		Ref<AudioStream> audio = res;
		if (audio.is_valid()) {
			if (!_create_audio_node(target_node, path, drop_pos)) {
				error_files.push_back(path.get_file());
			}
		}
	}

	undo_redo->commit_action();

	if (error_files.size() > 0) {
		accept->set_text(vformat(TTR("Error instantiating scene from %s."), String(", ").join(error_files)));
		accept->popup_centered();
	}
}

void Node3DEditorViewport::_show_tooltip(const String &p_title, const String &p_description) const {
	tooltip_panel->set_text(
			vformat("[font_size=%s][b][color=%s]%s[/color][/b][/font_size]\n%s",
					get_theme_default_font_size() + 2,
					get_theme_color(SNAME("accent_color"), EditorStringName(Editor)).to_html(false),
					p_title, p_description));
	tooltip_panel->show();
}

bool Node3DEditorViewport::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (previewing) {
		return false;
	}

	if (p_point == Vector2(Math::INF, Math::INF)) {
		tooltip_panel->hide();
		return false;
	}
	preview_node_viewport_pos = p_point;

	Dictionary d = p_data;
	if (!d.has("type") || String(d["type"]) != "files") {
		tooltip_panel->hide();
		return false;
	}
	Vector<String> files = d["files"];

	// If we already have a preview material or preview node,
	// We just need to update them.
	if (spatial_editor->get_preview_material().is_valid()) {
		ObjectID new_preview_material_target = _select_ray(p_point);
		return _apply_preview_material(new_preview_material_target, p_point);
	}

	if (preview_node->is_inside_tree()) {
		preview_node_viewport_pos = p_point;
		update_preview_node = true;
		return true;
	}

	// If we don't already have a preview material or preview node,
	// it means that this is the first time we are visiting this function.
	// In that case, we need to check that the file(s) are droppable.
	bool is_cyclical_dep = false;
	String error_file;

	enum {
		SCENE = 1 << 0,
		TEXTURE = 1 << 1,
		AUDIO = 1 << 2,
		MESH = 1 << 3,
		MATERIAL = 1 << 4,
	};
	int instantiate_type = 0;

	// Track whether a type other than PackedScene is valid to stop checking them and only
	// continue to check if the rest of the scenes are valid (don't have cyclic dependencies).
	bool is_other_valid = false;
	// Check if at least one of the dragged files is a mesh, material, texture, or scene.
	for (int i = 0; i < files.size(); i++) {
		const String &res_type = ResourceLoader::get_resource_type(files[i]);
		bool is_scene = ClassDB::is_parent_class(res_type, "PackedScene");
		bool is_mesh = ClassDB::is_parent_class(res_type, "Mesh");
		bool is_material = ClassDB::is_parent_class(res_type, "Material");
		bool is_texture = ClassDB::is_parent_class(res_type, "Texture");
		bool is_audio = ClassDB::is_parent_class(res_type, "AudioStream");

		if (is_mesh || is_scene || is_material || is_texture || is_audio) {
			Ref<Resource> res = ResourceLoader::load(files[i]);
			if (res.is_null()) {
				continue;
			}
			Ref<PackedScene> scn = res;
			Ref<Mesh> mesh = res;
			Ref<Material> mat = res;
			Ref<Texture2D> tex = res;
			Ref<AudioStream> audio = res;
			if (scn.is_valid()) {
				Node *instantiated_scene = scn->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
				if (!instantiated_scene) {
					continue;
				}
				Node *edited_scene = get_edited_scene();
				if (edited_scene && !edited_scene->get_scene_file_path().is_empty() && _cyclical_dependency_exists(edited_scene->get_scene_file_path(), instantiated_scene)) {
					memdelete(instantiated_scene);
					is_cyclical_dep = true;
					error_file = files[i].get_file();
					break;
				}
				memdelete(instantiated_scene);
				instantiate_type |= SCENE;
			} else if (!is_other_valid && mat.is_valid()) {
				Ref<BaseMaterial3D> base_mat = res;
				Ref<ShaderMaterial> shader_mat = res;

				if (base_mat.is_null() && shader_mat.is_null()) {
					continue;
				}

				spatial_editor->set_preview_material(mat);
				is_other_valid = true;
				instantiate_type |= MATERIAL;
				continue;
			} else if (!is_other_valid && mesh.is_valid()) {
				// Let the mesh pass.
				is_other_valid = true;
				instantiate_type |= MESH;
			} else if (!is_other_valid && tex.is_valid()) {
				Ref<StandardMaterial3D> new_mat;
				new_mat.instantiate();
				new_mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, tex);

				spatial_editor->set_preview_material(new_mat);
				is_other_valid = true;
				instantiate_type |= TEXTURE;
				continue;
			} else if (!is_other_valid && audio.is_valid()) {
				is_other_valid = true;
				instantiate_type |= AUDIO;
			} else {
				continue;
			}
		}
	}

	String title = TTRN("Can't drop the file...", "Can't drop the files...", files.size());
	if (is_cyclical_dep) {
		_show_tooltip(title, vformat(TTR("Circular dependency found at %s."), error_file));
		return false;
	}

	if (instantiate_type == 0) {
		_show_tooltip(title, TTR("File format is not supported."));
		return false;
	}

	// Only droppable file(s), on first frame, will make it to this point.
	// Hence it is a good place to create the previews and tooltips.
	_create_preview_node(files);
	preview_node->hide();

	String desc = "[ul]" +
			TTRN("[b]Default:[/b] Add as sibling of selected node (except when root is selected).",
					"[b]Default:[/b] Add as siblings of selected node (except when root is selected).",
					files.size()) +
			"\n" +
			TTRN("[b]Hold Shift:[/b] Add as child of selected node.",
					"[b]Hold Shift:[/b] Add as children of selected node.",
					files.size()) +
			"\n" +
			TTRN("[b]Hold Alt:[/b] Add as child of root node.",
					"[b]Hold Alt:[/b] Add as children of root node.",
					files.size());

	if (files.size() > 1) {
		title = TTR("Dropping multiple files...");
	} else if (instantiate_type & SCENE) {
		title = TTR("Dropping a Scene file...");
	} else if (instantiate_type & MESH) {
		title = TTR("Dropping a Mesh file...");
	} else if (instantiate_type & AUDIO) {
		title = TTR("Dropping an Audio file...");
	} else if (instantiate_type & MATERIAL || instantiate_type & TEXTURE) {
		title = TTR("Dropping a Material...");
		desc = "[ul]";
		desc += vformat(TTR("[b]Default:[/b] Place in Geometry's Material Override slot.") +
						"\n" + TTR("[b]Hold %s:[/b] Place in Mesh's Surface Material Override slot."),
				keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL));
	}
	desc += "[/ul]";

	_show_tooltip(title, desc);

	return true;
}

void Node3DEditorViewport::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (!can_drop_data_fw(p_point, p_data, p_from)) {
		return;
	}

	bool is_shift = Input::get_singleton()->is_key_pressed(Key::SHIFT);
	bool is_alt = Input::get_singleton()->is_key_pressed(Key::ALT);

	selected_files.clear();
	Dictionary d = p_data;
	if (d.has("type") && String(d["type"]) == "files") {
		selected_files = d["files"];
	}

	const List<Node *> &selected_nodes = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	Node *root_node = get_edited_scene();
	if (selected_nodes.size() > 0) {
		Node *selected_node = selected_nodes.front()->get();
		if (is_alt) {
			target_node = root_node;
		} else if (is_shift) {
			target_node = selected_node;
		} else { // Default behavior.
			target_node = (selected_node != root_node) ? selected_node->get_parent() : root_node;
		}
	} else {
		if (root_node) {
			target_node = root_node;
		} else {
			// Create a root node so we can add child nodes to it.
			SceneTreeDock::get_singleton()->add_root_node(memnew(Node3D));
			target_node = get_edited_scene();
		}
	}

	drop_pos = p_point;

	_perform_drop_data();
}

void Node3DEditorViewport::begin_transform(TransformMode p_mode, bool instant) {
	if (previewing) {
		return;
	}

	if (get_selected_count() > 0) {
		_edit.children_original_globals.clear();

		_edit.mode = p_mode;
		_compute_edit(_edit.mouse_pos);
		_edit.instant = instant;
		_edit.initial_click_vector = Vector3();
		_edit.previous_rotation_vector = Vector3();
		_edit.accumulated_rotation_angle = 0.0;
		_edit.rotation_angle = 0.0;
		_edit.gizmo_initiated = false;
		switch (p_mode) {
			case TRANSFORM_ROTATE:
				_edit.show_rotation_line = true;
				set_message(vformat(TTR("Rotating %s degrees."), String::num(0, 0)));
				break;
			case TRANSFORM_TRANSLATE:
				set_message(vformat(TTR("Translating: %s"), vformat("%.0v", Vector3())));
				break;
			case TRANSFORM_SCALE:
				set_message(vformat(TTR("Scaling: %s"), vformat("%.0v", Vector3())));
				break;
			default:
				break;
		}
		update_transform_gizmo_view();
		set_process_input(instant);
		surface->queue_redraw();
	}
}

// Apply the current transform operation.
void Node3DEditorViewport::commit_transform() {
	ERR_FAIL_COND(_edit.mode == TRANSFORM_NONE);
	static const char *_transform_name[4] = {
		TTRC("None"),
		TTRC("Rotate"),
		// TRANSLATORS: This refers to the movement that changes the position of an object.
		TTRC("Translate"),
		TTRC("Scale"),
	};
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(_transform_name[_edit.mode]);

	const List<Node *> &selection = editor_selection->get_top_selected_node_list();

	for (Node *E : selection) {
		Node3D *sp = Object::cast_to<Node3D>(E);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		undo_redo->add_do_method(sp, "set_transform", sp->get_local_gizmo_transform());
		undo_redo->add_undo_method(sp, "set_transform", se->original_local);
	}

	if (!_edit.children_original_globals.is_empty()) {
		for (const KeyValue<Node3D *, Transform3D> &pair : _edit.children_original_globals) {
			Node3D *child = pair.key;
			Transform3D original_global = pair.value;
			Transform3D current_global = child->get_global_transform();

			undo_redo->add_do_method(child, "set_global_transform", current_global);
			undo_redo->add_undo_method(child, "set_global_transform", original_global);
		}
	}

	undo_redo->commit_action();

	collision_reposition = false;
	finish_transform();
	_reset_follow_mode_count();
	set_message("");
}

void Node3DEditorViewport::apply_transform(Vector3 p_motion, double p_snap) {
	// View-plane translate/scale always uses global coords; rotation and axis operations respect local/global preference.
	bool local_coords = spatial_editor->are_local_coords_enabled() &&
			!(_edit.plane == TRANSFORM_VIEW && _edit.mode != TRANSFORM_ROTATE) &&
			!_edit.is_trackball;

	bool is_global_view_plane = (_edit.plane == TRANSFORM_VIEW) &&
			((_edit.mode != TRANSFORM_ROTATE) || !spatial_editor->are_local_coords_enabled());

	const List<Node *> &selection = editor_selection->get_top_selected_node_list();
	for (Node *E : selection) {
		Node3D *sp = Object::cast_to<Node3D>(E);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		if (sp->has_meta("_edit_lock_") && !spatial_editor->is_gizmo_visible()) {
			continue;
		}

		if (se->gizmo.is_valid()) {
			for (KeyValue<int, Transform3D> &GE : se->subgizmos) {
				Transform3D xform = GE.value;
				Transform3D new_xform = _compute_transform(_edit.mode, se->original * xform, xform, p_motion, p_snap, local_coords, _edit.plane != TRANSFORM_VIEW, is_global_view_plane); // Force orthogonal with subgizmo.
				if (!local_coords) {
					new_xform = se->original.affine_inverse() * new_xform;
				}
				se->gizmo->set_subgizmo_transform(GE.key, new_xform);
			}
		} else {
			Transform3D new_xform = _compute_transform(_edit.mode, se->original, se->original_local, p_motion, p_snap, local_coords, sp->get_rotation_edit_mode() != Node3D::ROTATION_EDIT_MODE_BASIS && _edit.plane != TRANSFORM_VIEW, is_global_view_plane);
			_transform_gizmo_apply(se->sp, new_xform, local_coords);
		}
	}

	spatial_editor->update_transform_gizmo();
	surface->queue_redraw();
}

// Update the current transform operation in response to an input.
void Node3DEditorViewport::update_transform(bool p_shift) {
	Vector3 ray_pos = get_ray_pos(_edit.mouse_pos);
	Vector3 ray = get_ray(_edit.mouse_pos);
	double snap = EDITOR_GET("interface/inspector/default_float_step");
	int snap_step_decimals = Math::range_step_decimals(snap);

	// View-plane translate/scale always uses global coords; rotation and axis operations respect local/global preference.
	bool local_coords = spatial_editor->are_local_coords_enabled() &&
			!(_edit.plane == TRANSFORM_VIEW && _edit.mode != TRANSFORM_ROTATE);

	switch (_edit.mode) {
		case TRANSFORM_SCALE: {
			Vector3 motion_mask;
			Plane plane;
			bool plane_mv = false;

			switch (_edit.plane) {
				case TRANSFORM_VIEW:
					motion_mask = Vector3(0, 0, 0);
					plane = Plane(_get_camera_normal(), _edit.center);
					break;
				case TRANSFORM_X_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(0).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_Y_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(1).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_Z_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(2).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_YZ:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(2).normalized() + spatial_editor->get_gizmo_transform().basis.get_column(1).normalized();
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(0).normalized(), _edit.center);
					plane_mv = true;
					break;
				case TRANSFORM_XZ:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(2).normalized() + spatial_editor->get_gizmo_transform().basis.get_column(0).normalized();
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(1).normalized(), _edit.center);
					plane_mv = true;
					break;
				case TRANSFORM_XY:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(0).normalized() + spatial_editor->get_gizmo_transform().basis.get_column(1).normalized();
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(2).normalized(), _edit.center);
					plane_mv = true;
					break;
			}

			Vector3 intersection;
			if (!plane.intersects_ray(ray_pos, ray, &intersection)) {
				break;
			}

			Vector3 click;
			if (!plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click)) {
				break;
			}

			Vector3 motion = intersection - click;
			if (_edit.plane != TRANSFORM_VIEW) {
				if (!plane_mv) {
					motion = motion_mask.dot(motion) * motion_mask;

				} else {
					// Alternative planar scaling mode
					if (p_shift) {
						motion = motion_mask.dot(motion) * motion_mask;
					}
				}

			} else {
				const real_t center_click_dist = click.distance_to(_edit.center);
				const real_t center_inters_dist = intersection.distance_to(_edit.center);
				if (center_click_dist == 0) {
					break;
				}

				const real_t scale = center_inters_dist - center_click_dist;
				motion = Vector3(scale, scale, scale);
			}

			motion /= click.distance_to(_edit.center);

			if (spatial_editor->is_snap_enabled()) {
				snap = spatial_editor->get_scale_snap() / 100;
			}
			Vector3 motion_snapped = motion;
			motion_snapped.snapf(snap);
			// This might not be necessary anymore after issue #288 is solved (in 4.0?).
			// TRANSLATORS: Refers to changing the scale of a node in the 3D editor.
			set_message(vformat(TTR("Scaling: %s"), vformat("%.*v", snap_step_decimals, motion_snapped)));
			if (local_coords) {
				// TODO: needed?
				motion = _edit.original.basis.inverse().xform(motion);
			}

			apply_transform(motion, snap);
		} break;

		case TRANSFORM_TRANSLATE: {
			Vector3 motion_mask;
			Plane plane;
			bool plane_mv = false;

			switch (_edit.plane) {
				case TRANSFORM_VIEW:
					plane = Plane(_get_camera_normal(), _edit.center);
					break;
				case TRANSFORM_X_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(0).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_Y_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(1).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_Z_AXIS:
					motion_mask = spatial_editor->get_gizmo_transform().basis.get_column(2).normalized();
					plane = Plane(motion_mask.cross(motion_mask.cross(_get_camera_normal())).normalized(), _edit.center);
					break;
				case TRANSFORM_YZ:
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(0).normalized(), _edit.center);
					plane_mv = true;
					break;
				case TRANSFORM_XZ:
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(1).normalized(), _edit.center);
					plane_mv = true;
					break;
				case TRANSFORM_XY:
					plane = Plane(spatial_editor->get_gizmo_transform().basis.get_column(2).normalized(), _edit.center);
					plane_mv = true;
					break;
			}

			Vector3 intersection;
			if (!plane.intersects_ray(ray_pos, ray, &intersection)) {
				break;
			}

			Vector3 click;
			if (!plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click)) {
				break;
			}

			Vector3 motion = intersection - click;
			if (_edit.plane != TRANSFORM_VIEW) {
				if (!plane_mv) {
					motion = motion_mask.dot(motion) * motion_mask;
				}
			}

			if (spatial_editor->is_snap_enabled()) {
				snap = spatial_editor->get_translate_snap();
			}
			Vector3 motion_snapped = motion;
			motion_snapped.snapf(snap);
			// TRANSLATORS: Refers to changing the position of a node in the 3D editor.
			set_message(vformat(TTR("Translating: %s"), vformat("%.*v", snap_step_decimals, motion_snapped)));
			if (local_coords) {
				motion = spatial_editor->get_gizmo_transform().basis.inverse().xform(motion);
			}

			apply_transform(motion, snap);
		} break;

		case TRANSFORM_ROTATE: {
			Plane plane;
			if (camera->get_projection() == Camera3D::PROJECTION_PERSPECTIVE) {
				Vector3 cam_to_obj = _edit.center - _get_camera_position();
				if (!cam_to_obj.is_zero_approx()) {
					plane = Plane(cam_to_obj.normalized(), _edit.center);
				} else {
					plane = Plane(_get_camera_normal(), _edit.center);
				}
			} else {
				plane = Plane(_get_camera_normal(), _edit.center);
			}

			if (_edit.is_trackball) {
				Vector2 motion_delta = _edit.mouse_pos - _edit.original_mouse_pos;
				real_t sensitivity = TRACKBALL_SENSITIVITY * EDSCALE;
				Vector2 rotation_input = motion_delta * sensitivity;

				Transform3D cam_transform = view_3d_controller->to_camera_transform();
				Vector3 cam_right = cam_transform.basis.get_column(0).normalized();
				Vector3 cam_up = cam_transform.basis.get_column(1).normalized();
				Vector3 rotation_axis = cam_up * rotation_input.x + cam_right * rotation_input.y;

				real_t rotation_angle = rotation_axis.length();
				if (rotation_angle > 0.0f) {
					rotation_axis /= rotation_angle;

					if (spatial_editor->is_snap_enabled()) {
						double snap_step = spatial_editor->get_rotate_snap();
						double angle_deg = Math::rad_to_deg(rotation_angle);
						angle_deg = Math::snapped(angle_deg, snap_step);
						rotation_angle = Math::deg_to_rad(angle_deg);
					}

					double angle_deg = Math::rad_to_deg(rotation_angle);
					set_message(vformat(TTR("Rotating %s degrees."), String::num(angle_deg, 2)));

					apply_transform(rotation_axis, rotation_angle);
				}
				break;
			}

			Vector3 local_axis;
			Vector3 global_axis;
			switch (_edit.plane) {
				case TRANSFORM_VIEW:
					local_axis = _edit.view_axis_local;
					global_axis = _get_camera_normal();
					break;
				case TRANSFORM_X_AXIS:
					local_axis = Vector3(1, 0, 0);
					break;
				case TRANSFORM_Y_AXIS:
					local_axis = Vector3(0, 1, 0);
					break;
				case TRANSFORM_Z_AXIS:
					local_axis = Vector3(0, 0, 1);
					break;
				case TRANSFORM_YZ:
				case TRANSFORM_XZ:
				case TRANSFORM_XY:
					break;
			}

			if (_edit.plane != TRANSFORM_VIEW) {
				global_axis = spatial_editor->get_gizmo_transform().basis.xform(local_axis).normalized();
			}

			Vector3 intersection;
			if (!plane.intersects_ray(ray_pos, ray, &intersection)) {
				break;
			}

			Vector3 click;
			if (!plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click)) {
				break;
			}

			Vector3 current_rotation_vector = (intersection - _edit.center).normalized();

			if (_edit.initial_click_vector == Vector3()) {
				Plane rotation_plane(global_axis, _edit.center);
				Vector3 click_on_rotation_plane;
				if (rotation_plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click_on_rotation_plane)) {
					_edit.initial_click_vector = (click_on_rotation_plane - _edit.center).normalized();
				} else {
					_edit.initial_click_vector = (click - _edit.center).normalized();
				}
				_edit.previous_rotation_vector = current_rotation_vector;
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
			}

			static const float orthogonal_threshold = Math::cos(Math::deg_to_rad(85.0f));
			bool axis_is_orthogonal = Math::abs(plane.normal.dot(global_axis)) < orthogonal_threshold;

			if (_edit.previous_rotation_vector != Vector3()) {
				double delta_angle = _edit.previous_rotation_vector.signed_angle_to(current_rotation_vector, global_axis);
				_edit.accumulated_rotation_angle += delta_angle;
			}
			_edit.previous_rotation_vector = current_rotation_vector;

			if (spatial_editor->is_snap_enabled()) {
				snap = spatial_editor->get_rotate_snap();
				snap_step_decimals = Math::range_step_decimals(snap);
			}

			if (axis_is_orthogonal) {
				_edit.show_rotation_line = false;
				Vector3 projection_axis = plane.normal.cross(global_axis);
				Vector3 delta = intersection - click;
				float projection = delta.dot(projection_axis);
				double orth_angle = (projection * (Math::PI / 2.0f)) / (gizmo_scale * GIZMO_CIRCLE_SIZE);
				_edit.rotation_angle = spatial_editor->is_snap_enabled()
						? Math::deg_to_rad(Math::snapped(Math::rad_to_deg(orth_angle), snap))
						: orth_angle;
			} else {
				_edit.show_rotation_line = true;
				_edit.rotation_angle = spatial_editor->is_snap_enabled()
						? Math::deg_to_rad(Math::snapped(Math::rad_to_deg(_edit.accumulated_rotation_angle), snap))
						: _edit.accumulated_rotation_angle;
			}
			set_message(vformat(TTR("Rotating %s degrees."), String::num(Math::rad_to_deg(_edit.rotation_angle), snap_step_decimals)));

			Vector3 compute_axis = local_coords ? local_axis : global_axis;
			apply_transform(compute_axis, _edit.rotation_angle);
		} break;
		default: {
		}
	}
}

void Node3DEditorViewport::update_transform_numeric() {
	Vector3 motion;
	switch (_edit.plane) {
		case TRANSFORM_VIEW: {
			switch (_edit.mode) {
				case TRANSFORM_TRANSLATE:
					motion = Vector3(1, 0, 0);
					break;
				case TRANSFORM_ROTATE:
					motion = _edit.view_axis_local;
					break;
				case TRANSFORM_SCALE:
					motion = Vector3(1, 1, 1);
					break;
				case TRANSFORM_NONE:
					ERR_FAIL_MSG("_edit.mode cannot be TRANSFORM_NONE in update_transform_numeric.");
			}
			break;
		}
		case TRANSFORM_X_AXIS:
			motion = Vector3(1, 0, 0);
			break;
		case TRANSFORM_Y_AXIS:
			motion = Vector3(0, 1, 0);
			break;
		case TRANSFORM_Z_AXIS:
			motion = Vector3(0, 0, 1);
			break;
		case TRANSFORM_XY:
			motion = Vector3(1, 1, 0);
			break;
		case TRANSFORM_XZ:
			motion = Vector3(1, 0, 1);
			break;
		case TRANSFORM_YZ:
			motion = Vector3(0, 1, 1);
			break;
	}

	double value = _edit.numeric_input * (_edit.numeric_negate ? -1 : 1);
	double extra = 0.0;
	switch (_edit.mode) {
		case TRANSFORM_TRANSLATE:
			motion *= value;
			set_message(vformat(TTR("Translating %s."), motion));
			break;
		case TRANSFORM_ROTATE:
			extra = Math::deg_to_rad(value);
			set_message(vformat(TTR("Rotating %f degrees."), value));
			break;
		case TRANSFORM_SCALE:
			// To halve the size of an object in Blender, you scale it by 0.5.
			// Doing the same in Godot is considered scaling it by -0.5.
			motion *= (value - 1.0);
			set_message(vformat(TTR("Scaling %s."), motion));
			break;
		case TRANSFORM_NONE:
			ERR_FAIL_MSG("_edit.mode cannot be TRANSFORM_NONE in update_transform_numeric.");
	}

	apply_transform(motion, extra);
}

// Perform cleanup after a transform operation is committed or canceled.
void Node3DEditorViewport::finish_transform() {
	_edit.mode = TRANSFORM_NONE;
	_edit.instant = false;
	_edit.numeric_input = 0;
	_edit.numeric_next_decimal = 0;
	_edit.numeric_negate = false;
	_edit.is_trackball = false;
	_edit.initial_click_vector = Vector3();
	_edit.previous_rotation_vector = Vector3();
	_edit.accumulated_rotation_angle = 0.0;
	_edit.rotation_angle = 0.0;
	_edit.gizmo_initiated = false;
	_edit.children_original_globals.clear();
	spatial_editor->set_local_coords_enabled(_edit.original_local);
	spatial_editor->update_transform_gizmo();
	surface->queue_redraw();
	set_process_input(false);
	clicked = ObjectID();
}

// Register a shortcut and also add it as an input action with the same events.
void Node3DEditorViewport::register_shortcut_action(const String &p_path, const String &p_name, Key p_keycode, bool p_physical) {
	Ref<Shortcut> sc = ED_SHORTCUT(p_path, p_name, p_keycode, p_physical);
	shortcut_changed_callback(sc, p_path);
	// Connect to the change event on the shortcut so the input binding can be updated.
	sc->connect_changed(callable_mp(this, &Node3DEditorViewport::shortcut_changed_callback).bind(sc, p_path));
}

// Update the action in the InputMap to the provided shortcut events.
void Node3DEditorViewport::shortcut_changed_callback(const Ref<Shortcut> p_shortcut, const String &p_shortcut_path) {
	InputMap *im = InputMap::get_singleton();
	if (im->has_action(p_shortcut_path)) {
		im->action_erase_events(p_shortcut_path);
	} else {
		im->add_action(p_shortcut_path);
	}

	for (int i = 0; i < p_shortcut->get_events().size(); i++) {
		im->action_add_event(p_shortcut_path, p_shortcut->get_events()[i]);
	}

	if (view_3d_controller.is_valid()) {
		_update_view_3d_controller();
	}
}

void Node3DEditorViewport::_set_lock_view_rotation(bool p_lock_rotation) {
	view_3d_controller->set_lock_rotation(p_lock_rotation);
	int idx = view_display_menu->get_popup()->get_item_index(VIEW_LOCK_ROTATION);
	view_display_menu->get_popup()->set_item_checked(idx, p_lock_rotation);
	if (p_lock_rotation) {
		locked_label->show();
	} else {
		locked_label->hide();
	}
}

void Node3DEditorViewport::_add_advanced_debug_draw_mode_item(PopupMenu *p_popup, const String &p_name, int p_value, SupportedRenderingMethods p_rendering_methods, const String &p_tooltip) {
	display_submenu->add_radio_check_item(p_name, p_value);
	Array item_data = { p_rendering_methods, p_tooltip };
	display_submenu->set_item_metadata(-1, item_data); // Tooltip is assigned in NOTIFICATION_TRANSLATION_CHANGED.
}

void Node3DEditorViewport::_load_viewport_inputs() {
	// Registering with Key::NONE intentionally creates an empty Array.
	register_shortcut_action("spatial_editor/viewport_orbit_modifier_1", TTRC("Viewport Orbit Modifier 1"), Key::NONE);
	register_shortcut_action("spatial_editor/viewport_orbit_modifier_2", TTRC("Viewport Orbit Modifier 2"), Key::NONE);
	register_shortcut_action("spatial_editor/viewport_orbit_snap_modifier_1", TTRC("Viewport Orbit Snap Modifier 1"), Key::ALT);
	register_shortcut_action("spatial_editor/viewport_orbit_snap_modifier_2", TTRC("Viewport Orbit Snap Modifier 2"), Key::NONE);
	register_shortcut_action("spatial_editor/viewport_pan_modifier_1", TTRC("Viewport Pan Modifier 1"), Key::SHIFT);
	register_shortcut_action("spatial_editor/viewport_pan_modifier_2", TTRC("Viewport Pan Modifier 2"), Key::NONE);
	register_shortcut_action("spatial_editor/viewport_zoom_modifier_1", TTRC("Viewport Zoom Modifier 1"), Key::CTRL);
	register_shortcut_action("spatial_editor/viewport_zoom_modifier_2", TTRC("Viewport Zoom Modifier 2"), Key::NONE);

	register_shortcut_action("spatial_editor/freelook_left", TTRC("Freelook Left"), Key::A, true);
	register_shortcut_action("spatial_editor/freelook_right", TTRC("Freelook Right"), Key::D, true);
	register_shortcut_action("spatial_editor/freelook_forward", TTRC("Freelook Forward"), Key::W, true);
	register_shortcut_action("spatial_editor/freelook_backwards", TTRC("Freelook Backwards"), Key::S, true);
	register_shortcut_action("spatial_editor/freelook_up", TTRC("Freelook Up"), Key::E, true);
	register_shortcut_action("spatial_editor/freelook_down", TTRC("Freelook Down"), Key::Q, true);
	register_shortcut_action("spatial_editor/freelook_speed_modifier", TTRC("Freelook Speed Modifier"), Key::SHIFT);
	register_shortcut_action("spatial_editor/freelook_slow_modifier", TTRC("Freelook Slow Modifier"), Key::ALT);
}

Node3DEditorViewport::Node3DEditorViewport(Node3DEditor *p_spatial_editor, int p_index) {
	cpu_time_history_index = 0;
	gpu_time_history_index = 0;

	_edit.mode = TRANSFORM_NONE;
	_edit.plane = TRANSFORM_VIEW;
	_edit.show_rotation_line = true;
	_edit.instant = false;
	_edit.gizmo_handle = -1;
	_edit.gizmo_handle_secondary = false;

	index = p_index;
	editor_selection = EditorNode::get_singleton()->get_editor_selection();
	editor_selection->connect("selection_changed", callable_mp(this, &Node3DEditorViewport::_reset_follow_mode_count));

	message_time = 0;
	zoom_indicator_delay = 0.0;

	spatial_editor = p_spatial_editor;
	SubViewportContainer *c = memnew(SubViewportContainer);
	subviewport_container = c;
	c->set_stretch(true);
	add_child(c);
	c->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	viewport = memnew(SubViewport);
	viewport->set_disable_input(true);

	c->add_child(viewport);
	surface = memnew(Control);
	SET_DRAG_FORWARDING_CD(surface, Node3DEditorViewport);
	add_child(surface);
	surface->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	surface->set_clip_contents(true);
	camera = memnew(Camera3D);
	camera->set_disable_gizmos(true);
	// Refined once the view has a world and a layer of its own to go with it.
	camera->set_cull_mask(((1 << 20) - 1) | (1 << GIZMO_BASE_LAYER) | (1 << GIZMO_EDIT_LAYER) | (1 << GIZMO_GRID_LAYER) | (1 << MISC_TOOL_LAYER));
	viewport->add_child(camera);
	camera->make_current();
	surface->set_focus_mode(FOCUS_ALL);

	VBoxContainer *vbox = memnew(VBoxContainer);
	surface->add_child(vbox);
	vbox->set_offset(SIDE_LEFT, 10 * EDSCALE);
	vbox->set_offset(SIDE_TOP, 10 * EDSCALE);

	HBoxContainer *hbox = memnew(HBoxContainer);
	vbox->add_child(hbox);

	view_display_menu = memnew(MenuButton);
	view_display_menu->set_flat(false);
	view_display_menu->set_h_size_flags(0);
	view_display_menu->set_shortcut_context(this);
	view_display_menu->set_accessibility_name(TTRC("View"));
	view_display_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	view_display_menu->get_popup()->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	hbox->add_child(view_display_menu);

	view_display_menu->get_popup()->set_hide_on_checkable_item_selection(false);

	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/top_view"), VIEW_TOP);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/bottom_view"), VIEW_BOTTOM);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/left_view"), VIEW_LEFT);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/right_view"), VIEW_RIGHT);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/front_view"), VIEW_FRONT);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/rear_view"), VIEW_REAR);
	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/switch_perspective_orthogonal"), VIEW_SWITCH_PERSPECTIVE_ORTHOGONAL);
	view_display_menu->get_popup()->add_radio_check_item(TTRC("Perspective"), VIEW_PERSPECTIVE);
	view_display_menu->get_popup()->add_radio_check_item(TTRC("Orthogonal"), VIEW_ORTHOGONAL);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_PERSPECTIVE), true);
	view_display_menu->get_popup()->add_check_item(TTRC("Auto Orthogonal Enabled"), VIEW_AUTO_ORTHOGONAL);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_AUTO_ORTHOGONAL), true);
	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_lock_rotation", TTRC("Lock View Rotation")), VIEW_LOCK_ROTATION);
	view_display_menu->get_popup()->add_separator();
	// TRANSLATORS: "Normal" as in "normal life", not "normal vector".
	view_display_menu->get_popup()->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/view_display_normal", TTRC("Display Normal")), VIEW_DISPLAY_NORMAL);
	view_display_menu->get_popup()->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/view_display_wireframe", TTRC("Display Wireframe")), VIEW_DISPLAY_WIREFRAME);
	view_display_menu->get_popup()->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/view_display_overdraw", TTRC("Display Overdraw")), VIEW_DISPLAY_OVERDRAW);
	view_display_menu->get_popup()->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/view_display_lighting", TTRC("Display Lighting")), VIEW_DISPLAY_LIGHTING);
	view_display_menu->get_popup()->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/view_display_unshaded", TTRC("Display Unshaded")), VIEW_DISPLAY_UNSHADED);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_DISPLAY_NORMAL), true);

	display_submenu = memnew(PopupMenu);
	display_submenu->set_hide_on_checkable_item_selection(false);
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Directional Shadow Splits"), VIEW_DISPLAY_DEBUG_PSSM_SPLITS, SupportedRenderingMethods::FORWARD_PLUS_MOBILE,
			TTRC("Displays directional shadow splits in different colors to make adjusting split thresholds easier. \nRed: 1st split (closest to the camera), Green: 2nd split, Blue: 3rd split, Yellow: 4th split (furthest from the camera)"));
	display_submenu->add_separator();
	// TRANSLATORS: "Normal" as in "normal vector", not "normal life".
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Normal Buffer"), VIEW_DISPLAY_NORMAL_BUFFER, SupportedRenderingMethods::FORWARD_PLUS);
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Shadow Atlas"), VIEW_DISPLAY_DEBUG_SHADOW_ATLAS, SupportedRenderingMethods::ALL,
			TTRC("Displays the shadow atlas used for positional (omni/spot) shadow mapping.\nRequires a visible OmniLight3D or SpotLight3D node with shadows enabled to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Directional Shadow Map"), VIEW_DISPLAY_DEBUG_DIRECTIONAL_SHADOW_ATLAS, SupportedRenderingMethods::ALL,
			TTRC("Displays the shadow map used for directional shadow mapping.\nRequires a visible DirectionalLight3D node with shadows enabled to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Decal Atlas"), VIEW_DISPLAY_DEBUG_DECAL_ATLAS, SupportedRenderingMethods::FORWARD_PLUS_MOBILE);
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("AreaLight3D Atlas"), VIEW_DISPLAY_DEBUG_AREA_LIGHT_ATLAS, SupportedRenderingMethods::FORWARD_PLUS_MOBILE);
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("VoxelGI Lighting"), VIEW_DISPLAY_DEBUG_VOXEL_GI_LIGHTING, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Requires a visible VoxelGI node that has been baked to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("VoxelGI Albedo"), VIEW_DISPLAY_DEBUG_VOXEL_GI_ALBEDO, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Requires a visible VoxelGI node that has been baked to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("VoxelGI Emission"), VIEW_DISPLAY_DEBUG_VOXEL_GI_EMISSION, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Requires a visible VoxelGI node that has been baked to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("SDFGI Cascades"), VIEW_DISPLAY_DEBUG_SDFGI, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Requires SDFGI to be enabled in Environment to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("SDFGI Probes"), VIEW_DISPLAY_DEBUG_SDFGI_PROBES, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Left-click a SDFGI probe to display its occlusion information (white = not occluded, red = fully occluded).\nRequires SDFGI to be enabled in Environment to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Scene Luminance"), VIEW_DISPLAY_DEBUG_SCENE_LUMINANCE, SupportedRenderingMethods::FORWARD_PLUS_MOBILE,
			TTRC("Displays the scene luminance computed from the 3D buffer. This is used for Auto Exposure calculation.\nRequires Auto Exposure to be enabled in CameraAttributes to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("SSAO"), VIEW_DISPLAY_DEBUG_SSAO, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Displays the screen-space ambient occlusion buffer. Requires SSAO to be enabled in Environment to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("SSIL"), VIEW_DISPLAY_DEBUG_SSIL, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Displays the screen-space indirect lighting buffer. Requires SSIL to be enabled in Environment to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("VoxelGI/SDFGI Buffer"), VIEW_DISPLAY_DEBUG_GI_BUFFER, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Requires SDFGI or VoxelGI to be enabled to have a visible effect."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Disable Mesh LOD"), VIEW_DISPLAY_DEBUG_DISABLE_LOD, SupportedRenderingMethods::ALL,
			TTRC("Renders all meshes with their highest level of detail regardless of their distance from the camera."));
	display_submenu->add_separator();
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("OmniLight3D Cluster"), VIEW_DISPLAY_DEBUG_CLUSTER_OMNI_LIGHTS, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Highlights tiles of pixels that are affected by at least one OmniLight3D."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("SpotLight3D Cluster"), VIEW_DISPLAY_DEBUG_CLUSTER_SPOT_LIGHTS, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Highlights tiles of pixels that are affected by at least one SpotLight3D."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("AreaLight3D Cluster"), VIEW_DISPLAY_DEBUG_CLUSTER_AREA_LIGHTS, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Highlights tiles of pixels that are affected by at least one AreaLight3D."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Decal Cluster"), VIEW_DISPLAY_DEBUG_CLUSTER_DECALS, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Highlights tiles of pixels that are affected by at least one Decal."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("ReflectionProbe Cluster"), VIEW_DISPLAY_DEBUG_CLUSTER_REFLECTION_PROBES, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Highlights tiles of pixels that are affected by at least one ReflectionProbe."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Occlusion Culling Buffer"), VIEW_DISPLAY_DEBUG_OCCLUDERS, SupportedRenderingMethods::FORWARD_PLUS_MOBILE,
			TTRC("Represents occluders with black pixels. Requires occlusion culling to be enabled to have a visible effect."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Motion Vectors"), VIEW_DISPLAY_MOTION_VECTORS, SupportedRenderingMethods::FORWARD_PLUS,
			TTRC("Represents motion vectors with colored lines in the direction of motion. Gray dots represent areas with no per-pixel motion."));
	_add_advanced_debug_draw_mode_item(display_submenu, TTRC("Internal Buffer"), VIEW_DISPLAY_INTERNAL_BUFFER, SupportedRenderingMethods::FORWARD_PLUS_MOBILE,
			TTRC("Shows the scene rendered in linear colorspace before any tonemapping or post-processing."));
	view_display_menu->get_popup()->add_submenu_node_item(TTRC("Display Advanced..."), display_submenu, VIEW_DISPLAY_ADVANCED);

	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_environment", TTRC("View Environment")), VIEW_ENVIRONMENT);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_gizmos", TTRC("View Gizmos")), VIEW_GIZMOS);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_transform_gizmo", TTRC("View Transform Gizmo")), VIEW_TRANSFORM_GIZMO);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_grid_lines", TTRC("View Grid")), VIEW_GRID);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_information", TTRC("View Information")), VIEW_INFORMATION);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_fps", TTRC("View Frame Time")), VIEW_FRAME_TIME);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_ENVIRONMENT), true);
	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_half_resolution", TTRC("Half Resolution")), VIEW_HALF_RESOLUTION);
	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_audio_listener", TTRC("Audio Listener")), VIEW_AUDIO_LISTENER);
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_audio_doppler", TTRC("Enable Doppler")), VIEW_AUDIO_DOPPLER);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_GIZMOS), true);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_TRANSFORM_GIZMO), true);
	view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_GRID), true);

	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_cinematic_preview", TTRC("Cinematic Preview")), VIEW_CINEMATIC_PREVIEW);

	view_display_menu->get_popup()->add_separator();
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/focus_origin"), VIEW_CENTER_TO_ORIGIN);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/focus_selection"), VIEW_CENTER_TO_SELECTION);
	view_display_menu->get_popup()->set_item_tooltip(-1, TTR("Press Focus Selection twice to start following the selection as it moves. Press it yet another time to stop following the selection."));
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/align_transform_with_view"), VIEW_ALIGN_TRANSFORM_WITH_VIEW);
	view_display_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("spatial_editor/align_rotation_with_view"), VIEW_ALIGN_ROTATION_WITH_VIEW);
	view_display_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditorViewport::_menu_option));
	display_submenu->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditorViewport::_menu_option));
	view_display_menu->set_disable_shortcuts(true);

	_load_viewport_inputs();
	InputMap::get_singleton()->connect("project_settings_loaded", callable_mp(this, &Node3DEditorViewport::_load_viewport_inputs));

	// Z and ` only mean something here while nothing is being transformed; Z
	// then locks to the Z axis, which is checked first.
	ED_SHORTCUT("spatial_editor/pie_shading", TTRC("Shading Pie Menu"), Key::Z);
	ED_SHORTCUT("spatial_editor/pie_view", TTRC("View Pie Menu"), Key::QUOTELEFT);
	ED_SHORTCUT("spatial_editor/pie_snap", TTRC("Snap Pie Menu"), KeyModifierMask::SHIFT | Key::S);
	ED_SHORTCUT("spatial_editor/lock_transform_x", TTRC("Lock Transformation to X axis"), Key::X);
	ED_SHORTCUT("spatial_editor/lock_transform_y", TTRC("Lock Transformation to Y axis"), Key::Y);
	ED_SHORTCUT("spatial_editor/lock_transform_z", TTRC("Lock Transformation to Z axis"), Key::Z);
	ED_SHORTCUT("spatial_editor/lock_transform_yz", TTRC("Lock Transformation to YZ plane"), KeyModifierMask::SHIFT | Key::X);
	ED_SHORTCUT("spatial_editor/lock_transform_xz", TTRC("Lock Transformation to XZ plane"), KeyModifierMask::SHIFT | Key::Y);
	ED_SHORTCUT("spatial_editor/lock_transform_xy", TTRC("Lock Transformation to XY plane"), KeyModifierMask::SHIFT | Key::Z);
	ED_SHORTCUT("spatial_editor/cancel_transform", TTRC("Cancel Transformation"), Key::ESCAPE);
	ED_SHORTCUT("spatial_editor/instant_translate", TTRC("Begin Translate Transformation"));
	ED_SHORTCUT("spatial_editor/instant_rotate", TTRC("Begin Rotate Transformation"));
	ED_SHORTCUT("spatial_editor/instant_scale", TTRC("Begin Scale Transformation"));
	ED_SHORTCUT("spatial_editor/collision_reposition", TTRC("Reposition Using Collisions"), KeyModifierMask::SHIFT | Key::G);
	ED_SHORTCUT("spatial_editor/reset_transform_position", TTRC("Reset Position"), KeyModifierMask::ALT + Key::W);
	ED_SHORTCUT("spatial_editor/reset_transform_rotation", TTRC("Reset Rotation"), KeyModifierMask::ALT + Key::E);
	ED_SHORTCUT("spatial_editor/reset_transform_scale", TTRC("Reset Scale"), KeyModifierMask::ALT + Key::R);

	translation_preview_button = memnew(EditorTranslationPreviewButton);
	hbox->add_child(translation_preview_button);

	follow_mode = memnew(Button);
	follow_mode->set_tooltip_text(TTR("Click to stop following this node as it moves."));
	follow_mode->hide();
	vbox->add_child(follow_mode);
	follow_mode->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditorViewport::_disable_follow_mode));

	preview_camera = memnew(CheckBox);
	preview_camera->set_text(TTRC("Preview"));
	preview_camera->set_tooltip_text(TTRC("Preview through the selected camera.\nHold Shift while clicking to also enable Pilot mode."));
	// Using Control even on macOS to avoid conflict with Quick Open shortcut.
	preview_camera->set_shortcut(ED_SHORTCUT("spatial_editor/toggle_camera_preview", TTRC("Toggle Camera Preview"), KeyModifierMask::CTRL | Key::P));
	vbox->add_child(preview_camera);
	preview_camera->set_h_size_flags(0);
	preview_camera->set_theme_type_variation("CheckBoxNoIconTint");
	preview_camera->hide();
	preview_camera->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_camera_preview));

	pilot_camera = memnew(CheckBox);
	pilot_camera->set_text(TTRC("Pilot"));
	pilot_camera->set_tooltip_text(TTRC("Enable pilot mode for the preview camera.\nAllows WASD movement and mouse look when in preview mode."));
	pilot_camera->set_shortcut(ED_SHORTCUT("spatial_editor/toggle_pilot_preview", TTRC("Toggle Pilot Mode in Preview")));
	vbox->add_child(pilot_camera);
	pilot_camera->set_h_size_flags(0);
	pilot_camera->set_theme_type_variation("CheckBoxNoIconTint");
	pilot_camera->hide();
	pilot_camera->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditorViewport::_toggle_pilot_preview));
	previewing = nullptr;
	gizmo_scale = 1.0;

	preview_node = nullptr;

	bottom_center_vbox = memnew(VBoxContainer);
	bottom_center_vbox->set_anchors_preset(LayoutPreset::PRESET_CENTER);
	bottom_center_vbox->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -20 * EDSCALE);
	bottom_center_vbox->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -10 * EDSCALE);
	bottom_center_vbox->set_h_grow_direction(GROW_DIRECTION_BOTH);
	bottom_center_vbox->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	surface->add_child(bottom_center_vbox);

	info_panel = memnew(PanelContainer);
	info_panel->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -90 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -90 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -10 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -10 * EDSCALE);
	info_panel->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	info_panel->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	info_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	surface->add_child(info_panel);
	info_panel->hide();

	info_label = memnew(Label);
	info_label->set_focus_mode(FOCUS_ACCESSIBILITY);
	info_panel->add_child(info_label);

	cinema_label = memnew(Label);
	cinema_label->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, 10 * EDSCALE);
	cinema_label->set_h_grow_direction(GROW_DIRECTION_END);
	cinema_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	cinema_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	surface->add_child(cinema_label);
	cinema_label->set_text(TTRC("Cinematic Preview"));
	cinema_label->hide();
	previewing_cinema = false;

	locked_label = memnew(Label);
	locked_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	locked_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	locked_label->set_h_size_flags(SIZE_SHRINK_CENTER);
	bottom_center_vbox->add_child(locked_label);
	locked_label->set_text(TTRC("View Rotation Locked"));
	locked_label->hide();

	zoom_limit_label = memnew(Label);
	zoom_limit_label->set_text(TTRC(U"To zoom further, change the camera's clipping planes (View → Settings...)"));
	zoom_limit_label->set_name("ZoomLimitMessageLabel");
	zoom_limit_label->add_theme_color_override(SceneStringName(font_color), Color(1, 1, 1, 1));
	zoom_limit_label->hide();
	bottom_center_vbox->add_child(zoom_limit_label);

	tooltip_panel = memnew(RichTextLabel);
	vbox->add_child(tooltip_panel);
	tooltip_panel->hide();
	tooltip_panel->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	tooltip_panel->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	tooltip_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	tooltip_panel->set_focus_mode(FOCUS_ACCESSIBILITY);
	tooltip_panel->set_use_bbcode(true);
	tooltip_panel->set_fit_content(true);
	tooltip_panel->set_scroll_active(false);
	tooltip_panel->set_tab_size(1);
	tooltip_panel->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	tooltip_panel->set_anchors_and_offsets_preset(LayoutPreset::PRESET_TOP_LEFT);
	tooltip_panel->add_theme_color_override(SceneStringName(font_color), Color(0.8f, 0.8f, 0.8f, 1));
	tooltip_panel->add_theme_constant_override("paragraph_separation", 5);

	frame_time_gradient = memnew(Gradient);
	// The color is set when the theme changes.
	frame_time_gradient->add_point(0.5, Color());

	top_right_vbox = memnew(VBoxContainer);
	top_right_vbox->add_theme_constant_override("separation", 10.0 * EDSCALE);
	top_right_vbox->set_anchors_and_offsets_preset(PRESET_TOP_RIGHT, PRESET_MODE_MINSIZE, 10.0 * EDSCALE);
	top_right_vbox->set_h_grow_direction(GROW_DIRECTION_BEGIN);

	const int navigation_control_size = 150;

	position_control = memnew(ViewportNavigationControl);
	position_control->set_navigation_mode(View3DController::NAV_MODE_MOVE);
	position_control->set_custom_minimum_size(Size2(navigation_control_size, navigation_control_size) * EDSCALE);
	position_control->set_h_size_flags(SIZE_SHRINK_END);
	position_control->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN, 0);
	position_control->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -navigation_control_size * EDSCALE);
	position_control->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_BEGIN, navigation_control_size * EDSCALE);
	position_control->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, 0);
	position_control->set_viewport(this);
	surface->add_child(position_control);

	look_control = memnew(ViewportNavigationControl);
	look_control->set_navigation_mode(View3DController::NAV_MODE_LOOK);
	look_control->set_custom_minimum_size(Size2(navigation_control_size, navigation_control_size) * EDSCALE);
	look_control->set_h_size_flags(SIZE_SHRINK_END);
	look_control->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -navigation_control_size * EDSCALE);
	look_control->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -navigation_control_size * EDSCALE);
	look_control->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, 0);
	look_control->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, 0);
	look_control->set_viewport(this);
	surface->add_child(look_control);

	rotation_control = memnew(ViewportRotationControl);
	rotation_control->set_custom_minimum_size(Size2(80, 80) * EDSCALE);
	rotation_control->set_h_size_flags(SIZE_SHRINK_END);
	rotation_control->set_viewport(this);
	rotation_control->set_focus_mode(FOCUS_CLICK);
	top_right_vbox->add_child(rotation_control);

	frame_time_panel = memnew(PanelContainer);
	frame_time_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	top_right_vbox->add_child(frame_time_panel);
	frame_time_panel->hide();

	frame_time_vbox = memnew(VBoxContainer);
	frame_time_panel->add_child(frame_time_vbox);

	// Individual Labels are used to allow coloring each label with its own color.
	cpu_time_label = memnew(Label);
	frame_time_vbox->add_child(cpu_time_label);

	gpu_time_label = memnew(Label);
	frame_time_vbox->add_child(gpu_time_label);

	fps_label = memnew(Label);
	frame_time_vbox->add_child(fps_label);

	surface->add_child(top_right_vbox);

	accept = nullptr;

	selection_menu = memnew(PopupMenu);
	add_child(selection_menu);
	selection_menu->set_min_size(Size2(100, 0) * EDSCALE);
	selection_menu->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditorViewport::_selection_result_pressed));
	selection_menu->connect("popup_hide", callable_mp(this, &Node3DEditorViewport::_selection_menu_hide));

	if (p_index == 0) {
		view_display_menu->get_popup()->set_item_checked(view_display_menu->get_popup()->get_item_index(VIEW_AUDIO_LISTENER), true);
		viewport->set_as_audio_listener_3d(true);
	}

	ruler = memnew(Node);

	ruler_start_point = memnew(Node3D);
	ruler_start_point->set_visible(false);

	ruler_end_point = memnew(Node3D);
	ruler_end_point->set_visible(false);

	ruler_material.instantiate();
	ruler_material->set_albedo(Color(1.0, 0.9, 0.0, 1.0));
	ruler_material->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	ruler_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	ruler_material->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);

	ruler_material_xray.instantiate();
	ruler_material_xray->set_albedo(Color(1.0, 0.9, 0.0, 0.15));
	ruler_material_xray->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	ruler_material_xray->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	ruler_material_xray->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	ruler_material_xray->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	ruler_material_xray->set_render_priority(BaseMaterial3D::RENDER_PRIORITY_MAX);

	geometry.instantiate();

	geometry_xray.instantiate();

	ruler_line = memnew(MeshInstance3D);
	ruler_line->set_mesh(geometry);
	ruler_line->set_material_override(ruler_material);

	ruler_line_xray = memnew(MeshInstance3D);
	ruler_line_xray->set_mesh(geometry_xray);
	ruler_line_xray->set_material_override(ruler_material_xray);

	ruler_triangle_material.instantiate();
	ruler_triangle_material->set_albedo(Color(1.0, 1.0, 1.0, 1.0));
	ruler_triangle_material->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	ruler_triangle_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	ruler_triangle_material->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
	ruler_triangle_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

	ruler_triangle_material_xray.instantiate();
	ruler_triangle_material_xray->set_albedo(Color(1.0, 1.0, 1.0, 0.15));
	ruler_triangle_material_xray->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	ruler_triangle_material_xray->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	ruler_triangle_material_xray->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	ruler_triangle_material_xray->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	ruler_triangle_material_xray->set_render_priority(BaseMaterial3D::RENDER_PRIORITY_MAX);
	ruler_triangle_material_xray->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

	ruler_triangle_lines = memnew(MeshInstance3D);
	Ref<ImmediateMesh> triangle_mesh;
	triangle_mesh.instantiate();
	ruler_triangle_lines->set_mesh(triangle_mesh);
	ruler_triangle_lines->set_material_override(ruler_triangle_material);

	ruler_triangle_lines_xray = memnew(MeshInstance3D);
	Ref<ImmediateMesh> triangle_mesh_xray;
	triangle_mesh_xray.instantiate();
	ruler_triangle_lines_xray->set_mesh(triangle_mesh_xray);
	ruler_triangle_lines_xray->set_material_override(ruler_triangle_material_xray);

	ruler_label = memnew(Label);
	ruler_label->set_visible(false);

	ruler_label_x = memnew(Label);
	ruler_label_x->set_visible(false);

	ruler_label_y = memnew(Label);
	ruler_label_y->set_visible(false);

	ruler_label_z = memnew(Label);
	ruler_label_z->set_visible(false);

	ruler->add_child(ruler_start_point);
	ruler->add_child(ruler_end_point);
	ruler->add_child(ruler_line);
	ruler->add_child(ruler_line_xray);
	ruler->add_child(ruler_triangle_lines);
	ruler->add_child(ruler_triangle_lines_xray);

	viewport->add_child(ruler_label);
	viewport->add_child(ruler_label_x);
	viewport->add_child(ruler_label_y);
	viewport->add_child(ruler_label_z);

	view_3d_controller.instantiate();
	view_3d_controller->connect("view_state_changed", callable_mp(this, &Node3DEditorViewport::_view_state_changed));
	view_3d_controller->connect("fov_scaled", callable_mp((CanvasItem *)surface, &CanvasItem::queue_redraw));
	view_3d_controller->connect("freelook_changed", callable_mp(this, &Node3DEditorViewport::_freelook_changed));
	view_3d_controller->connect("freelook_speed_scaled", callable_mp(this, &Node3DEditorViewport::_freelook_speed_scaled));
	view_3d_controller->connect("cursor_panned", callable_mp(this, &Node3DEditorViewport::_disable_follow_mode));
	view_3d_controller->connect("cursor_interpolated", callable_mp(this, &Node3DEditorViewport::_cursor_interpolated));
	view_3d_controller->connect("cursor_distance_scaled", callable_mp(this, &Node3DEditorViewport::_cursor_distance_scaled));
	_update_view_3d_controller(true);

	_update_name();

	EditorNode::get_singleton()->register_hdr_viewport(viewport);
}

Node3DEditorViewport::~Node3DEditorViewport() {
	memdelete(ruler);
}

//////////////////////////////////////////////////////////////

void Node3DEditorViewportContainer::_update_split_drag_margin() {
	if (view != VIEW_USE_4_VIEWPORTS && view != VIEW_USE_3_VIEWPORTS) {
		return;
	}
	// Also for 3 viewports view since the first split container is used to remember the offset.
	first_split->set_split_offset(second_split->get_split_offset());

	if (view == VIEW_USE_4_VIEWPORTS) {
		// Extend to cover the first split on top.
		second_split->set_drag_area_margin_begin(second_split->get_size().y - get_size().y);
	}
}

void Node3DEditorViewportContainer::_notification(int p_what) {
	switch (p_what) {
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (!EditorSettings::get_singleton()->check_changed_settings_in_group("interface/touchscreen")) {
				return;
			}
			[[fallthrough]];
		}
		case NOTIFICATION_READY: {
			bool touch_optimizations = EDITOR_GET("interface/touchscreen/enable_touch_optimizations");
			main_split->set_touch_dragger_enabled(touch_optimizations);
			first_split->set_touch_dragger_enabled(touch_optimizations);
			second_split->set_touch_dragger_enabled(touch_optimizations);
		} break;
	}
}

void Node3DEditorViewportContainer::set_view(View p_view) {
	view = p_view;

	Node3DEditorViewport *viewports[4];
	for (uint32_t i = 0; i < 4; i++) {
		viewports[i] = contained_viewports[i];
		ERR_FAIL_NULL(viewports[i]);
	}

	const bool previous_main_vertical = !first_split->is_vertical();
	const float horizontal_offset = previous_main_vertical ? first_split->get_split_offset() : main_split->get_split_offset();
	const float vertical_offset = previous_main_vertical ? main_split->get_split_offset() : first_split->get_split_offset();

	first_split->set_dragging_enabled(true);
	second_split->set_drag_area_margin_begin(0);
	viewports[0]->show();

	switch (view) {
		case VIEW_USE_1_VIEWPORT: {
			for (int i = 1; i < 4; i++) {
				viewports[i]->hide();
			}
			second_split->hide();
		} break;
		case VIEW_USE_2_VIEWPORTS:
		case VIEW_USE_2_VIEWPORTS_ALT: {
			viewports[1]->show();
			viewports[2]->hide();
			viewports[3]->hide();
			second_split->hide();
			const bool is_vertical = view == VIEW_USE_2_VIEWPORTS;
			if (first_split->is_vertical() != is_vertical) {
				first_split->set_vertical(is_vertical);
				first_split->set_split_offset(is_vertical ? vertical_offset : horizontal_offset);
				main_split->set_split_offset(is_vertical ? horizontal_offset : vertical_offset); // Store the other offset here for later.
			}
		} break;
		case VIEW_USE_3_VIEWPORTS:
		case VIEW_USE_3_VIEWPORTS_ALT: {
			// Default mode has two on bottom (second_split). Alt mode has two on the left (first_split).
			const bool main_vertical = view == VIEW_USE_3_VIEWPORTS;
			viewports[1]->set_visible(!main_vertical);
			viewports[2]->show();
			viewports[3]->set_visible(main_vertical);
			second_split->show();
			main_split->set_vertical(main_vertical);
			main_split->set_split_offset(main_vertical ? vertical_offset : horizontal_offset);
			first_split->set_vertical(!main_vertical);
			first_split->set_split_offset(main_vertical ? horizontal_offset : vertical_offset);
			second_split->set_split_offset(main_vertical ? horizontal_offset : vertical_offset);
		} break;
		case VIEW_USE_4_VIEWPORTS: {
			for (int i = 1; i < 4; i++) {
				viewports[i]->show();
			}
			second_split->show();
			main_split->set_vertical(true);
			main_split->set_split_offset(vertical_offset);
			first_split->set_vertical(false);
			first_split->set_split_offset(horizontal_offset);
			second_split->set_split_offset(horizontal_offset);

			first_split->set_dragging_enabled(false);
			_update_split_drag_margin();
		} break;
	}
}

Node3DEditorViewportContainer::View Node3DEditorViewportContainer::get_view() {
	return view;
}

void Node3DEditorViewportContainer::add_viewport(Node3DEditorViewport *p_viewport, int p_index) {
	ERR_FAIL_INDEX(p_index, 4);
	contained_viewports[p_index] = p_viewport;
	if (p_index <= 1) {
		first_split->add_child(p_viewport);
	} else {
		second_split->add_child(p_viewport);
		if (p_index == 3) {
			// Connect to the second split's child to update the drag margin immediately whenever the split offset changes.
			p_viewport->connect(SceneStringName(resized), callable_mp(this, &Node3DEditorViewportContainer::_update_split_drag_margin));
		}
	}
}

Dictionary Node3DEditorViewportContainer::get_split_state() const {
	Dictionary state;
	state["main"] = Math::round(main_split->get_split_offset() / EDSCALE);
	state["first"] = Math::round(first_split->get_split_offset() / EDSCALE);
	state["second"] = Math::round(second_split->get_split_offset() / EDSCALE);
	return state;
}

void Node3DEditorViewportContainer::set_split_state(const Dictionary &p_state) {
	if (p_state.has("main")) {
		main_split->set_split_offset(int(p_state["main"]) * EDSCALE);
	}
	if (p_state.has("first")) {
		first_split->set_split_offset(int(p_state["first"]) * EDSCALE);
	}
	if (p_state.has("second")) {
		second_split->set_split_offset(int(p_state["second"]) * EDSCALE);
	}
}

Node3DEditorViewportContainer::Node3DEditorViewportContainer() {
	set_clip_contents(true);

	main_split = memnew(SplitContainer);
	main_split->set_drag_nested_intersections(true);

	first_split = memnew(SplitContainer);
	first_split->set_h_size_flags(SIZE_EXPAND_FILL);
	first_split->set_v_size_flags(SIZE_EXPAND_FILL);
	first_split->set_drag_nested_intersections(true);

	second_split = memnew(SplitContainer);
	second_split->set_h_size_flags(SIZE_EXPAND_FILL);
	second_split->set_v_size_flags(SIZE_EXPAND_FILL);
	second_split->set_drag_nested_intersections(true);

	main_split->add_child(first_split);
	main_split->add_child(second_split);
	add_child(main_split);
}

///////////////////////////////////////////////////////////////////

Node3DEditor *Node3DEditor::active_instance = nullptr;
Vector<Node3DEditor *> Node3DEditor::instances;

Node3DEditor *Node3DEditor::scene_visuals_owner = nullptr;
Vector<Ref<EditorNode3DGizmoPlugin>> Node3DEditor::gizmo_plugins_by_priority;
Vector<Ref<EditorNode3DGizmoPlugin>> Node3DEditor::gizmo_plugins_by_name;

Node3DEditorSelectedItem::~Node3DEditorSelectedItem() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	if (sbox_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance);
	}
	if (sbox_instance_offset.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_offset);
	}
	if (sbox_instance_xray.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_xray);
	}
	if (sbox_instance_xray_offset.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_xray_offset);
	}
}

void Node3DEditor::select_gizmo_highlight_axis(int p_axis) {
	for (int i = 0; i < 3; i++) {
		move_gizmo[i]->surface_set_material(0, i == p_axis ? gizmo_color_hl[i] : gizmo_color[i]);
		move_plane_gizmo[i]->surface_set_material(0, (i + 6) == p_axis ? plane_gizmo_color_hl[i] : plane_gizmo_color[i]);
		scale_gizmo[i]->surface_set_material(0, (i + 9) == p_axis ? gizmo_color_hl[i] : gizmo_color[i]);
		scale_plane_gizmo[i]->surface_set_material(0, (i + 12) == p_axis ? plane_gizmo_color_hl[i] : plane_gizmo_color[i]);
	}

	for (int i = 0; i < 4; i++) {
		bool highlight;
		if (i == 3) {
			highlight = (p_axis == GIZMO_HIGHLIGHT_AXIS_VIEW_ROTATION);
		} else {
			highlight = (i + 3) == p_axis;
		}
		rotate_gizmo[i]->surface_set_material(0, highlight ? rotate_gizmo_color_hl[i] : rotate_gizmo_color[i]);
	}

	bool highlight_trackball = (p_axis == GIZMO_HIGHLIGHT_AXIS_TRACKBALL);
	trackball_sphere_gizmo->surface_set_material(0, highlight_trackball ? trackball_sphere_material_hl : trackball_sphere_material);
}

void Node3DEditor::update_transform_gizmo() {
	int count = 0;
	bool local_gizmo_coords = are_local_coords_enabled();

	Vector3 gizmo_center;
	Basis gizmo_basis;

	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;

	if (se && se->gizmo.is_valid() && is_in_edited_document(se->sp)) {
		for (const KeyValue<int, Transform3D> &E : se->subgizmos) {
			Transform3D xf = se->sp->get_global_transform() * se->gizmo->get_subgizmo_transform(E.key);
			if (!xf.is_finite()) {
				continue;
			}
			gizmo_center += xf.origin;
			if ((unsigned int)count == se->subgizmos.size() - 1 && local_gizmo_coords) {
				gizmo_basis = xf.basis;
			}
			count++;
		}
	} else {
		// This view's document: the manipulator stands over what is selected in
		// the scene this pane shows, not in whichever scene is in context.
		const List<Node *> selection = editor_selection->get_top_selected_node_list_for(get_edited_scene());
		for (Node *E : selection) {
			Node3D *sp = Object::cast_to<Node3D>(E);
			if (!sp) {
				continue;
			}

			// The selection belongs to the editor, not to this view. A node of
			// another open document would put the manipulator at coordinates
			// that mean nothing in the world this view draws into.
			if (!is_in_edited_document(sp)) {
				continue;
			}

			if (sp->has_meta("_edit_lock_")) {
				continue;
			}

			Node3DEditorSelectedItem *sel_item = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
			if (!sel_item) {
				continue;
			}

			Transform3D xf = sel_item->sp->get_global_transform();
			if (!xf.is_finite()) {
				continue;
			}
			gizmo_center += xf.origin;
			if (count == selection.size() - 1 && local_gizmo_coords) {
				gizmo_basis = xf.basis;
			}
			count++;
		}
	}

	gizmo.visible = count > 0;
	gizmo.transform.origin = (count > 0) ? gizmo_center / count : Vector3();
	gizmo.transform.basis = gizmo_basis;

	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->update_transform_gizmo_view();
	}
}

void _update_all_gizmos(Node *p_node) {
	for (int i = p_node->get_child_count() - 1; 0 <= i; --i) {
		Node3D *spatial_node = Object::cast_to<Node3D>(p_node->get_child(i));
		if (spatial_node) {
			spatial_node->update_gizmos();
		}

		_update_all_gizmos(p_node->get_child(i));
	}
}

void Node3DEditor::update_all_gizmos(Node *p_node) {
	if (!p_node && is_inside_tree()) {
		p_node = get_edited_scene();
	}

	if (!p_node) {
		// No edited scene, so nothing to update.
		return;
	}
	_update_all_gizmos(p_node);
}

void Node3DEditorViewport::update_editing_world() {
	// Out of the tree this view renders nothing, and its manipulator instances
	// do not exist. Taking a layer here would be taking one nothing gives back,
	// since only leaving the tree releases it - and entering it again does all
	// of this anyway.
	if (!is_inside_tree()) {
		return;
	}
	const Ref<World3D> world = get_editing_world();
	if (world.is_null()) {
		return;
	}
	viewport->set_world_3d(world);

	// The layer was taken from the world being left, and a free one in the world
	// being entered may well be a different one.
	_acquire_gizmo_layer();
	_apply_gizmo_layer();

	// The manipulator instances were created in whatever world was current when
	// this viewport entered the tree, so carry them over rather than leaving
	// them drawing into a scene nobody is looking at.
	const RID scenario = world->get_scenario();
	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->instance_set_scenario(move_gizmo_instance[i], scenario);
		RS::get_singleton()->instance_set_scenario(move_plane_gizmo_instance[i], scenario);
		RS::get_singleton()->instance_set_scenario(scale_gizmo_instance[i], scenario);
		RS::get_singleton()->instance_set_scenario(scale_plane_gizmo_instance[i], scenario);
		RS::get_singleton()->instance_set_scenario(axis_gizmo_instance[i], scenario);
	}
	for (int i = 0; i < 4; i++) {
		RS::get_singleton()->instance_set_scenario(rotate_gizmo_instance[i], scenario);
	}
	RS::get_singleton()->instance_set_scenario(trackball_sphere_instance, scenario);
}

Node *Node3DEditorViewport::get_edited_scene() const {
	return spatial_editor->get_edited_scene();
}

SubViewport *Node3DEditorViewport::get_scene_root() const {
	return spatial_editor->get_scene_root();
}

Ref<World3D> Node3DEditorViewport::get_editing_world() const {
	return spatial_editor->get_editing_world();
}

static void _count_preview_blockers(Node *p_node, uint32_t &r_world_env_count, uint32_t &r_directional_light_count) {
	if (Object::cast_to<WorldEnvironment>(p_node)) {
		r_world_env_count++;
	} else if (Object::cast_to<DirectionalLight3D>(p_node)) {
		r_directional_light_count++;
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_count_preview_blockers(p_node->get_child(i), r_world_env_count, r_directional_light_count);
	}
}

int Node3DEditor::_shown_document_id() const {
	if (bound_document_id >= 0 && _bound_document_index() >= 0) {
		return bound_document_id;
	}
	EditorData &editor_data = EditorNode::get_editor_data();
	const int current = editor_data.get_edited_scene();
	if (current < 0 || current >= editor_data.get_edited_scene_count()) {
		return -1;
	}
	return editor_data.get_scene_history_id(current);
}

Array Node3DEditor::_save_cameras() const {
	Array cameras;
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		cameras.push_back(viewports[i] ? viewports[i]->get_state() : Dictionary());
	}
	return cameras;
}

void Node3DEditor::_restore_cameras(const Array &p_cameras) {
	for (uint32_t i = 0; i < VIEWPORTS_COUNT && i < (uint32_t)p_cameras.size(); i++) {
		const Dictionary camera = p_cameras[i];
		if (viewports[i] && !camera.is_empty()) {
			viewports[i]->set_state(camera);
		}
	}
}

void Node3DEditor::_switch_cameras() {
	const int now = _shown_document_id();
	if (now == shown_document) {
		return;
	}
	if (shown_document >= 0) {
		cameras_by_document[shown_document] = _save_cameras();
	}
	shown_document = now;
	if (now < 0) {
		return;
	}

	const Array *seen = cameras_by_document.getptr(now);
	if (seen) {
		_restore_cameras(*seen);
		return;
	}
	// Never shown here before: start where the document itself says it was last
	// looked at from, which is what it saved when it was closed.
	EditorData &editor_data = EditorNode::get_editor_data();
	const int idx = editor_data.get_scene_index_by_history_id(now);
	if (idx < 0) {
		return;
	}
	const Dictionary states = editor_data.get_scene_editor_states(idx);
	const Dictionary state = states.get("3D", Dictionary());
	const Array cameras = state.get("viewports", Array());
	if (!cameras.is_empty()) {
		_restore_cameras(cameras);
	}
}

void Node3DEditor::update_editing_world() {
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		if (viewports[i]) {
			viewports[i]->update_editing_world();
		}
	}
	_switch_cameras();

	const Ref<World3D> world = get_editing_world();
	if (world.is_null()) {
		return;
	}

	// The grid and origin lines of the world this view now shows.
	_acquire_world_visuals(world->get_scenario());

	// Documents stay live in roots of their own, so switching between them
	// fires no node-removed notifications and these counts would keep counting
	// the lights and environments of a scene that is no longer in front of us.
	world_env_count = 0;
	directional_light_count = 0;
	Node *edited_scene = get_edited_scene();
	if (edited_scene) {
		_count_preview_blockers(edited_scene, world_env_count, directional_light_count);
	}

	// A different world: the old one is lit by one of its other views, if it
	// has any, and this view shows what the new one is already showing - or,
	// alone in it, what the scene last had.
	if (preview_scenario != world->get_scenario()) {
		_release_preview_owner();
		preview_scenario = world->get_scenario();
		Node3DEditor *companion = nullptr;
		for (Node3DEditor *editor : instances) {
			if (editor != this && editor->preview_scenario == preview_scenario) {
				companion = editor;
				break;
			}
		}
		if (companion) {
			_set_preview_state(companion->_get_preview_state());
		} else {
			EditorData &editor_data = EditorNode::get_editor_data();
			const int index = bound_document_id < 0 ? editor_data.get_edited_scene() : editor_data.get_scene_index_by_history_id(bound_document_id);
			const Dictionary states = index >= 0 ? editor_data.get_scene_editor_states(index) : Dictionary();
			const Dictionary state = states.get("3D", Dictionary());
			if (state.has("preview_sun_env")) {
				_set_preview_state(state["preview_sun_env"]);
			}
		}
	}

	// The preview nodes are sitting in the previous document's root; take them
	// out and let the update below place them in the current one. That document
	// may be gone, in which case they went with it and are rebuilt there.
	_drop_freed_preview_nodes();
	if (preview_sun && preview_sun->get_parent()) {
		preview_sun->get_parent()->remove_child(preview_sun);
		preview_sun_dangling = true;
	}
	if (preview_environment && preview_environment->get_parent()) {
		preview_environment->get_parent()->remove_child(preview_environment);
		preview_env_dangling = true;
	}
	callable_mp(this, &Node3DEditor::_update_preview_environment).call_deferred();
}

void Node3DEditor::bind_document(int p_document_id) {
	if (bound_document_id == p_document_id) {
		return;
	}
	bound_document_id = p_document_id;
	update_editing_world();
}

int Node3DEditor::_bound_document_index() const {
	if (bound_document_id < 0) {
		return -1;
	}
	// A view whose document was closed follows the current one again rather
	// than going blank, which is also what -1 asks for.
	return EditorNode::get_editor_data().get_scene_index_by_history_id(bound_document_id);
}

Node *Node3DEditor::get_edited_scene() const {
	return EditorNode::get_editor_data().get_edited_scene_root(_bound_document_index());
}

SubViewport *Node3DEditor::get_scene_root() const {
	return EditorNode::get_editor_data().get_scene_root_viewport(_bound_document_index());
}

bool Node3DEditor::is_in_edited_document(const Node *p_node) const {
	const Node *edited_scene = get_edited_scene();
	return p_node && edited_scene && (p_node == edited_scene || edited_scene->is_ancestor_of(p_node));
}

Ref<World3D> Node3DEditor::get_editing_world() const {
	// The world belongs to the scene root the edited scene is hosted in, so the
	// gizmos, indicators and picks that go through here follow the scene rather
	// than the editor window.
	SubViewport *scene_root = get_scene_root();
	if (!scene_root) {
		// Asked before the first document exists, while the editor is still
		// being built. Fall back so nothing instances into a null world.
		return get_tree() ? get_tree()->get_root()->get_world_3d() : Ref<World3D>();
	}
	return scene_root->find_world_3d();
}

Object *Node3DEditor::_get_editor_data(Object *p_what) {
	Node3D *sp = Object::cast_to<Node3D>(p_what);
	if (!sp) {
		return nullptr;
	}

	Node3DEditorSelectedItem *si = memnew(Node3DEditorSelectedItem);

	si->sp = sp;
	si->sbox_instance = RenderingServer::get_singleton()->instance_create2(
			selection_box->get_rid(),
			sp->get_world_3d()->get_scenario());
	si->sbox_instance_offset = RenderingServer::get_singleton()->instance_create2(
			selection_box->get_rid(),
			sp->get_world_3d()->get_scenario());
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			si->sbox_instance,
			RSE::SHADOW_CASTING_SETTING_OFF);
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			si->sbox_instance_offset,
			RSE::SHADOW_CASTING_SETTING_OFF);
	// Use the Edit layer to hide the selection box when View Gizmos is disabled, since it is a bit distracting.
	// It's still possible to approximately guess what is selected by looking at the manipulation gizmo position.
	RS::get_singleton()->instance_set_layer_mask(si->sbox_instance, 1 << Node3DEditorViewport::GIZMO_EDIT_LAYER);
	RS::get_singleton()->instance_set_layer_mask(si->sbox_instance_offset, 1 << Node3DEditorViewport::GIZMO_EDIT_LAYER);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_offset, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_offset, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	si->sbox_instance_xray = RenderingServer::get_singleton()->instance_create2(
			selection_box_xray->get_rid(),
			sp->get_world_3d()->get_scenario());
	si->sbox_instance_xray_offset = RenderingServer::get_singleton()->instance_create2(
			selection_box_xray->get_rid(),
			sp->get_world_3d()->get_scenario());
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			si->sbox_instance_xray,
			RSE::SHADOW_CASTING_SETTING_OFF);
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			si->sbox_instance_xray_offset,
			RSE::SHADOW_CASTING_SETTING_OFF);
	// Use the Edit layer to hide the selection box when View Gizmos is disabled, since it is a bit distracting.
	// It's still possible to approximately guess what is selected by looking at the manipulation gizmo position.
	RS::get_singleton()->instance_set_layer_mask(si->sbox_instance_xray, 1 << Node3DEditorViewport::GIZMO_EDIT_LAYER);
	RS::get_singleton()->instance_set_layer_mask(si->sbox_instance_xray_offset, 1 << Node3DEditorViewport::GIZMO_EDIT_LAYER);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_xray, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_xray, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_xray_offset, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(si->sbox_instance_xray_offset, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

	return si;
}

void Node3DEditor::_generate_selection_boxes() {
	// Use two AABBs to create the illusion of a slightly thicker line.
	AABB aabb(Vector3(), Vector3(1, 1, 1));

	// Create a x-ray (visible through solid surfaces) and standard version of the selection box.
	// Both will be drawn at the same position, but with different opacity.
	// This lets the user see where the selection is while still having a sense of depth.
	Ref<SurfaceTool> st = memnew(SurfaceTool);
	Ref<SurfaceTool> st_xray = memnew(SurfaceTool);
	Ref<SurfaceTool> active_st = memnew(SurfaceTool);
	Ref<SurfaceTool> active_st_xray = memnew(SurfaceTool);

	st->begin(Mesh::PRIMITIVE_LINES);
	st_xray->begin(Mesh::PRIMITIVE_LINES);
	active_st->begin(Mesh::PRIMITIVE_LINES);
	active_st_xray->begin(Mesh::PRIMITIVE_LINES);
	for (int i = 0; i < 12; i++) {
		Vector3 a, b;
		aabb.get_edge(i, a, b);

		st->add_vertex(a);
		st->add_vertex(b);
		active_st->add_vertex(a);
		active_st->add_vertex(b);
		st_xray->add_vertex(a);
		st_xray->add_vertex(b);
		active_st_xray->add_vertex(a);
		active_st_xray->add_vertex(b);
	}

	const Color selection_box_color = EDITOR_GET("editors/3d/selection_box_color");
	const Color active_selection_box_color = EDITOR_GET("editors/3d/active_selection_box_color");

	selection_box_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_box_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	selection_box_mat->set_albedo(selection_box_color);
	selection_box_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	st->set_material(selection_box_mat);
	selection_box = st->commit();

	selection_box_mat_xray->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_box_mat_xray->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	selection_box_mat_xray->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	selection_box_mat_xray->set_albedo(selection_box_color * Color(1, 1, 1, 0.15));
	selection_box_mat_xray->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	st_xray->set_material(selection_box_mat_xray);
	selection_box_xray = st_xray->commit();

	active_selection_box_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	active_selection_box_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	active_selection_box_mat->set_albedo(active_selection_box_color);
	active_selection_box_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	active_st->set_material(active_selection_box_mat);
	active_selection_box = active_st->commit();

	active_selection_box_mat_xray->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	active_selection_box_mat_xray->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	active_selection_box_mat_xray->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	active_selection_box_mat_xray->set_albedo(active_selection_box_color * Color(1, 1, 1, 0.15));
	active_selection_box_mat_xray->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	active_st_xray->set_material(active_selection_box_mat_xray);
	active_selection_box_xray = active_st_xray->commit();
}

Dictionary Node3DEditor::get_state() const {
	Dictionary d;

	d["snap_enabled"] = snap_enabled;
	d["trackball_enabled"] = trackball_enabled;
	d["translate_snap"] = snap_translate_value;
	d["rotate_snap"] = snap_rotate_value;
	d["scale_snap"] = snap_scale_value;

	d["local_coords"] = tool_option_button[TOOL_OPT_LOCAL_COORDS]->is_pressed();
	d["preserve_children_transform"] = tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->is_pressed();

	int vc = 0;
	if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT))) {
		vc = 1;
	} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS))) {
		vc = 2;
	} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS))) {
		vc = 3;
	} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS))) {
		vc = 4;
	} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT))) {
		vc = 5;
	} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT))) {
		vc = 6;
	}

	d["viewport_mode"] = vc;
	d["viewport_splits"] = viewport_base->get_split_state();
	Array vpdata;
	for (int i = 0; i < 4; i++) {
		vpdata.push_back(viewports[i]->get_state());
	}

	d["viewports"] = vpdata;

	d["vertex_snap_origin_mode"] = vertex_snap_origin_mode;
	d["vertex_snap_use_collision"] = vertex_snap_use_collision;
	d["show_grid"] = view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_GRID));
	d["show_origin"] = view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_ORIGIN));
	d["fov"] = get_fov();
	d["znear"] = get_znear();
	d["zfar"] = get_zfar();

	Dictionary gizmos_status;
	for (int i = 0; i < gizmo_plugins_by_name.size(); i++) {
		if (!gizmo_plugins_by_name[i]->can_be_hidden()) {
			continue;
		}
		int state = gizmos_menu->get_item_state(gizmos_menu->get_item_index(i));
		String name = gizmo_plugins_by_name[i]->get_gizmo_name();
		gizmos_status[name] = state;
	}

	d["gizmos_status"] = gizmos_status;
	d["preview_sun_env"] = _get_preview_state();

	return d;
}

Dictionary Node3DEditor::_get_preview_state() const {
	Dictionary pd;

	pd["sun_rotation"] = sun_rotation;

	pd["environ_sky_color"] = environ_sky_color->get_pick_color();
	pd["environ_ground_color"] = environ_ground_color->get_pick_color();
	pd["environ_energy"] = environ_energy->get_value();
	pd["environ_glow_enabled"] = environ_glow_button->is_pressed();
	pd["environ_tonemap_enabled"] = environ_tonemap_button->is_pressed();
	pd["environ_ao_enabled"] = environ_ao_button->is_pressed();
	pd["environ_gi_enabled"] = environ_gi_button->is_pressed();
	pd["sun_shadow_max_distance"] = sun_shadow_max_distance->get_value();

	pd["sun_color"] = sun_color->get_pick_color();
	pd["sun_energy"] = sun_energy->get_value();

	pd["sun_enabled"] = sun_button->is_pressed();
	pd["environ_enabled"] = environ_button->is_pressed();
	return pd;
}

void Node3DEditor::_set_preview_state(const Dictionary &p_state) {
	sun_environ_updating = true;
	sun_rotation = p_state.get("sun_rotation", sun_rotation);

	environ_sky_color->set_pick_color(p_state.get("environ_sky_color", environ_sky_color->get_pick_color()));
	environ_ground_color->set_pick_color(p_state.get("environ_ground_color", environ_ground_color->get_pick_color()));
	environ_energy->set_value_no_signal(p_state.get("environ_energy", environ_energy->get_value()));
	environ_glow_button->set_pressed_no_signal(p_state.get("environ_glow_enabled", environ_glow_button->is_pressed()));
	environ_tonemap_button->set_pressed_no_signal(p_state.get("environ_tonemap_enabled", environ_tonemap_button->is_pressed()));
	environ_ao_button->set_pressed_no_signal(p_state.get("environ_ao_enabled", environ_ao_button->is_pressed()));
	environ_gi_button->set_pressed_no_signal(p_state.get("environ_gi_enabled", environ_gi_button->is_pressed()));
	sun_shadow_max_distance->set_value_no_signal(p_state.get("sun_shadow_max_distance", sun_shadow_max_distance->get_value()));

	sun_color->set_pick_color(p_state.get("sun_color", sun_color->get_pick_color()));
	sun_energy->set_value_no_signal(p_state.get("sun_energy", sun_energy->get_value()));

	sun_button->set_pressed_no_signal(p_state.get("sun_enabled", sun_button->is_pressed()));
	environ_button->set_pressed_no_signal(p_state.get("environ_enabled", environ_button->is_pressed()));
	// The angles are shown as well as used.
	sun_angle_altitude->set_value_no_signal(-Math::rad_to_deg(sun_rotation.x));
	sun_angle_azimuth->set_value_no_signal(180.0 - Math::rad_to_deg(sun_rotation.y));

	sun_environ_updating = false;

	_preview_settings_changed();
	_update_preview_environment();
}

void Node3DEditor::_share_preview_settings() {
	if (sharing_preview_settings || !preview_scenario.is_valid()) {
		return;
	}
	// The views of one world show one preview - only one of them puts it in -
	// so they agree on what it is: whichever of them it was changed in.
	sharing_preview_settings = true;
	const Dictionary state = _get_preview_state();
	for (Node3DEditor *editor : instances) {
		if (editor != this && editor->preview_scenario == preview_scenario) {
			editor->_set_preview_state(state);
		}
	}
	sharing_preview_settings = false;
}

bool Node3DEditor::_claim_preview_owner() {
	const Ref<World3D> world = get_editing_world();
	if (world.is_null()) {
		return false;
	}
	const RID scenario = world->get_scenario();
	preview_scenario = scenario;
	const ObjectID *owner_id = preview_owners.getptr(scenario);
	Node3DEditor *owner = owner_id ? ObjectDB::get_instance<Node3DEditor>(*owner_id) : nullptr;
	// Still there, still looking at this world: it is the one lighting it.
	if (owner && owner != this && owner->preview_scenario == scenario) {
		return false;
	}
	preview_owners[scenario] = get_instance_id();
	return true;
}

void Node3DEditor::_release_preview_owner() {
	if (!preview_scenario.is_valid()) {
		return;
	}
	const RID scenario = preview_scenario;
	const ObjectID *owner_id = preview_owners.getptr(scenario);
	if (!owner_id || *owner_id != get_instance_id()) {
		return;
	}
	preview_owners.erase(scenario);
	// Another view of that world lights it now.
	for (Node3DEditor *editor : instances) {
		if (editor != this && editor->preview_scenario == scenario) {
			callable_mp(editor, &Node3DEditor::_update_preview_environment).call_deferred();
		}
	}
}

void Node3DEditor::set_state(const Dictionary &p_state) {
	Dictionary d = p_state;

	if (d.has("snap_enabled")) {
		snap_enabled = d["snap_enabled"];
		tool_option_button[TOOL_OPT_USE_SNAP]->set_pressed(d["snap_enabled"]);
	}

	if (d.has("trackball_enabled")) {
		trackball_enabled = d["trackball_enabled"];
		tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_pressed(d["trackball_enabled"]);
	}

	if (d.has("translate_snap")) {
		snap_translate_value = d["translate_snap"];
	}

	if (d.has("rotate_snap")) {
		snap_rotate_value = d["rotate_snap"];
	}

	if (d.has("scale_snap")) {
		snap_scale_value = d["scale_snap"];
	}

	_snap_update();

	if (d.has("preserve_children_transform")) {
		tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_pressed(d["preserve_children_transform"]);
	}

	if (d.has("vertex_snap_origin_mode")) {
		vertex_snap_origin_mode = d["vertex_snap_origin_mode"];
		int idx_vertex = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX);
		int idx_origin = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_ORIGIN);
		transform_menu->get_popup()->set_item_checked(idx_vertex, !vertex_snap_origin_mode);
		transform_menu->get_popup()->set_item_checked(idx_origin, vertex_snap_origin_mode);
	}

	if (d.has("vertex_snap_use_collision")) {
		vertex_snap_use_collision = d["vertex_snap_use_collision"];
		int idx_mesh = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH);
		int idx_collision = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_COLLISION);
		transform_menu->get_popup()->set_item_checked(idx_mesh, !vertex_snap_use_collision);
		transform_menu->get_popup()->set_item_checked(idx_collision, vertex_snap_use_collision);
	}

	if (d.has("local_coords")) {
		tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_pressed(d["local_coords"]);
		update_transform_gizmo();
	}

	if (d.has("viewport_mode")) {
		int vc = d["viewport_mode"];

		if (vc == 1) {
			_menu_item_pressed(MENU_VIEW_USE_1_VIEWPORT);
		} else if (vc == 2) {
			_menu_item_pressed(MENU_VIEW_USE_2_VIEWPORTS);
		} else if (vc == 3) {
			_menu_item_pressed(MENU_VIEW_USE_3_VIEWPORTS);
		} else if (vc == 4) {
			_menu_item_pressed(MENU_VIEW_USE_4_VIEWPORTS);
		} else if (vc == 5) {
			_menu_item_pressed(MENU_VIEW_USE_2_VIEWPORTS_ALT);
		} else if (vc == 6) {
			_menu_item_pressed(MENU_VIEW_USE_3_VIEWPORTS_ALT);
		}
	}

	if (d.has("zfar")) {
		settings_zfar->set_value(double(d["zfar"]));
	}
	if (d.has("znear")) {
		settings_znear->set_value(double(d["znear"]));
	}
	if (d.has("fov")) {
		settings_fov->set_value(double(d["fov"]));
	}
	if (d.has("viewport_splits")) {
		viewport_base->set_split_state(d["viewport_splits"]);
	}

	if (d.has("viewports")) {
		Array vp = d["viewports"];
		uint32_t vp_size = static_cast<uint32_t>(vp.size());
		if (vp_size > VIEWPORTS_COUNT) {
			WARN_PRINT("Ignoring superfluous viewport settings from spatial editor state.");
			vp_size = VIEWPORTS_COUNT;
		}

		for (uint32_t i = 0; i < vp_size; i++) {
			viewports[i]->set_state(vp[i]);
		}
	}

	if (d.has("show_grid")) {
		bool use = d["show_grid"];

		if (use != view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_GRID))) {
			_menu_item_pressed(MENU_VIEW_GRID);
		}
	}

	if (d.has("show_origin")) {
		bool use = d["show_origin"];

		if (use != view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_ORIGIN))) {
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_ORIGIN), use);
			origin_enabled = use;
			_update_origin_visibility();
		}
	}

	if (d.has("gizmos_status")) {
		Dictionary gizmos_status = d["gizmos_status"];

		for (int j = 0; j < gizmo_plugins_by_name.size(); ++j) {
			if (!gizmo_plugins_by_name[j]->can_be_hidden()) {
				continue;
			}
			int state = EditorNode3DGizmoPlugin::VISIBLE;
			for (const KeyValue<Variant, Variant> &kv : gizmos_status) {
				if (gizmo_plugins_by_name.write[j]->get_gizmo_name() == String(kv.key)) {
					state = kv.value;
					break;
				}
			}

			gizmo_plugins_by_name.write[j]->set_state(state);
		}
		_update_gizmos_menu();
	}

	if (d.has("preview_sun_env")) {
		_set_preview_state(d["preview_sun_env"]);
		_share_preview_settings();
	} else {
		_load_default_preview_settings();
		sun_button->set_pressed(true);
		environ_button->set_pressed(true);
		_preview_settings_changed();
		_update_preview_environment();
	}
}

void Node3DEditor::edit(Node3D *p_spatial) {
	if (p_spatial != selected) {
		if (selected) {
			Vector<Ref<Node3DGizmo>> gizmos = selected->get_gizmos();
			for (int i = 0; i < gizmos.size(); i++) {
				Ref<EditorNode3DGizmo> seg = gizmos[i];
				if (seg.is_null()) {
					continue;
				}
				seg->set_selected(false);
			}

			Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected);
			if (se) {
				se->gizmo.unref();
				se->subgizmos.clear();
			}

			selected->update_gizmos();
		}

		selected = p_spatial;
		current_hover_gizmo = Ref<EditorNode3DGizmo>();
		current_hover_gizmo_handle = -1;
		current_hover_gizmo_handle_secondary = false;

		if (selected) {
			Vector<Ref<Node3DGizmo>> gizmos = selected->get_gizmos();
			for (int i = 0; i < gizmos.size(); i++) {
				Ref<EditorNode3DGizmo> seg = gizmos[i];
				if (seg.is_null()) {
					continue;
				}
				seg->set_selected(true);
			}
			selected->update_gizmos();
		}
	}
}

void Node3DEditor::_snap_changed() {
	snap_translate_value = snap_translate->get_value();
	snap_rotate_value = snap_rotate->get_value();
	snap_scale_value = snap_scale->get_value();

	EditorSettings::get_singleton()->set_project_metadata("3d_editor", "snap_translate_value", snap_translate_value);
	EditorSettings::get_singleton()->set_project_metadata("3d_editor", "snap_rotate_value", snap_rotate_value);
	EditorSettings::get_singleton()->set_project_metadata("3d_editor", "snap_scale_value", snap_scale_value);
}

void Node3DEditor::_snap_update() {
	snap_translate->set_value(snap_translate_value);
	snap_rotate->set_value(snap_rotate_value);
	snap_scale->set_value(snap_scale_value);
}

void Node3DEditor::_update_vertex_snap_tooltips() {
	String snap_key = ED_GET_SHORTCUT("spatial_editor/vertex_snap")->get_as_text();
	PopupMenu *p = transform_menu->get_popup();
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX),
			vformat(TTR("Hold %s to highlight a vertex on the currently selected node,\nthen drag to move the node and snap it to vertices on neighboring nodes.\n\nFor nodes without a vertex-based representation,\nSnap Origin to Vertex is always used instead."), snap_key));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_BASE_ORIGIN),
			vformat(TTR("Hold %s to highlight another node's vertex,\nthen click to teleport the selected node to the highlighted vertex."), snap_key));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH),
			TTR("Snap to vertices of visual meshes.\nHold Shift while vertex snapping to temporarily snap to collision shapes instead."));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_SOURCE_COLLISION),
			TTR("Snap to vertices of collision shapes.\nHold Shift while vertex snapping to temporarily snap to mesh vertices instead."));
}

void Node3DEditor::_xform_dialog_action() {
	Transform3D t;
	//translation
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;

	for (int i = 0; i < 3; i++) {
		translate[i] = xform_translate[i]->get_text().to_float();
		rotate[i] = Math::deg_to_rad(xform_rotate[i]->get_text().to_float());
		scale[i] = xform_scale[i]->get_text().to_float();
	}

	t.basis.scale(scale);
	t.basis.rotate(rotate);
	t.origin = translate;

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("XForm Dialog"));

	const List<Node *> &selection = editor_selection->get_top_selected_node_list();

	for (Node *E : selection) {
		Node3D *sp = Object::cast_to<Node3D>(E);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		bool post = xform_type->get_selected() > 0;

		Transform3D tr = sp->get_global_gizmo_transform();
		if (post) {
			tr = tr * t;
		} else {
			tr.basis = t.basis * tr.basis;
			tr.origin += t.origin;
		}

		Node3D *parent = sp->get_parent_node_3d();
		Transform3D local_tr = parent ? parent->get_global_transform().affine_inverse() * tr : tr;
		undo_redo->add_do_method(sp, "set_transform", local_tr);
		undo_redo->add_undo_method(sp, "set_transform", sp->get_transform());
	}
	undo_redo->commit_action();
}

void Node3DEditor::_menu_item_toggled(bool pressed, int p_option) {
	switch (p_option) {
		case MENU_TOOL_LOCAL_COORDS: {
			tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_pressed(pressed);
			update_transform_gizmo();
		} break;

		case MENU_TOOL_USE_SNAP: {
			tool_option_button[TOOL_OPT_USE_SNAP]->set_pressed(pressed);
			snap_enabled = pressed;
		} break;

		case MENU_TOOL_USE_TRACKBALL: {
			tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_pressed(pressed);
			trackball_enabled = pressed;
			for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
				viewports[i]->update_transform_gizmo_highlight();
			}
		} break;

		case MENU_TOOL_PRESERVE_CHILDREN_TRANSFORM: {
			tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_pressed(pressed);
			if (pressed) {
				EditorNode::get_editor_data().add_undo_redo_inspector_hook_callback(callable_mp(this, &Node3DEditor::_undo_redo_inspector_callback));
			} else {
				EditorNode::get_editor_data().remove_undo_redo_inspector_hook_callback(callable_mp(this, &Node3DEditor::_undo_redo_inspector_callback));
			}
		} break;
	}
}

void Node3DEditor::_undo_redo_inspector_callback(Object *p_undo_redo, Object *p_edited, const String &p_property, const Variant &p_new_value) {
	Node3D *node = Object::cast_to<Node3D>(p_edited);
	if (!node) {
		return;
	}

	static const char *transform_properties[] = { "position", "rotation", "scale", "quaternion", "basis", "transform", nullptr };
	bool is_transform_prop = false;
	for (int i = 0; transform_properties[i]; i++) {
		if (p_property == transform_properties[i]) {
			is_transform_prop = true;
			break;
		}
	}
	if (!is_transform_prop) {
		return;
	}

	EditorUndoRedoManager *undo_redo = Object::cast_to<EditorUndoRedoManager>(p_undo_redo);
	ERR_FAIL_NULL(undo_redo);

	int child_count = node->get_child_count();
	for (int i = 0; i < child_count; i++) {
		Node3D *child = Object::cast_to<Node3D>(node->get_child(i));
		if (child) {
			Transform3D child_global = child->get_global_transform();
			undo_redo->add_do_method(child, "set_global_transform", child_global);
			undo_redo->add_undo_method(child, "set_global_transform", child_global);
		}
	}
}

void Node3DEditor::_menu_gizmo_toggled(int p_option) {
	const int idx = gizmos_menu->get_item_index(p_option);
	gizmos_menu->toggle_item_multistate(idx);

	// Change icon
	const int state = gizmos_menu->get_item_state(idx);
	switch (state) {
		case EditorNode3DGizmoPlugin::VISIBLE:
			gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
			break;
		case EditorNode3DGizmoPlugin::ON_TOP:
			gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityXray")));
			break;
		case EditorNode3DGizmoPlugin::HIDDEN:
			gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityHidden")));
			break;
	}

	gizmo_plugins_by_name.write[p_option]->set_state(state);

	update_all_gizmos();
}

void Node3DEditor::_menu_item_activated(int p_option) {
	// Acting in a pane makes it the one being worked in, exactly as clicking in
	// its viewport does. Without this, a button pressed in one pane would reach
	// for the selection and the undo history of whichever document some other
	// pane had made current - so the buttons of a pane would not be that pane's.
	// Shortcuts land here too, and they are routed to the view under the mouse,
	// which is the pane the user means to act on.
	//
	// Only what the user pressed comes through here. _menu_item_pressed() is
	// also called from inside this class - restoring a document's state, among
	// other things - and switching the current document from there would be
	// answering a question nobody asked.
	make_active();
	EditorMainScreen *main_screen = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_main_screen() : nullptr;
	if (main_screen) {
		main_screen->view_activated(this);
	}
	_menu_item_pressed(p_option);
}

void Node3DEditor::_menu_item_pressed(int p_option) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	switch (p_option) {
		case MENU_TOOL_TRANSFORM:
		case MENU_TOOL_MOVE:
		case MENU_TOOL_ROTATE:
		case MENU_TOOL_SCALE:
		case MENU_TOOL_SELECT:
		case MENU_TOOL_LIST_SELECT: {
			for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
				if (viewports[i]->_edit.mode != Node3DEditorViewport::TRANSFORM_NONE) {
					viewports[i]->commit_transform();
				}
			}

			for (int i = 0; i < TOOL_MAX; i++) {
				tool_button[i]->set_pressed(i == p_option);
			}
			tool_mode = (ToolMode)p_option;
			update_transform_gizmo();

			for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
				viewports[i]->update_transform_gizmo_highlight();
			}
		} break;
		case MENU_TRANSFORM_CONFIGURE_SNAP: {
			if (sidebar) {
				sidebar->toggle_page(SIDEBAR_SNAP);
				break;
			}
			snap_dialog->popup_centered(Size2(200, 180));
		} break;
		case MENU_VERTEX_SNAP_BASE_VERTEX: {
			vertex_snap_origin_mode = false;
			int idx_vertex = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX);
			int idx_origin = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_ORIGIN);
			transform_menu->get_popup()->set_item_checked(idx_vertex, true);
			transform_menu->get_popup()->set_item_checked(idx_origin, false);
		} break;
		case MENU_VERTEX_SNAP_BASE_ORIGIN: {
			vertex_snap_origin_mode = true;
			int idx_vertex = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX);
			int idx_origin = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_BASE_ORIGIN);
			transform_menu->get_popup()->set_item_checked(idx_vertex, false);
			transform_menu->get_popup()->set_item_checked(idx_origin, true);
		} break;
		case MENU_VERTEX_SNAP_SOURCE_MESH: {
			vertex_snap_use_collision = false;
			int idx_mesh = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH);
			int idx_collision = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_COLLISION);
			transform_menu->get_popup()->set_item_checked(idx_mesh, true);
			transform_menu->get_popup()->set_item_checked(idx_collision, false);
		} break;
		case MENU_VERTEX_SNAP_SOURCE_COLLISION: {
			vertex_snap_use_collision = true;
			int idx_mesh = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH);
			int idx_collision = transform_menu->get_popup()->get_item_index(MENU_VERTEX_SNAP_SOURCE_COLLISION);
			transform_menu->get_popup()->set_item_checked(idx_mesh, false);
			transform_menu->get_popup()->set_item_checked(idx_collision, true);
		} break;
		case MENU_TRANSFORM_DIALOG: {
			for (int i = 0; i < 3; i++) {
				xform_translate[i]->set_text("0");
				xform_rotate[i]->set_text("0");
				xform_scale[i]->set_text("1");
			}

			xform_dialog->popup_centered(Size2(320, 240) * EDSCALE);

		} break;
		case MENU_VIEW_USE_1_VIEWPORT: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_1_VIEWPORT);
			if (last_used_viewport > 0) {
				last_used_viewport = 0;
			}

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), true);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), false);

		} break;
		case MENU_VIEW_USE_2_VIEWPORTS: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_2_VIEWPORTS);
			if (last_used_viewport > 1) {
				last_used_viewport = 0;
			}

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), true);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), false);

		} break;
		case MENU_VIEW_USE_2_VIEWPORTS_ALT: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_2_VIEWPORTS_ALT);
			if (last_used_viewport > 1) {
				last_used_viewport = 0;
			}

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), true);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), false);

		} break;
		case MENU_VIEW_USE_3_VIEWPORTS: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_3_VIEWPORTS);
			if (last_used_viewport > 2) {
				last_used_viewport = 0;
			}

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), true);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), false);

		} break;
		case MENU_VIEW_USE_3_VIEWPORTS_ALT: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_3_VIEWPORTS_ALT);
			if (last_used_viewport > 2) {
				last_used_viewport = 0;
			}

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), true);

		} break;
		case MENU_VIEW_USE_4_VIEWPORTS: {
			viewport_base->set_view(Node3DEditorViewportContainer::VIEW_USE_4_VIEWPORTS);

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), true);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), false);
			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), false);

		} break;
		case MENU_VIEW_ORIGIN: {
			bool is_checked = view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(p_option));

			origin_enabled = !is_checked;
			_update_origin_visibility();
			// Update the grid since its appearance depends on whether the origin is enabled
			_rebuild_all_grids();

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(p_option), origin_enabled);
		} break;
		case MENU_VIEW_GRID: {
			bool is_checked = view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(p_option));

			grid_enabled = !is_checked;

			for (int i = 0; i < 3; ++i) {
				if (grid_enable[i]) {
					grid_visible[i] = grid_enabled;
				}
			}
			_rebuild_all_grids();

			view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(p_option), grid_enabled);

		} break;
		case MENU_VIEW_CAMERA_SNAPSHOT: {
			SubViewport *viewport = get_editor_viewport(0)->get_viewport_node();
			if (!viewport) {
				return;
			}

			Camera3D *editor_camera = viewport->get_camera_3d();
			if (!editor_camera) {
				return;
			}

			Node *root = get_edited_scene();
			if (!root) {
				WARN_PRINT("No active scene is currently open. Cannot create camera from view.");
				return;
			}

			int suffix = 1;
			String base_name = "ViewCamera3D";
			String final_name = base_name;
			while (root->has_node(final_name)) {
				final_name = base_name + "_" + itos(suffix++);
			}

			Camera3D *view_camera = memnew(Camera3D);
			view_camera->set_global_transform(editor_camera->get_global_transform());
			view_camera->set_fov(editor_camera->get_fov());
			view_camera->set_near(editor_camera->get_near());
			view_camera->set_far(editor_camera->get_far());
			view_camera->set_name(final_name);

			undo_redo->create_action(TTR("View Camera Snapshot"));
			undo_redo->add_do_method(root, "add_child", view_camera);
			undo_redo->add_do_method(view_camera, "set_owner", root);
			undo_redo->add_undo_method(root, "remove_child", view_camera);
			undo_redo->add_do_reference(view_camera);
			undo_redo->commit_action();

			if (editor_selection) {
				editor_selection->clear();
				editor_selection->add_node(view_camera);
			}
		} break;
		case MENU_VIEW_CAMERA_SETTINGS: {
			if (sidebar) {
				sidebar->toggle_page(SIDEBAR_VIEW);
				break;
			}
			settings_dialog->popup_centered(settings_vbc->get_combined_minimum_size() + Size2(50, 50));
		} break;
		case MENU_SNAP_TO_FLOOR: {
			snap_selected_nodes_to_floor();
		} break;
		case MENU_LOCK_SELECTED: {
			undo_redo->create_action(TTR("Lock Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			for (Node *E : selection) {
				Node3D *spatial = Object::cast_to<Node3D>(E);
				if (!spatial || !spatial->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(spatial, "set_meta", "_edit_lock_", true);
				undo_redo->add_undo_method(spatial, "remove_meta", "_edit_lock_");
				undo_redo->add_do_method(this, "emit_signal", "item_lock_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_lock_status_changed");
			}

			undo_redo->add_do_method(this, "_refresh_menu_icons");
			undo_redo->add_undo_method(this, "_refresh_menu_icons");
			undo_redo->commit_action();
		} break;
		case MENU_UNLOCK_SELECTED: {
			undo_redo->create_action(TTR("Unlock Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			for (Node *E : selection) {
				Node3D *spatial = Object::cast_to<Node3D>(E);
				if (!spatial || !spatial->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(spatial, "remove_meta", "_edit_lock_");
				undo_redo->add_undo_method(spatial, "set_meta", "_edit_lock_", true);
				undo_redo->add_do_method(this, "emit_signal", "item_lock_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_lock_status_changed");
			}

			undo_redo->add_do_method(this, "_refresh_menu_icons");
			undo_redo->add_undo_method(this, "_refresh_menu_icons");
			undo_redo->commit_action();
		} break;
		case MENU_GROUP_SELECTED: {
			undo_redo->create_action(TTR("Group Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			for (Node *E : selection) {
				Node3D *spatial = Object::cast_to<Node3D>(E);
				if (!spatial || !spatial->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(spatial, "set_meta", "_edit_group_", true);
				undo_redo->add_undo_method(spatial, "remove_meta", "_edit_group_");
				undo_redo->add_do_method(this, "emit_signal", "item_group_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_group_status_changed");
			}

			undo_redo->add_do_method(this, "_refresh_menu_icons");
			undo_redo->add_undo_method(this, "_refresh_menu_icons");
			undo_redo->commit_action();
		} break;
		case MENU_UNGROUP_SELECTED: {
			undo_redo->create_action(TTR("Ungroup Selected"));
			const List<Node *> &selection = editor_selection->get_top_selected_node_list();

			for (Node *E : selection) {
				Node3D *spatial = Object::cast_to<Node3D>(E);
				if (!spatial || !spatial->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(spatial, "remove_meta", "_edit_group_");
				undo_redo->add_undo_method(spatial, "set_meta", "_edit_group_", true);
				undo_redo->add_do_method(this, "emit_signal", "item_group_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_group_status_changed");
			}

			undo_redo->add_do_method(this, "_refresh_menu_icons");
			undo_redo->add_undo_method(this, "_refresh_menu_icons");
			undo_redo->commit_action();
		} break;
		case MENU_RULER: {
			for (int i = 0; i < TOOL_MAX; i++) {
				tool_button[i]->set_pressed(i == p_option);
			}
			tool_button[TOOL_RULER]->set_pressed(true);
			tool_mode = ToolMode::TOOL_RULER;
			update_transform_gizmo();
		} break;
	}
}

void Node3DEditor::_init_indicators() {
	// The grid and origin lines are instanced into the world every view shares,
	// so only their owner builds them - and only once. Entering the tree used to
	// happen exactly once; a pane that is split reparents its view, so it
	// happens again, and building them again leaves the old ones in the scenario
	// with the new ones drawn over them. That is what turned the origin's red
	// line pale: two of it, blended.
	//
	// The manipulator meshes further down are this view's own and are always
	// built - a view without them crashes the moment one of its viewports tries
	// to instance them.
	if (scene_visuals_owner == this && !origin_multimesh.is_valid()) {
		origin_enabled = true;
		grid_enabled = true;

		Ref<Shader> origin_shader = memnew(Shader);
		origin_shader->set_code(R"(
// 3D editor origin line shader.

shader_type spatial;
render_mode blend_mix, cull_disabled, unshaded, fog_disabled;

void vertex() {
	vec3 point_a = MODEL_MATRIX[3].xyz;
	// Encoded in scale.
	vec3 point_b = vec3(MODEL_MATRIX[0].x, MODEL_MATRIX[1].y, MODEL_MATRIX[2].z);

	// Points are already in world space, so no need for MODEL_MATRIX anymore.
	vec4 clip_a = PROJECTION_MATRIX * (VIEW_MATRIX * vec4(point_a, 1.0));
	vec4 clip_b = PROJECTION_MATRIX * (VIEW_MATRIX * vec4(point_b, 1.0));

	vec2 screen_a = VIEWPORT_SIZE * (0.5 * clip_a.xy / clip_a.w + 0.5);
	vec2 screen_b = VIEWPORT_SIZE * (0.5 * clip_b.xy / clip_b.w + 0.5);

	vec2 x_basis = normalize(screen_b - screen_a);
	vec2 y_basis = vec2(-x_basis.y, x_basis.x);

	float width = 3.0;
	vec2 screen_point_a = screen_a + width * (VERTEX.x * x_basis + VERTEX.y * y_basis);
	vec2 screen_point_b = screen_b + width * (VERTEX.x * x_basis + VERTEX.y * y_basis);
	vec2 screen_point_final = mix(screen_point_a, screen_point_b, VERTEX.z);

	vec4 clip_final = mix(clip_a, clip_b, VERTEX.z);

	POSITION = vec4(clip_final.w * ((2.0 * screen_point_final) / VIEWPORT_SIZE - 1.0), clip_final.z, clip_final.w);
	UV = VERTEX.yz * clip_final.w;

	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb, vec3(0.04045)));
	}
}

void fragment() {
	// Multiply by 0.5 since UV is actually UV is [-1, 1].
	float line_width = fwidth(UV.x * 0.5);
	float line_uv = abs(UV.x * 0.5);
	float line = smoothstep(line_width * 1.0, line_width * 0.25, line_uv);

	ALBEDO = COLOR.rgb;
	ALPHA *= COLOR.a * line;
}
)");

		origin_mat.instantiate();
		origin_mat->set_shader(origin_shader);

		Vector<Vector3> origin_points;
		origin_points.resize(6);

		origin_points.set(0, Vector3(0.0, -0.5, 0.0));
		origin_points.set(1, Vector3(0.0, -0.5, 1.0));
		origin_points.set(2, Vector3(0.0, 0.5, 1.0));

		origin_points.set(3, Vector3(0.0, -0.5, 0.0));
		origin_points.set(4, Vector3(0.0, 0.5, 1.0));
		origin_points.set(5, Vector3(0.0, 0.5, 0.0));

		Array d;
		d.resize(RSE::ARRAY_MAX);
		d[RSE::ARRAY_VERTEX] = origin_points;

		origin_mesh = RenderingServer::get_singleton()->mesh_create();

		RenderingServer::get_singleton()->mesh_add_surface_from_arrays(origin_mesh, RSE::PRIMITIVE_TRIANGLES, d);
		RenderingServer::get_singleton()->mesh_surface_set_material(origin_mesh, 0, origin_mat->get_rid());

		origin_multimesh = RenderingServer::get_singleton()->multimesh_create();
		RenderingServer::get_singleton()->multimesh_set_mesh(origin_multimesh, origin_mesh);
		RenderingServer::get_singleton()->multimesh_allocate_data(origin_multimesh, 12, RSE::MultimeshTransformFormat::MULTIMESH_TRANSFORM_3D, true, false);
		RenderingServer::get_singleton()->multimesh_set_visible_instances(origin_multimesh, -1);

		LocalVector<float> distances;
		distances.resize(5);
		distances[0] = -1000000.0;
		distances[1] = -1000.0;
		distances[2] = 0.0;
		distances[3] = 1000.0;
		distances[4] = 1000000.0;

		for (int i = 0; i < 3; i++) {
			Color origin_color;
			switch (i) {
				case 0:
					origin_color = get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor));
					break;
				case 1:
					origin_color = get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor));
					break;
				case 2:
					origin_color = get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor));
					break;
				default:
					origin_color = Color();
					break;
			}

			Vector3 axis;
			axis[i] = 1;

			for (int j = 0; j < 4; j++) {
				Transform3D t = Transform3D();
				if (distances[j] > 0.0) {
					t = t.scaled(axis * distances[j + 1]);
					t = t.translated(axis * distances[j]);
				} else {
					t = t.scaled(axis * distances[j]);
					t = t.translated(axis * distances[j + 1]);
				}
				RenderingServer::get_singleton()->multimesh_instance_set_transform(origin_multimesh, i * 4 + j, t);
				RenderingServer::get_singleton()->multimesh_instance_set_color(origin_multimesh, i * 4 + j, origin_color);
			}
		}

		// Instanced into each world by the views looking at it: see
		// _acquire_world_visuals().

		Ref<Shader> grid_shader = memnew(Shader);
		grid_shader->set_code(R"(
// 3D editor grid shader.

shader_type spatial;

render_mode unshaded, fog_disabled;

uniform bool orthogonal;
uniform float grid_size;

void vertex() {
	// From FLAG_SRGB_VERTEX_COLOR.
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb, vec3(0.04045)));
	}
}

void fragment() {
	ALBEDO = COLOR.rgb;
	vec3 dir = orthogonal ? -vec3(0, 0, 1) : VIEW;
	float angle_fade = abs(dot(dir, NORMAL));
	angle_fade = smoothstep(0.05, 0.2, angle_fade);

	vec3 world_pos = (INV_VIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
	vec3 world_normal = (INV_VIEW_MATRIX * vec4(NORMAL, 0.0)).xyz;
	vec3 camera_world_pos = INV_VIEW_MATRIX[3].xyz;
	vec3 camera_world_pos_on_plane = camera_world_pos * (1.0 - world_normal);
	float dist_fade = 1.0 - (distance(world_pos, camera_world_pos_on_plane) / grid_size);
	dist_fade = smoothstep(0.02, 0.3, dist_fade);

	ALPHA = COLOR.a * dist_fade * angle_fade;
}
)");

		for (int i = 0; i < 3; i++) {
			grid_mat[i].instantiate();
			grid_mat[i]->set_shader(grid_shader);
		}

		grid_enable[0] = EDITOR_GET("editors/3d/grid_xy_plane");
		grid_enable[1] = EDITOR_GET("editors/3d/grid_yz_plane");
		grid_enable[2] = EDITOR_GET("editors/3d/grid_xz_plane");
		grid_visible[0] = grid_enable[0];
		grid_visible[1] = grid_enable[1];
		grid_visible[2] = grid_enable[2];
	}

	{
		//move gizmo

		// Inverted zxy.
		Vector3 ivec = Vector3(0, 0, -1);
		Vector3 nivec = Vector3(-1, -1, 0);
		Vector3 ivec2 = Vector3(-1, 0, 0);
		Vector3 ivec3 = Vector3(0, -1, 0);

		for (int i = 0; i < 4; i++) {
			Color col;
			switch (i) {
				case 0:
					col = get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor));
					break;
				case 1:
					col = get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor));
					break;
				case 2:
					col = get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor));
					break;
				case 3:
					col = get_theme_color(SNAME("axis_view_plane_color"), EditorStringName(Editor));
					break;
				default:
					col = Color();
					break;
			}

			col.a = col.a * (float)EDITOR_GET("editors/3d/manipulator_gizmo_opacity");

			if (i < 3) {
				move_gizmo[i].instantiate();
				move_plane_gizmo[i].instantiate();
				scale_gizmo[i].instantiate();
				scale_plane_gizmo[i].instantiate();
				axis_gizmo[i].instantiate();
			}

			rotate_gizmo[i].instantiate();

			const Color albedo = col.from_hsv(col.get_h(), col.get_s() * 0.25, 1.0, 1);

			Ref<StandardMaterial3D> mat;
			Ref<StandardMaterial3D> mat_hl;

			if (i < 3) {
				// Only create standard materials for X, Y, Z axes (move/scale gizmos).
				mat.instantiate();
				mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
				mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
				mat->set_on_top_of_alpha();
				mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
				mat->set_albedo(col);
				gizmo_color[i] = mat;

				mat_hl = mat->duplicate();
				mat_hl->set_albedo(albedo);
				gizmo_color_hl[i] = mat_hl;
			}

			// Only create translate gizmo for X, Y, Z axes (not view rotation).
			if (i < 3) {
				//translate
				{
					Ref<SurfaceTool> surftool;
					surftool.instantiate();
					surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

					// Arrow profile
					const int arrow_points = 5;
					Vector3 arrow[5] = {
						nivec * 0.0 + ivec * 0.0,
						nivec * 0.01 + ivec * 0.0,
						nivec * 0.01 + ivec * GIZMO_ARROW_OFFSET,
						nivec * 0.065 + ivec * GIZMO_ARROW_OFFSET,
						nivec * 0.0 + ivec * (GIZMO_ARROW_OFFSET + GIZMO_ARROW_SIZE),
					};

					int arrow_sides = 16;

					const real_t arrow_sides_step = Math::TAU / arrow_sides;
					for (int k = 0; k < arrow_sides; k++) {
						Basis ma(ivec, k * arrow_sides_step);
						Basis mb(ivec, (k + 1) * arrow_sides_step);

						for (int j = 0; j < arrow_points - 1; j++) {
							Vector3 points[4] = {
								ma.xform(arrow[j]),
								mb.xform(arrow[j]),
								mb.xform(arrow[j + 1]),
								ma.xform(arrow[j + 1]),
							};
							surftool->add_vertex(points[0]);
							surftool->add_vertex(points[1]);
							surftool->add_vertex(points[2]);

							surftool->add_vertex(points[0]);
							surftool->add_vertex(points[2]);
							surftool->add_vertex(points[3]);
						}
					}

					surftool->set_material(mat);
					surftool->commit(move_gizmo[i]);
				}

				// Plane Translation
				{
					Ref<SurfaceTool> surftool;
					surftool.instantiate();
					surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

					Vector3 vec = ivec2 - ivec3;
					Vector3 plane[4] = {
						vec * GIZMO_PLANE_DST,
						vec * GIZMO_PLANE_DST + ivec2 * GIZMO_PLANE_SIZE,
						vec * (GIZMO_PLANE_DST + GIZMO_PLANE_SIZE),
						vec * GIZMO_PLANE_DST - ivec3 * GIZMO_PLANE_SIZE
					};

					Basis ma(ivec, Math::PI / 2);

					Vector3 points[4] = {
						ma.xform(plane[0]),
						ma.xform(plane[1]),
						ma.xform(plane[2]),
						ma.xform(plane[3]),
					};
					surftool->add_vertex(points[0]);
					surftool->add_vertex(points[1]);
					surftool->add_vertex(points[2]);

					surftool->add_vertex(points[0]);
					surftool->add_vertex(points[2]);
					surftool->add_vertex(points[3]);

					Ref<StandardMaterial3D> plane_mat;
					plane_mat.instantiate();
					plane_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
					plane_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
					plane_mat->set_on_top_of_alpha();
					plane_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
					plane_mat->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
					plane_mat->set_albedo(col);
					plane_gizmo_color[i] = plane_mat; // Needed, so we can draw planes from both sides.
					surftool->set_material(plane_mat);
					surftool->commit(move_plane_gizmo[i]);

					Ref<StandardMaterial3D> plane_mat_hl = plane_mat->duplicate();
					plane_mat_hl->set_albedo(albedo);
					plane_gizmo_color_hl[i] = plane_mat_hl; // Needed, so we can draw planes from both sides.
				}
			}

			// Rotate - for all 4 indices (X, Y, Z, and view rotation).
			{
				Ref<SurfaceTool> surftool;
				surftool.instantiate();
				surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

				int n = 128; // number of circle segments
				int m = 3; // number of thickness segments

				real_t step = Math::TAU / n;
				for (int j = 0; j < n; ++j) {
					Basis basis = Basis(ivec, j * step);

					Vector3 vertex = basis.xform(ivec2 * GIZMO_CIRCLE_SIZE);

					for (int k = 0; k < m; ++k) {
						Vector2 ofs = Vector2(Math::cos((Math::TAU * k) / m), Math::sin((Math::TAU * k) / m));
						Vector3 normal = ivec * ofs.x + ivec2 * ofs.y;

						surftool->set_normal(basis.xform(normal));
						surftool->add_vertex(vertex);
					}
				}

				for (int j = 0; j < n; ++j) {
					for (int k = 0; k < m; ++k) {
						int current_ring = j * m;
						int next_ring = ((j + 1) % n) * m;
						int current_segment = k;
						int next_segment = (k + 1) % m;

						surftool->add_index(current_ring + next_segment);
						surftool->add_index(current_ring + current_segment);
						surftool->add_index(next_ring + current_segment);

						surftool->add_index(next_ring + current_segment);
						surftool->add_index(next_ring + next_segment);
						surftool->add_index(current_ring + next_segment);
					}
				}

				Ref<Shader> rotate_shader = memnew(Shader);

				// Use special shader for view rotation (index 3) with camera-relative transformation.
				if (i == 3) {
					rotate_shader->set_code(R"(

shader_type spatial;

render_mode unshaded, depth_test_disabled, fog_disabled;

uniform vec4 albedo;

mat3 orthonormalize(mat3 m) {
	vec3 x = normalize(m[0]);
	vec3 y = normalize(m[1] - x * dot(x, m[1]));
	vec3 z = m[2] - x * dot(x, m[2]);
	z = normalize(z - y * (dot(y, m[2])));
	return mat3(x,y,z);
}

void vertex() {
	mat3 mv = orthonormalize(mat3(MODELVIEW_MATRIX));
	mv = inverse(mv);
	VERTEX += NORMAL * 0.008;
	vec3 camera_dir_local = mv * vec3(0.0, 0.0, 1.0);
	vec3 camera_up_local = mv * vec3(0.0, 1.0, 0.0);
	mat3 rotation_matrix = mat3(cross(camera_dir_local, camera_up_local), camera_up_local, camera_dir_local);
	VERTEX = rotation_matrix * VERTEX;
}

void fragment() {
	ALBEDO = albedo.rgb;
	ALPHA = albedo.a;
}
)");
				} else {
					// Standard shader for X, Y, Z rotation gizmos.
					rotate_shader->set_code(R"(
// 3D editor rotation manipulator gizmo shader.

shader_type spatial;

render_mode unshaded, depth_test_disabled, fog_disabled;

uniform vec4 albedo;

mat3 orthonormalize(mat3 m) {
	vec3 x = normalize(m[0]);
	vec3 y = normalize(m[1] - x * dot(x, m[1]));
	vec3 z = m[2] - x * dot(x, m[2]);
	z = normalize(z - y * (dot(y, m[2])));
	return mat3(x, y, z);
}

void vertex() {
	mat3 mv = orthonormalize(mat3(MODELVIEW_MATRIX));
	vec3 n = mv * VERTEX;
	float orientation = dot(vec3(0.0, 0.0, -1.0), n);
	if (orientation <= 0.005) {
		VERTEX += NORMAL * 0.02;
	}
}

void fragment() {
	ALBEDO = albedo.rgb;
	ALPHA = albedo.a;
}
)");
				}

				Ref<ShaderMaterial> rotate_mat;
				rotate_mat.instantiate();
				rotate_mat->set_render_priority(Material::RENDER_PRIORITY_MAX);
				rotate_mat->set_shader(rotate_shader);
				rotate_mat->set_shader_parameter("albedo", col);
				rotate_gizmo_color[i] = rotate_mat;

				Array arrays = surftool->commit_to_arrays();
				rotate_gizmo[i]->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
				rotate_gizmo[i]->surface_set_material(0, rotate_mat);

				Ref<ShaderMaterial> rotate_mat_hl = rotate_mat->duplicate();
				rotate_mat_hl->set_shader_parameter("albedo", albedo);
				rotate_gizmo_color_hl[i] = rotate_mat_hl;
			}

			// Only create scale gizmo for X, Y, Z axes (not view rotation).
			if (i < 3) {
				// Scale.
				{
					Ref<SurfaceTool> surftool;
					surftool.instantiate();
					surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

					// Cube arrow profile.
					const int arrow_points = 6;
					Vector3 arrow[6] = {
						nivec * 0.0 + ivec * 0.0,
						nivec * 0.01 + ivec * 0.0,
						nivec * 0.01 + ivec * 1.0 * GIZMO_SCALE_OFFSET,
						nivec * 0.07 + ivec * 1.0 * GIZMO_SCALE_OFFSET,
						nivec * 0.07 + ivec * 1.11 * GIZMO_SCALE_OFFSET,
						nivec * 0.0 + ivec * 1.11 * GIZMO_SCALE_OFFSET,
					};

					int arrow_sides = 4;

					const real_t arrow_sides_step = Math::TAU / arrow_sides;
					for (int k = 0; k < 4; k++) {
						Basis ma(ivec, k * arrow_sides_step);
						Basis mb(ivec, (k + 1) * arrow_sides_step);

						for (int j = 0; j < arrow_points - 1; j++) {
							Vector3 points[4] = {
								ma.xform(arrow[j]),
								mb.xform(arrow[j]),
								mb.xform(arrow[j + 1]),
								ma.xform(arrow[j + 1]),
							};
							surftool->add_vertex(points[0]);
							surftool->add_vertex(points[1]);
							surftool->add_vertex(points[2]);

							surftool->add_vertex(points[0]);
							surftool->add_vertex(points[2]);
							surftool->add_vertex(points[3]);
						}
					}

					surftool->set_material(mat);
					surftool->commit(scale_gizmo[i]);
				}

				// Plane Scale.
				{
					Ref<SurfaceTool> surftool;
					surftool.instantiate();
					surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

					Vector3 vec = ivec2 - ivec3;
					Vector3 plane[4] = {
						vec * GIZMO_PLANE_DST,
						vec * GIZMO_PLANE_DST + ivec2 * GIZMO_PLANE_SIZE,
						vec * (GIZMO_PLANE_DST + GIZMO_PLANE_SIZE),
						vec * GIZMO_PLANE_DST - ivec3 * GIZMO_PLANE_SIZE
					};

					Basis ma(ivec, Math::PI / 2);

					Vector3 points[4] = {
						ma.xform(plane[0]),
						ma.xform(plane[1]),
						ma.xform(plane[2]),
						ma.xform(plane[3]),
					};
					surftool->add_vertex(points[0]);
					surftool->add_vertex(points[1]);
					surftool->add_vertex(points[2]);

					surftool->add_vertex(points[0]);
					surftool->add_vertex(points[2]);
					surftool->add_vertex(points[3]);

					Ref<StandardMaterial3D> plane_mat;
					plane_mat.instantiate();
					plane_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
					plane_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
					plane_mat->set_on_top_of_alpha();
					plane_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
					plane_mat->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
					plane_mat->set_albedo(col);
					plane_gizmo_color[i] = plane_mat; // needed, so we can draw planes from both sides.
					surftool->set_material(plane_mat);
					surftool->commit(scale_plane_gizmo[i]);

					Ref<StandardMaterial3D> plane_mat_hl = plane_mat->duplicate();
					plane_mat_hl->set_albedo(col.from_hsv(col.get_h(), col.get_s() * 0.25, 1.0, 1));
					plane_gizmo_color_hl[i] = plane_mat_hl; // needed, so we can draw planes from both sides.
				}

				// Lines to visualize transforms locked to an axis/plane.
				{
					Ref<SurfaceTool> surftool;
					surftool.instantiate();
					surftool->begin(Mesh::PRIMITIVE_LINE_STRIP);

					Vector3 vec;
					vec[i] = 1;

					// Line extending through like infinity.
					surftool->add_vertex(vec * -1048576);
					surftool->add_vertex(Vector3());
					surftool->add_vertex(vec * 1048576);
					surftool->set_material(mat_hl);
					surftool->commit(axis_gizmo[i]);
				}
			}
		}
	}

	// Create trackball sphere
	{
		trackball_sphere_gizmo.instantiate();
		Ref<SurfaceTool> surftool;
		surftool.instantiate();
		surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

		const int sphere_rings = TRACKBALL_SPHERE_RINGS;
		const int sphere_sectors = TRACKBALL_SPHERE_SECTORS;
		const real_t sphere_radius = GIZMO_CIRCLE_SIZE;

		for (int r = 0; r <= sphere_rings; ++r) {
			for (int s = 0; s <= sphere_sectors; ++s) {
				real_t ring_angle = Math::PI * r / sphere_rings;
				real_t sector_angle = 2.0 * Math::PI * s / sphere_sectors;

				Vector3 vertex(
						sphere_radius * Math::sin(ring_angle) * Math::cos(sector_angle),
						sphere_radius * Math::cos(ring_angle),
						sphere_radius * Math::sin(ring_angle) * Math::sin(sector_angle));

				surftool->set_normal(vertex.normalized());
				surftool->add_vertex(vertex);
			}
		}

		for (int r = 0; r < sphere_rings; ++r) {
			for (int s = 0; s < sphere_sectors; ++s) {
				int current = r * (sphere_sectors + 1) + s;
				int next = current + sphere_sectors + 1;

				surftool->add_index(current);
				surftool->add_index(next);
				surftool->add_index(current + 1);

				surftool->add_index(current + 1);
				surftool->add_index(next);
				surftool->add_index(next + 1);
			}
		}

		trackball_sphere_material.instantiate();
		trackball_sphere_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		trackball_sphere_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
		trackball_sphere_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
		trackball_sphere_material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
		trackball_sphere_material->set_albedo(Color(1.0, 1.0, 1.0, 0.0));
		trackball_sphere_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);

		trackball_sphere_material_hl = trackball_sphere_material->duplicate();
		trackball_sphere_material_hl->set_albedo(Color(1.0, 1.0, 1.0, TRACKBALL_HIGHLIGHT_ALPHA));

		surftool->set_material(trackball_sphere_material);
		surftool->commit(trackball_sphere_gizmo);
	}

	_generate_selection_boxes();

	// Now that the shared meshes exist, this view's world gets its grid and
	// origin lines, or shares them with the views already looking at it.
	const Ref<World3D> world = get_editing_world();
	if (world.is_valid()) {
		_acquire_world_visuals(world->get_scenario());
	}
}

void Node3DEditor::_update_gizmos_menu() {
	_fill_gizmos_menu(gizmos_menu);
}

void Node3DEditor::_fill_gizmos_menu(PopupMenu *p_menu) {
	p_menu->clear();

	for (int i = 0; i < gizmo_plugins_by_name.size(); ++i) {
		if (!gizmo_plugins_by_name[i]->can_be_hidden()) {
			continue;
		}
		String plugin_name = gizmo_plugins_by_name[i]->get_gizmo_name();
		const int plugin_state = gizmo_plugins_by_name[i]->get_state();
		p_menu->add_multistate_item(plugin_name, 3, plugin_state, i);
		const int idx = p_menu->get_item_index(i);
		p_menu->set_item_tooltip(
				idx,
				TTR("Click to toggle between visibility states.\n\nOpen eye: Gizmo is visible.\nClosed eye: Gizmo is hidden.\nHalf-open eye: Gizmo is also visible through opaque surfaces (\"x-ray\")."));
		switch (plugin_state) {
			case EditorNode3DGizmoPlugin::VISIBLE:
				p_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
				break;
			case EditorNode3DGizmoPlugin::ON_TOP:
				p_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityXray")));
				break;
			case EditorNode3DGizmoPlugin::HIDDEN:
				p_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityHidden")));
				break;
		}
	}
}

void Node3DEditor::_update_gizmos_menu_theme() {
	for (int i = 0; i < gizmo_plugins_by_name.size(); ++i) {
		if (!gizmo_plugins_by_name[i]->can_be_hidden()) {
			continue;
		}
		const int plugin_state = gizmo_plugins_by_name[i]->get_state();
		const int idx = gizmos_menu->get_item_index(i);
		if (idx < 0) {
			// A view built after the gizmo plugins were registered is themed
			// before its menu is filled from them, and there is nothing yet to
			// put an icon on. _update_gizmos_menu() sets them when it builds it.
			continue;
		}
		switch (plugin_state) {
			case EditorNode3DGizmoPlugin::VISIBLE:
				gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
				break;
			case EditorNode3DGizmoPlugin::ON_TOP:
				gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityXray")));
				break;
			case EditorNode3DGizmoPlugin::HIDDEN:
				gizmos_menu->set_item_icon(idx, get_editor_theme_icon(SNAME("GuiVisibilityHidden")));
				break;
		}
	}
}

void Node3DEditor::_build_world_grid(WorldVisuals &p_visuals, const RID &p_scenario) {
	if (!grid_enabled) {
		return;
	}
	Camera3D *camera = get_editor_viewport(0)->camera;
	Vector3 camera_position = camera->get_position();
	if (camera_position == Vector3()) {
		return; // Camera3D is invalid, don't draw the grid.
	}

	bool orthogonal = camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL;

	static LocalVector<Color> grid_colors[3];
	static LocalVector<Vector3> grid_points[3];
	static LocalVector<Vector3> grid_normals[3];

	for (uint32_t n = 0; n < 3; n++) {
		grid_colors[n].clear();
		grid_points[n].clear();
		grid_normals[n].clear();
	}

	Color primary_grid_color = EDITOR_GET("editors/3d/primary_grid_color");
	Color secondary_grid_color = EDITOR_GET("editors/3d/secondary_grid_color");
	int grid_size = EDITOR_GET("editors/3d/grid_size");
	int primary_grid_steps = EDITOR_GET("editors/3d/primary_grid_steps");

	// Which grid planes are enabled? Which should we generate?
	grid_enable[0] = grid_visible[0] = orthogonal || EDITOR_GET("editors/3d/grid_xy_plane");
	grid_enable[1] = grid_visible[1] = orthogonal || EDITOR_GET("editors/3d/grid_yz_plane");
	grid_enable[2] = grid_visible[2] = orthogonal || EDITOR_GET("editors/3d/grid_xz_plane");

	// Offsets division_level for bigger or smaller grids.
	// Default value is -0.2. -1.0 gives Blender-like behavior, 0.5 gives huge grids.
	real_t division_level_bias = EDITOR_GET("editors/3d/grid_division_level_bias");
	// Default largest grid size is 8^2 (default value is 2) when primary_grid_steps is 8 (64m apart, so primary grid lines are 512m apart).
	int division_level_max = EDITOR_GET("editors/3d/grid_division_level_max");
	// Default smallest grid size is 8^0 (default value is 0) when primary_grid_steps is 8.
	int division_level_min = EDITOR_GET("editors/3d/grid_division_level_min");
	ERR_FAIL_COND_MSG(division_level_max < division_level_min, "The 3D grid's maximum division level cannot be lower than its minimum division level.");

	if (primary_grid_steps != 10) { // Log10 of 10 is 1.
		// Change of base rule, divide by ln(10).
		real_t div = Math::log((real_t)primary_grid_steps) / (real_t)2.302585092994045901094;
		// Truncation (towards zero) is intentional.
		division_level_max = (int)(division_level_max / div);
		division_level_min = (int)(division_level_min / div);
	}

	for (int a = 0; a < 3; a++) {
		if (!grid_enable[a]) {
			continue; // If this grid plane is disabled, skip generation.
		}
		int b = (a + 1) % 3;
		int c = (a + 2) % 3;

		Vector3 normal;
		normal[c] = 1.0;

		real_t camera_distance = Math::abs(camera_position[c]);

		if (orthogonal) {
			camera_distance = camera->get_size() / 2.0;
			Vector3 camera_direction = -camera->get_global_transform().get_basis().get_column(2);
			Plane grid_plane = Plane(normal);
			Vector3 intersection;
			if (grid_plane.intersects_ray(camera_position, camera_direction, &intersection)) {
				camera_position = intersection;
			}
		}

		real_t division_level = Math::log(Math::abs(camera_distance)) / Math::log((double)primary_grid_steps) + division_level_bias;

		real_t clamped_division_level = CLAMP(division_level, division_level_min, division_level_max);
		real_t division_level_floored = Math::floor(clamped_division_level);
		real_t division_level_decimals = clamped_division_level - division_level_floored;

		real_t small_step_size = Math::pow(primary_grid_steps, division_level_floored);
		real_t large_step_size = small_step_size * primary_grid_steps;
		real_t center_a = large_step_size * (int)(camera_position[a] / large_step_size);
		real_t center_b = large_step_size * (int)(camera_position[b] / large_step_size);

		real_t bgn_a = center_a - grid_size * small_step_size;
		real_t end_a = center_a + grid_size * small_step_size;
		real_t bgn_b = center_b - grid_size * small_step_size;
		real_t end_b = center_b + grid_size * small_step_size;

		real_t fade_size = Math::pow(primary_grid_steps, division_level - 1.0);
		real_t min_fade_size = Math::pow(primary_grid_steps, float(division_level_min));
		real_t max_fade_size = Math::pow(primary_grid_steps, float(division_level_max));
		fade_size = CLAMP(fade_size, min_fade_size, max_fade_size);

		real_t grid_fade_size = (grid_size - primary_grid_steps) * fade_size;
		grid_mat[c]->set_shader_parameter("grid_size", grid_fade_size);
		grid_mat[c]->set_shader_parameter("orthogonal", orthogonal);

		LocalVector<Vector3> &ref_grid = grid_points[c];
		LocalVector<Vector3> &ref_grid_normals = grid_normals[c];
		LocalVector<Color> &ref_grid_colors = grid_colors[c];

		// Count our elements same as code below it.
		int expected_size = 0;
		for (int i = -grid_size; i <= grid_size; i++) {
			const real_t position_a = center_a + i * small_step_size;
			const real_t position_b = center_b + i * small_step_size;

			// Don't draw lines over the origin if it's enabled.
			if (!(origin_enabled && Math::is_zero_approx(position_a))) {
				expected_size += 2;
			}

			if (!(origin_enabled && Math::is_zero_approx(position_b))) {
				expected_size += 2;
			}
		}

		int idx = 0;
		ref_grid.resize(expected_size);
		ref_grid_normals.resize(expected_size);
		ref_grid_colors.resize(expected_size);

		// In each iteration of this loop, draw one line in each direction (so two lines per loop, in each if statement).
		for (int i = -grid_size; i <= grid_size; i++) {
			Color line_color;
			// Is this a primary line? Set the appropriate color.
			if (i % primary_grid_steps == 0) {
				line_color = primary_grid_color.lerp(secondary_grid_color, division_level_decimals);
			} else {
				line_color = secondary_grid_color;
				line_color.a = line_color.a * (1 - division_level_decimals);
			}

			real_t position_a = center_a + i * small_step_size;
			real_t position_b = center_b + i * small_step_size;

			// Don't draw lines over the origin if it's enabled.
			if (!(origin_enabled && Math::is_zero_approx(position_a))) {
				Vector3 line_bgn;
				Vector3 line_end;
				line_bgn[a] = position_a;
				line_end[a] = position_a;
				line_bgn[b] = bgn_b;
				line_end[b] = end_b;
				ref_grid[idx] = line_bgn;
				ref_grid[idx + 1] = line_end;
				ref_grid_colors[idx] = line_color;
				ref_grid_colors[idx + 1] = line_color;
				ref_grid_normals[idx] = normal;
				ref_grid_normals[idx + 1] = normal;
				idx += 2;
			}

			if (!(origin_enabled && Math::is_zero_approx(position_b))) {
				Vector3 line_bgn;
				Vector3 line_end;
				line_bgn[b] = position_b;
				line_end[b] = position_b;
				line_bgn[a] = bgn_a;
				line_end[a] = end_a;
				ref_grid[idx] = line_bgn;
				ref_grid[idx + 1] = line_end;
				ref_grid_colors[idx] = line_color;
				ref_grid_colors[idx + 1] = line_color;
				ref_grid_normals[idx] = normal;
				ref_grid_normals[idx + 1] = normal;
				idx += 2;
			}
		}

		// Create a mesh from the pushed vector points and colors.
		p_visuals.grid_mesh[c] = RenderingServer::get_singleton()->mesh_create();
		Array d;
		d.resize(RSE::ARRAY_MAX);
		d[RSE::ARRAY_VERTEX] = (Vector<Vector3>)grid_points[c];
		d[RSE::ARRAY_COLOR] = (Vector<Color>)grid_colors[c];
		d[RSE::ARRAY_NORMAL] = (Vector<Vector3>)grid_normals[c];
		RenderingServer::get_singleton()->mesh_add_surface_from_arrays(p_visuals.grid_mesh[c], RSE::PRIMITIVE_LINES, d);
		RenderingServer::get_singleton()->mesh_surface_set_material(p_visuals.grid_mesh[c], 0, grid_mat[c]->get_rid());
		p_visuals.grid_instance[c] = RenderingServer::get_singleton()->instance_create2(p_visuals.grid_mesh[c], p_scenario);

		// Yes, the end of this line is supposed to be a.
		RenderingServer::get_singleton()->instance_set_visible(p_visuals.grid_instance[c], grid_visible[a]);
		RenderingServer::get_singleton()->instance_geometry_set_cast_shadows_setting(p_visuals.grid_instance[c], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(p_visuals.grid_instance[c], 1 << Node3DEditorViewport::GIZMO_GRID_LAYER);
		RS::get_singleton()->instance_geometry_set_flag(p_visuals.grid_instance[c], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(p_visuals.grid_instance[c], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	}

	p_visuals.grid_built = true;
	p_visuals.grid_center = camera_position;
	p_visuals.grid_projection = camera->get_projection();
}

void Node3DEditor::_finish_indicators() {
	// The last view is going: every world's visuals go with it, and the shared
	// meshes after them. Cleared, not just freed: whether these exist is what
	// says the shared visuals have been built.
	for (KeyValue<RID, WorldVisuals> &E : world_visuals) {
		_free_world_grid(E.value);
		if (E.value.origin_instance.is_valid()) {
			RenderingServer::get_singleton()->free_rid(E.value.origin_instance);
		}
	}
	world_visuals.clear();
	RenderingServer::get_singleton()->free_rid(origin_multimesh);
	RenderingServer::get_singleton()->free_rid(origin_mesh);
	origin_multimesh = RID();
	origin_mesh = RID();
}

void Node3DEditor::_free_world_grid(WorldVisuals &p_visuals) {
	for (int i = 0; i < 3; i++) {
		if (p_visuals.grid_instance[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(p_visuals.grid_instance[i]);
			p_visuals.grid_instance[i] = RID();
		}
		if (p_visuals.grid_mesh[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(p_visuals.grid_mesh[i]);
			p_visuals.grid_mesh[i] = RID();
		}
	}
	p_visuals.grid_built = false;
}

void Node3DEditor::_acquire_world_visuals(const RID &p_scenario) {
	if (visuals_scenario == p_scenario) {
		return;
	}
	_release_world_visuals();
	if (!p_scenario.is_valid() || !origin_multimesh.is_valid()) {
		// Not built yet: the view that builds them comes by here once it has.
		return;
	}

	WorldVisuals &visuals = world_visuals[p_scenario];
	visuals.users.push_back(get_instance_id());
	visuals_scenario = p_scenario;

	if (!visuals.origin_instance.is_valid()) {
		visuals.origin_instance = RenderingServer::get_singleton()->instance_create2(origin_multimesh, p_scenario);
		RS::get_singleton()->instance_set_layer_mask(visuals.origin_instance, 1 << Node3DEditorViewport::GIZMO_GRID_LAYER);
		RS::get_singleton()->instance_geometry_set_flag(visuals.origin_instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(visuals.origin_instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		RenderingServer::get_singleton()->instance_geometry_set_cast_shadows_setting(visuals.origin_instance, RSE::SHADOW_CASTING_SETTING_OFF);
		RenderingServer::get_singleton()->instance_set_visible(visuals.origin_instance, origin_enabled);
	}
	if (!visuals.grid_built) {
		_free_world_grid(visuals);
		_build_world_grid(visuals, p_scenario);
	}
}

void Node3DEditor::_release_world_visuals() {
	if (!visuals_scenario.is_valid()) {
		return;
	}
	const RID scenario = visuals_scenario;
	visuals_scenario = RID();
	WorldVisuals *visuals = world_visuals.getptr(scenario);
	if (!visuals) {
		return;
	}
	const bool was_keeper = !visuals->users.is_empty() && visuals->users[0] == get_instance_id();
	visuals->users.erase(get_instance_id());

	// Views that went away without saying so.
	for (int i = int(visuals->users.size()) - 1; i >= 0; i--) {
		if (!ObjectDB::get_instance(visuals->users[i])) {
			visuals->users.remove_at(i);
		}
	}

	if (visuals->users.is_empty()) {
		// Nobody is looking at this world any more.
		_free_world_grid(*visuals);
		if (visuals->origin_instance.is_valid()) {
			RenderingServer::get_singleton()->free_rid(visuals->origin_instance);
		}
		world_visuals.erase(scenario);
		return;
	}
	if (was_keeper) {
		// The grid was around this view's camera; now it is around the next
		// one's, which is the one that will keep it up to date.
		Node3DEditor *keeper = ObjectDB::get_instance<Node3DEditor>(visuals->users[0]);
		_free_world_grid(*visuals);
		if (keeper) {
			keeper->_build_world_grid(*visuals, scenario);
		}
	}
}

bool Node3DEditor::world_has_grid_and_origin(const RID &p_scenario) {
	const WorldVisuals *visuals = world_visuals.getptr(p_scenario);
	if (!visuals || !visuals->origin_instance.is_valid()) {
		return false;
	}
	for (int i = 0; i < 3; i++) {
		if (visuals->grid_instance[i].is_valid()) {
			return true;
		}
	}
	return false;
}

int Node3DEditor::count_preview_suns_in(const RID &p_scenario) {
	int count = 0;
	for (const Node3DEditor *editor : instances) {
		const DirectionalLight3D *sun = editor->preview_sun;
		if (sun && ObjectDB::get_instance(editor->preview_sun_id) && sun->is_inside_tree() && sun->get_world_3d().is_valid() && sun->get_world_3d()->get_scenario() == p_scenario) {
			count++;
		}
	}
	return count;
}

void Node3DEditor::_rebuild_all_grids() {
	// What the grid looks like changed - planes turned on or off, colors, the
	// origin shown or not - so every world's is built again, each around its
	// own keeper's camera.
	for (KeyValue<RID, WorldVisuals> &E : world_visuals) {
		_free_world_grid(E.value);
		Node3DEditor *keeper = E.value.users.is_empty() ? nullptr : ObjectDB::get_instance<Node3DEditor>(E.value.users[0]);
		if (keeper) {
			keeper->_build_world_grid(E.value, E.key);
		}
	}
}

void Node3DEditor::_update_origin_visibility() {
	for (KeyValue<RID, WorldVisuals> &E : world_visuals) {
		if (E.value.origin_instance.is_valid()) {
			RenderingServer::get_singleton()->instance_set_visible(E.value.origin_instance, origin_enabled);
		}
	}
}

void Node3DEditor::update_gizmo_opacity() {
	if (!origin_multimesh.is_valid()) {
		return;
	}

	const float opacity = EDITOR_GET("editors/3d/manipulator_gizmo_opacity");

	for (int i = 0; i < 3; i++) {
		Color col = gizmo_color[i]->get_albedo();
		col.a = opacity;
		gizmo_color[i]->set_albedo(col);

		col = gizmo_color_hl[i]->get_albedo();
		col.a = 1.0;
		gizmo_color_hl[i]->set_albedo(col);

		col = plane_gizmo_color[i]->get_albedo();
		col.a = opacity;
		plane_gizmo_color[i]->set_albedo(col);

		col = plane_gizmo_color_hl[i]->get_albedo();
		col.a = 1.0;
		plane_gizmo_color_hl[i]->set_albedo(col);
	}
}

void Node3DEditor::update_grid() {
	WorldVisuals *visuals = world_visuals.getptr(visuals_scenario);
	if (!visuals || visuals->users.is_empty() || visuals->users[0] != get_instance_id()) {
		// Another view of this world keeps its grid; moving this camera is no
		// reason to rebuild it - and never in some other world.
		return;
	}
	Camera3D *camera = get_editor_viewport(0)->camera;
	const Vector3 camera_position = camera->get_position();
	if (!visuals->grid_built || camera->get_projection() != visuals->grid_projection || visuals->grid_center.distance_squared_to(camera_position) >= 100.0f) {
		_free_world_grid(*visuals);
		_build_world_grid(*visuals, visuals_scenario);
	}
}

void Node3DEditor::_selection_changed() {
	_refresh_menu_icons();

	// This view's document, not the one in context: a pane showing another scene
	// draws what is selected in *that* scene, and both are live at once.
	Node *edited_scene = get_edited_scene();
	const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection_for(edited_scene);
	const List<Node *> top_selected = editor_selection->get_top_selected_node_list_for(edited_scene);

	for (const KeyValue<ObjectID, Object *> &E : selection) {
		Node3D *sp = ObjectDB::get_instance<Node3D>(E.key);
		if (!sp) {
			continue;
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
		if (!se) {
			continue;
		}

		if (!top_selected.is_empty() && sp == top_selected.back()->get()) {
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance, active_selection_box->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_xray, active_selection_box_xray->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_offset, active_selection_box->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_xray_offset, active_selection_box_xray->get_rid());
		} else {
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance, selection_box->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_xray, selection_box_xray->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_offset, selection_box->get_rid());
			RenderingServer::get_singleton()->instance_set_base(se->sbox_instance_xray_offset, selection_box_xray->get_rid());
		}
	}

	if (selected && top_selected.size() != 1) {
		Vector<Ref<Node3DGizmo>> gizmos = selected->get_gizmos();
		for (int i = 0; i < gizmos.size(); i++) {
			Ref<EditorNode3DGizmo> seg = gizmos[i];
			if (seg.is_null()) {
				continue;
			}
			seg->set_selected(false);
		}

		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected);
		if (se) {
			se->gizmo.unref();
			se->subgizmos.clear();
		}
		selected->update_gizmos();
		selected = nullptr;
	}

	// Ensure gizmo updates are performed when the selection changes
	// outside of the 3D view (see GH-106713).
	if (!is_visible()) {
		if (top_selected.size() == 1) {
			Node3D *new_selected = Object::cast_to<Node3D>(top_selected.back()->get());
			if (new_selected != selected) {
				gizmos_dirty = true;
			}
		}
	}

	update_transform_gizmo();
}

void Node3DEditor::refresh_dirty_gizmos() {
	if (!gizmos_dirty) {
		return;
	}

	const List<Node *> &top_selected = editor_selection->get_top_selected_node_list();
	if (top_selected.size() == 1) {
		Node3D *new_selected = Object::cast_to<Node3D>(top_selected.back()->get());
		if (new_selected != selected) {
			edit(new_selected);
		}
	}
	gizmos_dirty = false;
}

void Node3DEditor::_refresh_menu_icons() {
	bool all_locked = true;
	bool all_grouped = true;
	bool has_node3d_item = false;

	// This pane's buttons report on this pane's scene.
	const List<Node *> selection = editor_selection->get_top_selected_node_list_for(get_edited_scene());

	if (selection.is_empty()) {
		all_locked = false;
		all_grouped = false;
	} else {
		for (Node *E : selection) {
			Node3D *node = Object::cast_to<Node3D>(E);
			if (node) {
				if (all_locked && !node->has_meta("_edit_lock_")) {
					all_locked = false;
				}
				if (all_grouped && !node->has_meta("_edit_group_")) {
					all_grouped = false;
				}
				has_node3d_item = true;
			}
			if (!all_locked && !all_grouped) {
				break;
			}
		}
	}

	all_locked = all_locked && has_node3d_item;
	all_grouped = all_grouped && has_node3d_item;

	tool_button[TOOL_LOCK_SELECTED]->set_visible(!all_locked);
	tool_button[TOOL_LOCK_SELECTED]->set_disabled(!has_node3d_item);
	tool_button[TOOL_UNLOCK_SELECTED]->set_visible(all_locked);
	tool_button[TOOL_UNLOCK_SELECTED]->set_disabled(!has_node3d_item);

	tool_button[TOOL_GROUP_SELECTED]->set_visible(!all_grouped);
	tool_button[TOOL_GROUP_SELECTED]->set_disabled(!has_node3d_item);
	tool_button[TOOL_UNGROUP_SELECTED]->set_visible(all_grouped);
	tool_button[TOOL_UNGROUP_SELECTED]->set_disabled(!has_node3d_item);
}

template <typename T>
HashSet<T *> _get_child_nodes(Node *parent_node) {
	HashSet<T *> nodes = HashSet<T *>();
	T *node = Node::cast_to<T>(parent_node);
	if (node) {
		nodes.insert(node);
	}

	for (int i = 0; i < parent_node->get_child_count(); i++) {
		Node *child_node = parent_node->get_child(i);
		HashSet<T *> child_nodes = _get_child_nodes<T>(child_node);
		for (T *I : child_nodes) {
			nodes.insert(I);
		}
	}

	return nodes;
}

HashSet<RID> _get_physics_bodies_rid(Node *node) {
	HashSet<RID> rids = HashSet<RID>();
	PhysicsBody3D *pb = Node::cast_to<PhysicsBody3D>(node);
	if (pb) {
		rids.insert(pb->get_rid());
	}
	HashSet<PhysicsBody3D *> child_nodes = _get_child_nodes<PhysicsBody3D>(node);
	for (const PhysicsBody3D *I : child_nodes) {
		rids.insert(I->get_rid());
	}

	return rids;
}

void Node3DEditor::snap_selected_nodes_to_floor() {
	do_snap_selected_nodes_to_floor = true;
}

void Node3DEditor::_snap_selected_nodes_to_floor() {
	const List<Node *> &selection = editor_selection->get_top_selected_node_list();
	Dictionary snap_data;

	for (Node *E : selection) {
		Node3D *sp = Object::cast_to<Node3D>(E);
		if (sp) {
			Vector3 from;
			Vector3 position_offset;

			// Priorities for snapping to floor are CollisionShapes, VisualInstances and then origin
			HashSet<VisualInstance3D *> vi = _get_child_nodes<VisualInstance3D>(sp);
			HashSet<CollisionShape3D *> cs = _get_child_nodes<CollisionShape3D>(sp);
			bool found_valid_shape = false;

			if (cs.size()) {
				AABB aabb;
				HashSet<CollisionShape3D *>::Iterator I = cs.begin();
				if ((*I)->get_shape().is_valid()) {
					CollisionShape3D *collision_shape = *cs.begin();
					aabb = collision_shape->get_global_transform().xform(collision_shape->get_shape()->get_debug_mesh()->get_aabb());
					found_valid_shape = true;
				}

				for (++I; I; ++I) {
					CollisionShape3D *col_shape = *I;
					if (col_shape->get_shape().is_valid()) {
						aabb.merge_with(col_shape->get_global_transform().xform(col_shape->get_shape()->get_debug_mesh()->get_aabb()));
						found_valid_shape = true;
					}
				}
				if (found_valid_shape) {
					Vector3 size = aabb.size * Vector3(0.5, 0.0, 0.5);
					from = aabb.position + size;
					position_offset.y = from.y - sp->get_global_transform().origin.y;
				}
			}
			if (!found_valid_shape && vi.size()) {
				VisualInstance3D *begin = *vi.begin();
				AABB aabb = begin->get_global_transform().xform(begin->get_aabb());
				for (const VisualInstance3D *I : vi) {
					aabb.merge_with(I->get_global_transform().xform(I->get_aabb()));
				}
				Vector3 size = aabb.size * Vector3(0.5, 0.0, 0.5);
				from = aabb.position + size;
				position_offset.y = from.y - sp->get_global_transform().origin.y;
			} else if (!found_valid_shape) {
				from = sp->get_global_transform().origin;
			}

			// We add a bit of margin to the from position to avoid it from snapping
			// when the spatial is already on a floor and there's another floor under
			// it
			from = from + Vector3(0.0, 1, 0.0);

			Dictionary d;

			d["from"] = from;
			d["position_offset"] = position_offset;
			snap_data[sp] = d;
		}
	}

	PhysicsDirectSpaceState3D *ss = get_editing_world()->get_direct_space_state();
	PhysicsDirectSpaceState3D::RayResult result;

	// The maximum height an object can travel to be snapped
	const float max_snap_height = 500.0;

	// Will be set to `true` if at least one node from the selection was successfully snapped
	bool snapped_to_floor = false;

	if (!snap_data.is_empty()) {
		// For snapping to be performed, there must be solid geometry under at least one of the selected nodes.
		// We need to check this before snapping to register the undo/redo action only if needed.
		for (const KeyValue<Variant, Variant> &kv : snap_data) {
			Node *node = Object::cast_to<Node>(kv.key);
			Node3D *sp = Object::cast_to<Node3D>(node);
			Dictionary d = kv.value;
			Vector3 from = d["from"];
			Vector3 to = from - Vector3(0.0, max_snap_height, 0.0);
			HashSet<RID> excluded = _get_physics_bodies_rid(sp);

			PhysicsDirectSpaceState3D::RayParameters ray_params;
			ray_params.from = from;
			ray_params.to = to;
			ray_params.exclude = excluded;

			if (ss->intersect_ray(ray_params, result)) {
				snapped_to_floor = true;
			}
		}

		if (snapped_to_floor) {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->create_action(TTR("Snap Nodes to Floor"));

			// Perform snapping if at least one node can be snapped
			for (const KeyValue<Variant, Variant> &kv : snap_data) {
				Node *node = Object::cast_to<Node>(kv.key);
				Node3D *sp = Object::cast_to<Node3D>(node);
				Dictionary d = kv.value;
				Vector3 from = d["from"];
				Vector3 to = from - Vector3(0.0, max_snap_height, 0.0);
				HashSet<RID> excluded = _get_physics_bodies_rid(sp);

				PhysicsDirectSpaceState3D::RayParameters ray_params;
				ray_params.from = from;
				ray_params.to = to;
				ray_params.exclude = excluded;

				if (ss->intersect_ray(ray_params, result)) {
					Vector3 position_offset = d["position_offset"];
					Transform3D new_transform = sp->get_global_transform();

					new_transform.origin.y = result.position.y;
					new_transform.origin = new_transform.origin - position_offset;

					Node3D *parent = sp->get_parent_node_3d();
					Transform3D new_local_xform = parent ? parent->get_global_transform().affine_inverse() * new_transform : new_transform;
					undo_redo->add_do_method(sp, "set_transform", new_local_xform);
					undo_redo->add_undo_method(sp, "set_transform", sp->get_transform());
				}
			}

			undo_redo->commit_action();
		} else {
			EditorNode::get_singleton()->show_warning(TTR("Couldn't find a solid floor to snap the selection to."));
		}
	}
}

void Node3DEditor::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (!is_visible_in_tree()) {
		return;
	}

	snap_key_enabled = Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL);
}

void Node3DEditor::_sun_environ_settings_pressed() {
	if (sidebar) {
		sidebar->toggle_page(SIDEBAR_ENVIRONMENT);
		return;
	}
	Vector2 pos = sun_environ_settings->get_screen_position() + sun_environ_settings->get_size();
	sun_environ_popup->set_position(pos - Vector2(sun_environ_popup->get_contents_minimum_size().width / 2, 0));
	sun_environ_popup->reset_size();
	sun_environ_popup->popup();
	// Grabbing the focus is required for Shift modifier checking to be functional
	// (when the Add sun/environment buttons are pressed).
	sun_environ_popup->grab_focus();
}

void Node3DEditor::_add_sun_to_scene(bool p_already_added_environment) {
	_ensure_preview_nodes();
	sun_environ_popup->hide();

	if (!p_already_added_environment && world_env_count == 0 && Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		// Prevent infinite feedback loop between the sun and environment methods.
		_add_environment_to_scene(true);
	}

	Node *base = get_edited_scene();
	if (!base) {
		// Create a root node so we can add child nodes to it.
		SceneTreeDock::get_singleton()->add_root_node(memnew(Node3D));
		base = get_edited_scene();
	}
	ERR_FAIL_NULL(base);
	Node *new_sun = preview_sun->duplicate();

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Preview Sun to Scene"));
	undo_redo->add_do_method(base, "add_child", new_sun, true);
	// Move to the beginning of the scene tree since more "global" nodes
	// generally look better when placed at the top.
	undo_redo->add_do_method(base, "move_child", new_sun, 0);
	undo_redo->add_do_method(new_sun, "set_owner", base);
	undo_redo->add_undo_method(base, "remove_child", new_sun);
	undo_redo->add_do_reference(new_sun);
	undo_redo->commit_action();
}

void Node3DEditor::_add_environment_to_scene(bool p_already_added_sun) {
	_ensure_preview_nodes();
	sun_environ_popup->hide();

	if (!p_already_added_sun && directional_light_count == 0 && Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		// Prevent infinite feedback loop between the sun and environment methods.
		_add_sun_to_scene(true);
	}

	Node *base = get_edited_scene();
	if (!base) {
		// Create a root node so we can add child nodes to it.
		SceneTreeDock::get_singleton()->add_root_node(memnew(Node3D));
		base = get_edited_scene();
	}
	ERR_FAIL_NULL(base);

	WorldEnvironment *new_env = memnew(WorldEnvironment);
	new_env->set_environment(preview_environment->get_environment()->duplicate(true));
	if (GLOBAL_GET("rendering/lights_and_shadows/use_physical_light_units")) {
		new_env->set_camera_attributes(preview_environment->get_camera_attributes()->duplicate(true));
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Preview Environment to Scene"));
	undo_redo->add_do_method(base, "add_child", new_env, true);
	// Move to the beginning of the scene tree since more "global" nodes
	// generally look better when placed at the top.
	undo_redo->add_do_method(base, "move_child", new_env, 0);
	undo_redo->add_do_method(new_env, "set_owner", base);
	undo_redo->add_undo_method(base, "remove_child", new_env);
	undo_redo->add_do_reference(new_env);
	undo_redo->commit_action();
}

void Node3DEditor::_update_theme() {
	tool_button[TOOL_MODE_TRANSFORM]->set_button_icon(get_editor_theme_icon(SNAME("ToolTransform")));
	tool_button[TOOL_MODE_MOVE]->set_button_icon(get_editor_theme_icon(SNAME("ToolMove")));
	tool_button[TOOL_MODE_ROTATE]->set_button_icon(get_editor_theme_icon(SNAME("ToolRotate")));
	tool_button[TOOL_MODE_SCALE]->set_button_icon(get_editor_theme_icon(SNAME("ToolScale")));
	tool_button[TOOL_MODE_SELECT]->set_button_icon(get_editor_theme_icon(SNAME("ToolSelect")));
	tool_button[TOOL_MODE_LIST_SELECT]->set_button_icon(get_editor_theme_icon(SNAME("ListSelect")));
	tool_button[TOOL_LOCK_SELECTED]->set_button_icon(get_editor_theme_icon(SNAME("Lock")));
	tool_button[TOOL_UNLOCK_SELECTED]->set_button_icon(get_editor_theme_icon(SNAME("Unlock")));
	tool_button[TOOL_GROUP_SELECTED]->set_button_icon(get_editor_theme_icon(SNAME("Group")));
	tool_button[TOOL_UNGROUP_SELECTED]->set_button_icon(get_editor_theme_icon(SNAME("Ungroup")));
	tool_button[TOOL_RULER]->set_button_icon(get_editor_theme_icon(SNAME("Ruler")));

	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_button_icon(get_editor_theme_icon(SNAME("Object")));
	tool_option_button[TOOL_OPT_USE_SNAP]->set_button_icon(get_editor_theme_icon(SNAME("Snap")));
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_button_icon(get_editor_theme_icon(SNAME("Trackball")));
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_button_icon(get_editor_theme_icon(SNAME("Pin")));

	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT), get_editor_theme_icon(SNAME("Panels1")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS), get_editor_theme_icon(SNAME("Panels2")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT), get_editor_theme_icon(SNAME("Panels2Alt")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS), get_editor_theme_icon(SNAME("Panels3")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT), get_editor_theme_icon(SNAME("Panels3Alt")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS), get_editor_theme_icon(SNAME("Panels4")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_CAMERA_SNAPSHOT), get_editor_theme_icon(SNAME("CameraSnapshot")));
	view_layout_menu->get_popup()->set_item_icon(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_CAMERA_SETTINGS), get_editor_theme_icon(SNAME("Settings")));

	sun_button->set_button_icon(get_editor_theme_icon(SNAME("PreviewSun")));
	environ_button->set_button_icon(get_editor_theme_icon(SNAME("PreviewEnvironment")));
	sun_environ_settings->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));

	sun_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("title_font"), SNAME("Window")));
	environ_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("title_font"), SNAME("Window")));

	sun_color->set_custom_minimum_size(Size2(0, get_theme_constant(SNAME("inspector_property_height"), EditorStringName(Editor))));
	environ_sky_color->set_custom_minimum_size(Size2(0, get_theme_constant(SNAME("inspector_property_height"), EditorStringName(Editor))));
	environ_ground_color->set_custom_minimum_size(Size2(0, get_theme_constant(SNAME("inspector_property_height"), EditorStringName(Editor))));

	context_toolbar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("ContextualToolbar"), EditorStringName(EditorStyles)));
	if (tool_column_panel) {
		EditorViewHeaderGroup::apply_style(tool_column_panel);
	}
	if (display_menu) {
		display_menu->set_button_icon(get_theme_icon(SNAME("arrow"), SNAME("OptionButton")));
	}
	if (overlays_menu) {
		overlays_menu->set_button_icon(get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
	}
	if (sidebar_button) {
		sidebar_button->set_button_icon(get_editor_theme_icon(SNAME("Tools")));
	}
	if (shading_buttons[0]) {
		// Drawn rather than loaded, in the theme's own colors.
		const Color ink = get_theme_color(SNAME("icon_normal_color"), SNAME("Button"));
		const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		const int size = get_editor_theme_icon(SNAME("ToolMove"))->get_width();
		for (int i = 0; i < Node3DEditorChrome::SHADING_MAX; i++) {
			shading_buttons[i]->set_button_icon(Node3DEditorChrome::make_shading_icon((Node3DEditorChrome::Shading)i, size, ink, accent));
		}
	}
}

void Node3DEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			const String show_list_tooltip = TTR("Alt+RMB: Show list of all nodes at position clicked, including locked.");
			tool_button[TOOL_MODE_TRANSFORM]->set_tooltip_text(vformat(TTR("%s+Drag: Rotate selected node around pivot."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)) + "\n" + show_list_tooltip);
			tool_button[TOOL_MODE_MOVE]->set_tooltip_text(vformat(TTR("%s+Drag: Use snap."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)) + "\n" + show_list_tooltip);
			tool_button[TOOL_MODE_ROTATE]->set_tooltip_text(vformat(TTR("%s+Drag: Use snap."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)) + "\n" + show_list_tooltip);
			tool_button[TOOL_MODE_SCALE]->set_tooltip_text(vformat(TTR("%s+Drag: Use snap."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)) + "\n" + show_list_tooltip);
			tool_button[TOOL_MODE_SELECT]->set_tooltip_text(show_list_tooltip);
			tool_button[TOOL_MODE_LIST_SELECT]->set_tooltip_text(TTR("Show list of selectable nodes at position clicked.") + "\n" + show_list_tooltip);
			tool_button[TOOL_RULER]->set_tooltip_text(TTR("LMB+Drag: Measure the distance between two points in 3D space.") + "\n" + TTR("Shift+LMB+Drag: Show component measurements.") + "\n" + show_list_tooltip);
			_update_gizmos_menu();
			_update_vertex_snap_tooltips();
		} break;

		case NOTIFICATION_READY: {
			_menu_item_pressed(MENU_VIEW_USE_1_VIEWPORT);

			_refresh_menu_icons();

			get_tree()->connect("node_removed", callable_mp(this, &Node3DEditor::_node_removed));
			get_tree()->connect("node_added", callable_mp(this, &Node3DEditor::_node_added));
			SceneTreeDock::get_singleton()->get_tree_editor()->connect("node_changed", callable_mp(this, &Node3DEditor::_refresh_menu_icons));
			editor_selection->connect("selection_changed", callable_mp(this, &Node3DEditor::_selection_changed));

			_update_preview_environment();

			sun_state->set_custom_minimum_size(sun_vb->get_combined_minimum_size());
			environ_state->set_custom_minimum_size(environ_vb->get_combined_minimum_size());

			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &Node3DEditor::update_all_gizmos).bind(Variant()));
		} break;

		case NOTIFICATION_ENTER_TREE: {
			_update_theme();
			if (scene_visuals_owner == this) {
				// The built-in plugins go into the shared set once; later views
				// pick them up from there.
				_register_all_gizmos();
			}
			// A view opened after registration has an empty Gizmos menu until it
			// is built from the set that is already there.
			_update_gizmos_menu();
			if (addon_mirror && primary_instance != this) {
				_queue_addon_mirror_rebuild();
			}
			_init_indicators();
			// Every open document, not only the one this view shows: a document
			// loaded before any view existed asked for gizmos when nothing was
			// listening, and a pane can be pointed at it.
			{
				EditorData &editor_data = EditorNode::get_editor_data();
				for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
					Node *document_root = editor_data.get_edited_scene_root(i);
					if (document_root) {
						update_all_gizmos(document_root);
					}
				}
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			// The shared grid and origin lines are not torn down here. Leaving the
			// tree no longer means going away: a pane that is split reparents its
			// view, and rebuilding shaders and meshes every time something moved
			// would be waste at best - at worst the grid would stay gone, since
			// update_grid() only rebuilds when the camera has travelled. They go
			// with the last view that is actually destroyed.
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			// Appearing only claims the editing context when nothing else holds
			// it, so revealing a second pane does not pull the context out of
			// the one being worked in; from there on a click decides. With a
			// single view open this is the same as claiming it unconditionally.
			if (is_visible_in_tree() && (!active_instance || !active_instance->is_visible_in_tree())) {
				make_active();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
			_update_gizmos_menu_theme();
			sun_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("title_font"), SNAME("Window")));
			environ_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("title_font"), SNAME("Window")));
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("editors/3d")) {
				const Color selection_box_color = EDITOR_GET("editors/3d/selection_box_color");
				const Color active_selection_box_color = EDITOR_GET("editors/3d/active_selection_box_color");

				if (selection_box_color != selection_box_mat->get_albedo()) {
					selection_box_mat->set_albedo(selection_box_color);
					selection_box_mat_xray->set_albedo(selection_box_color * Color(1, 1, 1, 0.15));
				}

				if (active_selection_box_color != active_selection_box_mat->get_albedo()) {
					active_selection_box_mat->set_albedo(active_selection_box_color);
					active_selection_box_mat_xray->set_albedo(active_selection_box_color * Color(1, 1, 1, 0.15));
				}

				gizmo_view_rotation_scale = GIZMO_CIRCLE_SIZE * (float)EDITOR_GET("editors/3d/view_plane_rotation_gizmo_scale");

				// Update grid color by rebuilding grid.
				_rebuild_all_grids();

				for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
					viewports[i]->update_transform_gizmo_view();
				}
				update_gizmo_opacity();
			}
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("editors/3d_gizmos/gizmo_settings")) {
				CollisionShape3DGizmoPlugin::set_show_only_when_selected(EDITOR_GET("editors/3d_gizmos/gizmo_settings/show_collision_shapes_only_when_selected"));
				update_all_gizmos();
			}
			_update_vertex_snap_tooltips();
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/inspector")) {
				snap_translate->set_step(EDITOR_GET("interface/inspector/default_float_step"));
			}
		} break;

		case NOTIFICATION_PHYSICS_PROCESS: {
			if (do_snap_selected_nodes_to_floor) {
				_snap_selected_nodes_to_floor();
				do_snap_selected_nodes_to_floor = false;
			}
		}
	}
}

bool Node3DEditor::is_subgizmo_selected(int p_id) {
	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;
	if (se) {
		return se->subgizmos.has(p_id);
	}
	return false;
}

bool Node3DEditor::is_current_selected_gizmo(const EditorNode3DGizmo *p_gizmo) {
	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;
	if (se) {
		return se->gizmo == p_gizmo;
	}
	return false;
}

Vector<int> Node3DEditor::get_subgizmo_selection() {
	Node3DEditorSelectedItem *se = selected ? editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected) : nullptr;

	Vector<int> ret;
	if (se) {
		for (const KeyValue<int, Transform3D> &E : se->subgizmos) {
			ret.push_back(E.key);
		}
	}
	return ret;
}

void Node3DEditor::clear_subgizmo_selection(Object *p_obj) {
	_clear_subgizmo_selection(p_obj);
}

void Node3DEditor::add_control_to_menu_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->add_control_to_menu_panel(p_control);
		return;
	}
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(p_control->get_parent());
	_queue_addon_mirror_rebuild();

	VSeparator *sep = memnew(VSeparator);
	context_toolbar_hbox->add_child(sep);
	context_toolbar_hbox->add_child(p_control);
	context_toolbar_separators[p_control] = sep;

	p_control->connect(SceneStringName(visibility_changed), callable_mp(this, &Node3DEditor::_update_context_toolbar));

	_update_context_toolbar();
}

void Node3DEditor::remove_control_from_menu_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->remove_control_from_menu_panel(p_control);
		return;
	}
	_queue_addon_mirror_rebuild();
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(p_control->get_parent() != context_toolbar_hbox);

	p_control->disconnect(SceneStringName(visibility_changed), callable_mp(this, &Node3DEditor::_update_context_toolbar));

	VSeparator *sep = context_toolbar_separators[p_control];
	context_toolbar_hbox->remove_child(sep);
	context_toolbar_hbox->remove_child(p_control);
	context_toolbar_separators.erase(p_control);
	memdelete(sep);

	_update_context_toolbar();
}

void Node3DEditor::_update_context_toolbar() {
	bool has_visible = false;
	bool first_visible = false;

	for (int i = 0; i < context_toolbar_hbox->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(context_toolbar_hbox->get_child(i));
		if (!child || !context_toolbar_separators.has(child)) {
			continue;
		}
		if (child->is_visible()) {
			first_visible = !has_visible;
			has_visible = true;
		}

		VSeparator *sep = context_toolbar_separators[child];
		sep->set_visible(!first_visible && child->is_visible());
	}

	context_toolbar_panel->set_visible(has_visible);
}

void Node3DEditor::set_can_preview(Camera3D *p_preview) {
	for (int i = 0; i < 4; i++) {
		viewports[i]->set_can_preview(p_preview);
	}

	viewports[last_used_viewport]->switch_preview_camera(p_preview);
}

VSplitContainer *Node3DEditor::get_shader_split() {
	// Where addons put their bottom panels: the view that stays.
	if (primary_instance && primary_instance != this) {
		return primary_instance->get_shader_split();
	}
	return shader_split;
}

Node3DEditorViewport *Node3DEditor::get_last_used_viewport() {
	return viewports[last_used_viewport];
}

void Node3DEditor::add_control_to_left_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->add_control_to_left_panel(p_control);
		return;
	}
	left_panel_split->add_child(p_control);
	left_panel_split->move_child(p_control, 0);
}

void Node3DEditor::add_control_to_right_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->add_control_to_right_panel(p_control);
		return;
	}
	right_panel_split->add_child(p_control);
	right_panel_split->move_child(p_control, 1);
}

void Node3DEditor::remove_control_from_left_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->remove_control_from_left_panel(p_control);
		return;
	}
	left_panel_split->remove_child(p_control);
}

void Node3DEditor::remove_control_from_right_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->remove_control_from_right_panel(p_control);
		return;
	}
	right_panel_split->remove_child(p_control);
}

void Node3DEditor::move_control_to_left_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->move_control_to_left_panel(p_control);
		return;
	}
	ERR_FAIL_NULL(p_control);
	if (p_control->get_parent() == left_panel_split) {
		return;
	}

	ERR_FAIL_COND(p_control->get_parent() != right_panel_split);
	right_panel_split->remove_child(p_control);

	add_control_to_left_panel(p_control);
}

void Node3DEditor::move_control_to_right_panel(Control *p_control) {
	if (primary_instance && primary_instance != this) {
		primary_instance->move_control_to_right_panel(p_control);
		return;
	}
	ERR_FAIL_NULL(p_control);
	if (p_control->get_parent() == right_panel_split) {
		return;
	}

	ERR_FAIL_COND(p_control->get_parent() != left_panel_split);
	left_panel_split->remove_child(p_control);

	add_control_to_right_panel(p_control);
}

void Node3DEditor::_request_gizmo(Object *p_obj) {
	Node3D *sp = Object::cast_to<Node3D>(p_obj);
	if (!sp) {
		return;
	}

	bool is_selected = (sp == selected);

	// A gizmo belongs to the node, not to a view: one view builds them for
	// everyone, so asking whether the node is in *its* document would leave
	// every other open document without any - and a pane showing one of those
	// with nothing to pick, select or drag.
	Node *document_root = EditorNode::get_editor_data().get_document_root_for(sp);
	if (document_root && (sp == document_root || sp->get_owner())) {
		for (int i = 0; i < gizmo_plugins_by_priority.size(); ++i) {
			Ref<EditorNode3DGizmo> seg = gizmo_plugins_by_priority.write[i]->get_gizmo(sp);

			if (seg.is_valid()) {
				sp->add_gizmo(seg);

				if (is_selected != seg->is_selected()) {
					seg->set_selected(is_selected);
				}
			}
		}
		if (!sp->get_gizmos().is_empty()) {
			sp->update_gizmos();
		}
	}
}

void Node3DEditor::_request_gizmo_for_id(ObjectID p_id) {
	Node3D *node = ObjectDB::get_instance<Node3D>(p_id);
	if (node) {
		_request_gizmo(node);
	}
}

void Node3DEditor::_set_subgizmo_selection(Object *p_obj, Ref<Node3DGizmo> p_gizmo, int p_id, Transform3D p_transform) {
	if (p_id == -1) {
		_clear_subgizmo_selection(p_obj);
		return;
	}

	Node3D *sp = nullptr;
	if (p_obj) {
		sp = Object::cast_to<Node3D>(p_obj);
	} else {
		sp = selected;
	}

	if (!sp) {
		return;
	}

	Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
	if (se) {
		se->subgizmos.clear();
		se->subgizmos.insert(p_id, p_transform);
		se->gizmo = p_gizmo;
		sp->update_gizmos();
		update_transform_gizmo();
	}
}

void Node3DEditor::_clear_subgizmo_selection(Object *p_obj) {
	Node3D *sp = nullptr;
	if (p_obj) {
		sp = Object::cast_to<Node3D>(p_obj);
	} else {
		sp = selected;
	}

	if (!sp) {
		return;
	}

	Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(sp);
	if (se) {
		se->subgizmos.clear();
		se->gizmo.unref();
		sp->update_gizmos();
		update_transform_gizmo();
	}
}

void Node3DEditor::_toggle_maximize_view(Object *p_viewport) {
	if (!p_viewport) {
		return;
	}
	Node3DEditorViewport *current_viewport = Object::cast_to<Node3DEditorViewport>(p_viewport);
	if (!current_viewport) {
		return;
	}

	int index = -1;
	bool maximized = false;
	for (int i = 0; i < 4; i++) {
		if (viewports[i] == current_viewport) {
			index = i;
			if (current_viewport->get_global_rect() == viewport_base->get_global_rect()) {
				maximized = true;
			}
			break;
		}
	}
	if (index == -1) {
		return;
	}

	if (!maximized) {
		for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
			if (i == (uint32_t)index) {
				viewports[i]->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			} else {
				viewports[i]->hide();
			}
		}
	} else {
		for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
			viewports[i]->show();
		}

		if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_1_VIEWPORT))) {
			_menu_item_pressed(MENU_VIEW_USE_1_VIEWPORT);
		} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS))) {
			_menu_item_pressed(MENU_VIEW_USE_2_VIEWPORTS);
		} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_2_VIEWPORTS_ALT))) {
			_menu_item_pressed(MENU_VIEW_USE_2_VIEWPORTS_ALT);
		} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS))) {
			_menu_item_pressed(MENU_VIEW_USE_3_VIEWPORTS);
		} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_3_VIEWPORTS_ALT))) {
			_menu_item_pressed(MENU_VIEW_USE_3_VIEWPORTS_ALT);
		} else if (view_layout_menu->get_popup()->is_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_USE_4_VIEWPORTS))) {
			_menu_item_pressed(MENU_VIEW_USE_4_VIEWPORTS);
		}
	}
}

void Node3DEditor::_viewport_clicked(int p_viewport_idx) {
	last_used_viewport = p_viewport_idx;
	// A click, never a hover: this brings the Scene tree, the Inspector and the
	// selection to the document this view shows, which is not something to do
	// because the mouse passed over it.
	make_active();
	EditorMainScreen *main_screen = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_main_screen() : nullptr;
	if (main_screen) {
		main_screen->view_activated(this);
	}
}

bool Node3DEditor::_is_preview_node_of_any_view(const Node *p_node) {
	for (const Node3DEditor *editor : instances) {
		if (p_node == editor->preview_sun || p_node == editor->preview_environment) {
			return true;
		}
	}
	return false;
}

void Node3DEditor::_node_added(Node *p_node) {
	if (_is_preview_node_of_any_view(p_node)) {
		// Preview nodes live in the document's root so they light and shade its
		// world, but they are not part of the scene and must not count towards
		// what disables the preview - whichever view they belong to.
		return;
	}
	if (get_scene_root()->is_ancestor_of(p_node)) {
		// Deferred because this fires while the scene root is still adding the
		// scene's children, and the update parents the preview nodes into it.
		if (Object::cast_to<WorldEnvironment>(p_node)) {
			world_env_count++;
			if (world_env_count == 1) {
				callable_mp(this, &Node3DEditor::_update_preview_environment).call_deferred();
			}
		} else if (Object::cast_to<DirectionalLight3D>(p_node)) {
			directional_light_count++;
			if (directional_light_count == 1) {
				callable_mp(this, &Node3DEditor::_update_preview_environment).call_deferred();
			}
		}
	}
}

void Node3DEditor::_node_removed(Node *p_node) {
	if (_is_preview_node_of_any_view(p_node)) {
		return;
	}
	if (get_scene_root()->is_ancestor_of(p_node)) {
		if (Object::cast_to<WorldEnvironment>(p_node)) {
			world_env_count--;
			if (world_env_count == 0) {
				callable_mp(this, &Node3DEditor::_update_preview_environment).call_deferred();
			}
		} else if (Object::cast_to<DirectionalLight3D>(p_node)) {
			directional_light_count--;
			if (directional_light_count == 0) {
				callable_mp(this, &Node3DEditor::_update_preview_environment).call_deferred();
			}
		}
	}

	if (p_node == selected) {
		Node3DEditorSelectedItem *se = editor_selection->get_node_editor_data<Node3DEditorSelectedItem>(selected);
		if (se) {
			se->gizmo.unref();
			se->subgizmos.clear();
		}
		selected = nullptr;
		update_transform_gizmo();
	}
}

void Node3DEditor::_register_all_gizmos() {
	// Once for the whole editor. This is called from entering the tree, and a
	// view enters the tree again whenever it is moved - into a pane, into a
	// window, between them. Registering again put every plugin in the shared
	// list twice, then three times, so every node grew a second and third set
	// of the same gizmos: handles drawn over themselves, and every click tested
	// against each copy.
	if (built_in_gizmos_registered) {
		return;
	}
	built_in_gizmos_registered = true;

	add_gizmo_plugin(Ref<Camera3DGizmoPlugin>(memnew(Camera3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Light3DGizmoPlugin>(memnew(Light3DGizmoPlugin)));
	add_gizmo_plugin(Ref<AudioStreamPlayer3DGizmoPlugin>(memnew(AudioStreamPlayer3DGizmoPlugin)));
	add_gizmo_plugin(Ref<AudioListener3DGizmoPlugin>(memnew(AudioListener3DGizmoPlugin)));
	add_gizmo_plugin(Ref<MeshInstance3DGizmoPlugin>(memnew(MeshInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<OccluderInstance3DGizmoPlugin>(memnew(OccluderInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SoftBody3DGizmoPlugin>(memnew(SoftBody3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpriteBase3DGizmoPlugin>(memnew(SpriteBase3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Label3DGizmoPlugin>(memnew(Label3DGizmoPlugin)));
	add_gizmo_plugin(Ref<GeometryInstance3DGizmoPlugin>(memnew(GeometryInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Marker3DGizmoPlugin>(memnew(Marker3DGizmoPlugin)));
	add_gizmo_plugin(Ref<RayCast3DGizmoPlugin>(memnew(RayCast3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ShapeCast3DGizmoPlugin>(memnew(ShapeCast3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpringArm3DGizmoPlugin>(memnew(SpringArm3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpringBoneCollision3DGizmoPlugin>(memnew(SpringBoneCollision3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpringBoneSimulator3DGizmoPlugin>(memnew(SpringBoneSimulator3DGizmoPlugin)));
	add_gizmo_plugin(Ref<VehicleWheel3DGizmoPlugin>(memnew(VehicleWheel3DGizmoPlugin)));
	add_gizmo_plugin(Ref<VisibleOnScreenNotifier3DGizmoPlugin>(memnew(VisibleOnScreenNotifier3DGizmoPlugin)));
	add_gizmo_plugin(Ref<GPUParticles3DGizmoPlugin>(memnew(GPUParticles3DGizmoPlugin)));
	add_gizmo_plugin(Ref<GPUParticlesCollision3DGizmoPlugin>(memnew(GPUParticlesCollision3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Particles3DEmissionShapeGizmoPlugin>(memnew(Particles3DEmissionShapeGizmoPlugin)));
	add_gizmo_plugin(Ref<CPUParticles3DGizmoPlugin>(memnew(CPUParticles3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ReflectionProbeGizmoPlugin>(memnew(ReflectionProbeGizmoPlugin)));
	add_gizmo_plugin(Ref<DecalGizmoPlugin>(memnew(DecalGizmoPlugin)));
	add_gizmo_plugin(Ref<VoxelGIGizmoPlugin>(memnew(VoxelGIGizmoPlugin)));
	add_gizmo_plugin(Ref<LightmapGIGizmoPlugin>(memnew(LightmapGIGizmoPlugin)));
	add_gizmo_plugin(Ref<LightmapProbeGizmoPlugin>(memnew(LightmapProbeGizmoPlugin)));
	add_gizmo_plugin(Ref<CollisionObject3DGizmoPlugin>(memnew(CollisionObject3DGizmoPlugin)));
	add_gizmo_plugin(Ref<CollisionShape3DGizmoPlugin>(memnew(CollisionShape3DGizmoPlugin)));
	add_gizmo_plugin(Ref<CollisionPolygon3DGizmoPlugin>(memnew(CollisionPolygon3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Joint3DGizmoPlugin>(memnew(Joint3DGizmoPlugin)));
	add_gizmo_plugin(Ref<PhysicalBone3DGizmoPlugin>(memnew(PhysicalBone3DGizmoPlugin)));
	add_gizmo_plugin(Ref<FogVolumeGizmoPlugin>(memnew(FogVolumeGizmoPlugin)));
	add_gizmo_plugin(Ref<TwoBoneIK3DGizmoPlugin>(memnew(TwoBoneIK3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ChainIK3DGizmoPlugin>(memnew(ChainIK3DGizmoPlugin)));
}

void Node3DEditor::_bind_methods() {
	ClassDB::bind_method("_get_editor_data", &Node3DEditor::_get_editor_data);
	ClassDB::bind_method("_request_gizmo", &Node3DEditor::_request_gizmo);
	ClassDB::bind_method("_request_gizmo_for_id", &Node3DEditor::_request_gizmo_for_id);
	ClassDB::bind_method("_set_subgizmo_selection", &Node3DEditor::_set_subgizmo_selection);
	ClassDB::bind_method("_clear_subgizmo_selection", &Node3DEditor::_clear_subgizmo_selection);
	ClassDB::bind_method("_refresh_menu_icons", &Node3DEditor::_refresh_menu_icons);
	ClassDB::bind_method("_preview_settings_changed", &Node3DEditor::_preview_settings_changed);

	ClassDB::bind_method("update_all_gizmos", &Node3DEditor::update_all_gizmos);
	ClassDB::bind_method("update_transform_gizmo", &Node3DEditor::update_transform_gizmo);

	ADD_SIGNAL(MethodInfo("transform_key_request"));
	ADD_SIGNAL(MethodInfo("item_lock_status_changed"));
	ADD_SIGNAL(MethodInfo("item_group_status_changed"));
}

void Node3DEditor::clear() {
	settings_fov->set_value(EDITOR_GET("editors/3d/default_fov"));
	settings_znear->set_value(EDITOR_GET("editors/3d/default_z_near"));
	settings_zfar->set_value(EDITOR_GET("editors/3d/default_z_far"));

	snap_translate_value = EditorSettings::get_singleton()->get_project_metadata("3d_editor", "snap_translate_value", 1);
	snap_rotate_value = EditorSettings::get_singleton()->get_project_metadata("3d_editor", "snap_rotate_value", 15);
	snap_scale_value = EditorSettings::get_singleton()->get_project_metadata("3d_editor", "snap_scale_value", 10);
	_snap_update();

	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->reset();
	}

	origin_enabled = true;
	_update_origin_visibility();

	view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_ORIGIN), true);
	for (int i = 0; i < 3; ++i) {
		if (grid_enable[i]) {
			grid_visible[i] = true;
		}
	}

	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->view_display_menu->get_popup()->set_item_checked(viewports[i]->view_display_menu->get_popup()->get_item_index(Node3DEditorViewport::VIEW_AUDIO_LISTENER), i == 0);
		viewports[i]->viewport->set_as_audio_listener_3d(i == 0);
	}

	view_layout_menu->get_popup()->set_item_checked(view_layout_menu->get_popup()->get_item_index(MENU_VIEW_GRID), true);
	grid_enabled = true;
	_rebuild_all_grids();
}

void Node3DEditor::_sun_direction_draw() {
	_ensure_preview_nodes();
	sun_direction->draw_rect(Rect2(Vector2(), sun_direction->get_size()), Color(1, 1, 1, 1));
	Vector3 z_axis = preview_sun->get_transform().basis.get_column(Vector3::AXIS_Z);
	z_axis = get_editor_viewport(0)->camera->get_camera_transform().basis.xform_inv(z_axis);
	sun_direction_material->set_shader_parameter("sun_direction", Vector3(z_axis.x, -z_axis.y, z_axis.z));
	Color color = sun_color->get_pick_color() * sun_energy->get_value();
	sun_direction_material->set_shader_parameter("sun_color", Vector3(color.r, color.g, color.b));
}

void Node3DEditor::_preview_settings_changed() {
	_ensure_preview_nodes();
	if (sun_environ_updating) {
		return;
	}

	{ // preview sun
		sun_rotation.x = Math::deg_to_rad(-sun_angle_altitude->get_value());
		sun_rotation.y = Math::deg_to_rad(180.0 - sun_angle_azimuth->get_value());
		Transform3D t;
		t.basis = Basis::from_euler(Vector3(sun_rotation.x, sun_rotation.y, 0));
		preview_sun->set_transform(t);
		sun_direction->queue_redraw();
		preview_sun->set_param(Light3D::PARAM_ENERGY, sun_energy->get_value());
		preview_sun->set_param(Light3D::PARAM_SHADOW_MAX_DISTANCE, sun_shadow_max_distance->get_value());
		preview_sun->set_color(sun_color->get_pick_color());
	}

	{ //preview env
		sky_material->set_energy_multiplier(environ_energy->get_value());
		Color hz_color = environ_sky_color->get_pick_color().lerp(environ_ground_color->get_pick_color(), 0.5);
		float hz_lum = hz_color.get_luminance() * 3.333;
		hz_color = hz_color.lerp(Color(hz_lum, hz_lum, hz_lum), 0.5);
		sky_material->set_sky_top_color(environ_sky_color->get_pick_color());
		sky_material->set_sky_horizon_color(hz_color);
		sky_material->set_ground_bottom_color(environ_ground_color->get_pick_color());
		sky_material->set_ground_horizon_color(hz_color);

		environment->set_ssao_enabled(environ_ao_button->is_pressed());
		environment->set_glow_enabled(environ_glow_button->is_pressed());
		environment->set_sdfgi_enabled(environ_gi_button->is_pressed());
		environment->set_tonemapper(environ_tonemap_button->is_pressed() ? Environment::TONE_MAPPER_FILMIC : Environment::TONE_MAPPER_LINEAR);
	}

	_share_preview_settings();
}

void Node3DEditor::_load_default_preview_settings() {
	sun_environ_updating = true;

	// These default rotations place the preview sun at an angular altitude
	// of 60 degrees (must be negative) and an azimuth of 30 degrees clockwise
	// from north (or 150 CCW from south), from north east, facing south west.
	// On any not-tidally-locked planet, a sun would have an angular altitude
	// of 60 degrees as the average of all points on the sphere at noon.
	// The azimuth choice is arbitrary, but ideally shouldn't be on an axis.
	sun_rotation = Vector2(-Math::deg_to_rad(60.0), Math::deg_to_rad(150.0));

	sun_angle_altitude->set_value_no_signal(-Math::rad_to_deg(sun_rotation.x));
	sun_angle_azimuth->set_value_no_signal(180.0 - Math::rad_to_deg(sun_rotation.y));
	sun_direction->queue_redraw();
	environ_sky_color->set_pick_color(Color(0.385, 0.454, 0.55));
	environ_ground_color->set_pick_color(Color(0.2, 0.169, 0.133));
	environ_energy->set_value_no_signal(1.0);
	if (OS::get_singleton()->get_current_rendering_method() != "gl_compatibility" && OS::get_singleton()->get_current_rendering_method() != "dummy") {
		environ_glow_button->set_pressed_no_signal(true);
	}
	environ_tonemap_button->set_pressed_no_signal(false);
	environ_ao_button->set_pressed_no_signal(false);
	environ_gi_button->set_pressed_no_signal(false);
	sun_shadow_max_distance->set_value_no_signal(100);

	sun_color->set_pick_color(Color(1, 1, 1));
	sun_energy->set_value_no_signal(1.0);

	sun_environ_updating = false;
}

void Node3DEditor::_drop_freed_preview_nodes() {
	// Whoever the preview nodes were parked in may have been freed since, which
	// leaves these pointers reading as garbage rather than as null.
	if (preview_sun && !ObjectDB::get_instance(preview_sun_id)) {
		preview_sun = nullptr;
		preview_sun_dangling = false;
	}
	if (preview_environment && !ObjectDB::get_instance(preview_environment_id)) {
		preview_environment = nullptr;
		preview_env_dangling = false;
	}
}

void Node3DEditor::_ensure_preview_nodes() {
	_drop_freed_preview_nodes();

	// The settings themselves live in this view's controls and in these
	// resources, so a rebuilt node picks up where the old one left off.
	if (environment.is_null()) {
		environment.instantiate();
		Ref<Sky> sky;
		sky.instantiate();
		sky_material.instantiate();
		sky->set_material(sky_material);
		environment->set_sky(sky);
		environment->set_background(Environment::BG_SKY);
	}

	bool rebuilt = false;

	if (!preview_sun) {
		preview_sun = memnew(DirectionalLight3D);
		preview_sun_id = preview_sun->get_instance_id();
		preview_sun->set_shadow(true);
		preview_sun->set_shadow_mode(DirectionalLight3D::SHADOW_PARALLEL_4_SPLITS);
		rebuilt = true;
	}

	if (!preview_environment) {
		preview_environment = memnew(WorldEnvironment);
		preview_environment_id = preview_environment->get_instance_id();
		preview_environment->set_environment(environment);
		if (GLOBAL_GET("rendering/lights_and_shadows/use_physical_light_units")) {
			if (camera_attributes.is_null()) {
				camera_attributes.instantiate();
			}
			preview_environment->set_camera_attributes(camera_attributes);
		}
		rebuilt = true;
	}

	// Only once the view is built: the constructor applies the settings itself,
	// after the controls they are read from exist.
	if (rebuilt && is_inside_tree()) {
		_preview_settings_changed();
	}
}

void Node3DEditor::_update_preview_environment() {
	_ensure_preview_nodes();

	// One view per world puts the preview in; the others show the same one.
	const bool owns = _claim_preview_owner();
	SubViewport *scene_root = get_scene_root();

	bool disable_light = directional_light_count > 0 || !sun_button->is_pressed();

	sun_button->set_disabled(directional_light_count > 0);

	const bool place_sun = !disable_light && owns && scene_root;
	if (!place_sun && preview_sun->get_parent()) {
		preview_sun->get_parent()->remove_child(preview_sun);
		preview_sun_dangling = true;
	}
	if (place_sun && !preview_sun->get_parent()) {
		// Into the scene root, so the preview lights the world the scene is
		// actually in rather than the editor window's.
		scene_root->add_child(preview_sun, true);
		preview_sun_dangling = false;
	}
	sun_state->set_visible(disable_light);
	sun_vb->set_visible(!disable_light);
	if (disable_light) {
		if (directional_light_count > 0) {
			sun_state->set_text(TTRC("Scene contains\nDirectionalLight3D.\nPreview disabled."));
		} else {
			sun_state->set_text(TTRC("Preview disabled."));
		}
	}

	sun_angle_altitude->set_value_no_signal(-Math::rad_to_deg(sun_rotation.x));
	sun_angle_azimuth->set_value_no_signal(180.0 - Math::rad_to_deg(sun_rotation.y));

	bool disable_env = world_env_count > 0 || !environ_button->is_pressed();

	environ_button->set_disabled(world_env_count > 0);

	const bool place_env = !disable_env && owns && scene_root;
	if (!place_env && preview_environment->get_parent()) {
		preview_environment->get_parent()->remove_child(preview_environment);
		preview_env_dangling = true;
	}
	if (place_env && !preview_environment->get_parent()) {
		scene_root->add_child(preview_environment);
		preview_env_dangling = false;
	}
	environ_state->set_visible(disable_env);
	environ_vb->set_visible(!disable_env);
	if (disable_env) {
		if (world_env_count > 0) {
			environ_state->set_text(TTRC("Scene contains\nWorldEnvironment.\nPreview disabled."));
		} else {
			environ_state->set_text(TTRC("Preview disabled."));
		}
	}
}

void Node3DEditor::_sun_direction_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && mm->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
		sun_rotation.x += mm->get_relative().y * (0.02 * EDSCALE);
		sun_rotation.y -= mm->get_relative().x * (0.02 * EDSCALE);
		sun_rotation.x = CLAMP(sun_rotation.x, -Math::TAU / 4, Math::TAU / 4);

		EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
		undo_redo->create_action(TTR("Set Preview Sun Direction"), UndoRedo::MergeMode::MERGE_ENDS);
		undo_redo->add_do_method(sun_angle_altitude, "set_value_no_signal", -Math::rad_to_deg(sun_rotation.x));
		undo_redo->add_undo_method(sun_angle_altitude, "set_value_no_signal", sun_angle_altitude->get_value());
		undo_redo->add_do_method(sun_angle_azimuth, "set_value_no_signal", 180.0 - Math::rad_to_deg(sun_rotation.y));
		undo_redo->add_undo_method(sun_angle_azimuth, "set_value_no_signal", sun_angle_azimuth->get_value());
		undo_redo->add_do_method(this, "_preview_settings_changed");
		undo_redo->add_undo_method(this, "_preview_settings_changed");
		undo_redo->commit_action();
	}
}

void Node3DEditor::_sun_direction_set_altitude(float p_altitude) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Sun Altitude"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(sun_angle_altitude, "set_value_no_signal", p_altitude);
	undo_redo->add_undo_method(sun_angle_altitude, "set_value_no_signal", -Math::rad_to_deg(sun_rotation.x));
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_sun_direction_set_azimuth(float p_azimuth) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Sun Azimuth"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(sun_angle_azimuth, "set_value_no_signal", p_azimuth);
	undo_redo->add_undo_method(sun_angle_azimuth, "set_value_no_signal", 180.0 - Math::rad_to_deg(sun_rotation.y));
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_sun_set_color(const Color &p_color) {
	_ensure_preview_nodes();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Sun Color"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(sun_color, "set_pick_color", p_color);
	undo_redo->add_undo_method(sun_color, "set_pick_color", preview_sun->get_color());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_sun_set_energy(float p_energy) {
	_ensure_preview_nodes();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Sun Energy"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(sun_energy, "set_value_no_signal", p_energy);
	undo_redo->add_undo_method(sun_energy, "set_value_no_signal", preview_sun->get_param(Light3D::PARAM_ENERGY));
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_sun_set_shadow_max_distance(float p_shadow_max_distance) {
	_ensure_preview_nodes();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Sun Max Shadow Distance"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(sun_shadow_max_distance, "set_value_no_signal", p_shadow_max_distance);
	undo_redo->add_undo_method(sun_shadow_max_distance, "set_value_no_signal", preview_sun->get_param(Light3D::PARAM_SHADOW_MAX_DISTANCE));
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_sky_color(const Color &p_color) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Sky Color"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(environ_sky_color, "set_pick_color", p_color);
	undo_redo->add_undo_method(environ_sky_color, "set_pick_color", sky_material->get_sky_top_color());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_ground_color(const Color &p_color) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Ground Color"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(environ_ground_color, "set_pick_color", p_color);
	undo_redo->add_undo_method(environ_ground_color, "set_pick_color", sky_material->get_ground_bottom_color());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_sky_energy(float p_energy) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Energy"), UndoRedo::MergeMode::MERGE_ENDS);
	undo_redo->add_do_method(environ_energy, "set_value_no_signal", p_energy);
	undo_redo->add_undo_method(environ_energy, "set_value_no_signal", sky_material->get_energy_multiplier());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_ao() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Ambient Occlusion"));
	undo_redo->add_do_method(environ_ao_button, "set_pressed", environ_ao_button->is_pressed());
	undo_redo->add_undo_method(environ_ao_button, "set_pressed", !environ_ao_button->is_pressed());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_glow() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Glow"));
	undo_redo->add_do_method(environ_glow_button, "set_pressed", environ_glow_button->is_pressed());
	undo_redo->add_undo_method(environ_glow_button, "set_pressed", !environ_glow_button->is_pressed());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_tonemap() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Tonemap"));
	undo_redo->add_do_method(environ_tonemap_button, "set_pressed", environ_tonemap_button->is_pressed());
	undo_redo->add_undo_method(environ_tonemap_button, "set_pressed", !environ_tonemap_button->is_pressed());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::_environ_set_gi() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Preview Environment Global Illumination"));
	undo_redo->add_do_method(environ_gi_button, "set_pressed", environ_gi_button->is_pressed());
	undo_redo->add_undo_method(environ_gi_button, "set_pressed", !environ_gi_button->is_pressed());
	undo_redo->add_do_method(this, "_preview_settings_changed");
	undo_redo->add_undo_method(this, "_preview_settings_changed");
	undo_redo->commit_action();
}

void Node3DEditor::PreviewSunEnvPopup::shortcut_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		bool handled = false;

		if (ED_IS_SHORTCUT("ui_undo", p_event)) {
			EditorNode::get_singleton()->undo();
			handled = true;
		}

		if (ED_IS_SHORTCUT("ui_redo", p_event)) {
			EditorNode::get_singleton()->redo();
			handled = true;
		}

		if (handled) {
			set_input_as_handled();
		}
	}
}

Node3DEditor::Node3DEditor() {
	gizmo.visible = true;
	gizmo.scale = 1.0;
	gizmo_view_rotation_scale = GIZMO_CIRCLE_SIZE * (float)EDITOR_GET("editors/3d/view_plane_rotation_gizmo_scale");

	viewport_environment.instantiate();
	VBoxContainer *vbc = this;

	instances.push_back(this);
	if (active_instance == nullptr) {
		active_instance = this;
	}
	editor_selection = EditorNode::get_singleton()->get_editor_selection();
	editor_selection->add_editor_plugin(this);

	MarginContainer *toolbar_margin = memnew(MarginContainer);
	toolbar_margin->set_theme_type_variation("MainToolBarMargin");
	vbc->add_child(toolbar_margin);

	// A fluid container for all toolbars.
	HFlowContainer *main_flow = memnew(HFlowContainer);
	toolbar_margin->add_child(main_flow);
	toolbar_flow = main_flow;

	// Main toolbars. Each group is a row of its own in the flow container, so
	// the toolbar wraps when there is no room instead of making the whole view
	// as wide as every button laid end to end - which is what stopped two panes
	// from being resized against each other.
	HBoxContainer *main_menu_hbox = memnew(HBoxContainer);
	main_flow->add_child(main_menu_hbox);

	String sct;

	tool_button[TOOL_MODE_TRANSFORM] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_TRANSFORM]);
	tool_button[TOOL_MODE_TRANSFORM]->set_toggle_mode(true);
	tool_button[TOOL_MODE_TRANSFORM]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_MODE_TRANSFORM]->set_pressed(true);
	tool_button[TOOL_MODE_TRANSFORM]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_TRANSFORM));
	tool_button[TOOL_MODE_TRANSFORM]->set_shortcut(ED_SHORTCUT("spatial_editor/tool_transform", TTRC("Transform Mode"), Key::Q, true));
	tool_button[TOOL_MODE_TRANSFORM]->set_shortcut_context(this);
	tool_button[TOOL_MODE_TRANSFORM]->set_accessibility_name(TTRC("Transform Mode"));

	tool_button[TOOL_MODE_MOVE] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_MOVE]);
	tool_button[TOOL_MODE_MOVE]->set_toggle_mode(true);
	tool_button[TOOL_MODE_MOVE]->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tool_button[TOOL_MODE_MOVE]->set_theme_type_variation(SceneStringName(FlatButton));

	tool_button[TOOL_MODE_MOVE]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_MOVE));
	tool_button[TOOL_MODE_MOVE]->set_shortcut(ED_SHORTCUT("spatial_editor/tool_move", TTRC("Move Mode"), Key::W, true));
	tool_button[TOOL_MODE_MOVE]->set_shortcut_context(this);
	tool_button[TOOL_MODE_MOVE]->set_accessibility_name(TTRC("Move Mode"));

	tool_button[TOOL_MODE_ROTATE] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_ROTATE]);
	tool_button[TOOL_MODE_ROTATE]->set_toggle_mode(true);
	tool_button[TOOL_MODE_ROTATE]->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tool_button[TOOL_MODE_ROTATE]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_MODE_ROTATE]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_ROTATE));
	tool_button[TOOL_MODE_ROTATE]->set_shortcut(ED_SHORTCUT("spatial_editor/tool_rotate", TTRC("Rotate Mode"), Key::E, true));
	tool_button[TOOL_MODE_ROTATE]->set_shortcut_context(this);
	tool_button[TOOL_MODE_ROTATE]->set_accessibility_name(TTRC("Rotate Mode"));

	tool_button[TOOL_MODE_SCALE] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_SCALE]);
	tool_button[TOOL_MODE_SCALE]->set_toggle_mode(true);
	tool_button[TOOL_MODE_SCALE]->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tool_button[TOOL_MODE_SCALE]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_MODE_SCALE]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_SCALE));
	tool_button[TOOL_MODE_SCALE]->set_shortcut(ED_SHORTCUT("spatial_editor/tool_scale", TTRC("Scale Mode"), Key::R, true));
	tool_button[TOOL_MODE_SCALE]->set_shortcut_context(this);
	tool_button[TOOL_MODE_SCALE]->set_accessibility_name(TTRC("Scale Mode"));

	tool_button[TOOL_MODE_SELECT] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_SELECT]);
	tool_button[TOOL_MODE_SELECT]->set_toggle_mode(true);
	tool_button[TOOL_MODE_SELECT]->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tool_button[TOOL_MODE_SELECT]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_MODE_SELECT]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_SELECT));
	tool_button[TOOL_MODE_SELECT]->set_shortcut(ED_SHORTCUT("spatial_editor/tool_select", TTRC("Select Mode"), Key::V, true));
	tool_button[TOOL_MODE_SELECT]->set_shortcut_context(this);
	tool_button[TOOL_MODE_SELECT]->set_accessibility_name(TTRC("Select Mode"));

	main_menu_hbox->add_child(memnew(VSeparator));
	main_menu_hbox = memnew(HBoxContainer);
	main_flow->add_child(main_menu_hbox);

	tool_button[TOOL_MODE_LIST_SELECT] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_MODE_LIST_SELECT]);
	tool_button[TOOL_MODE_LIST_SELECT]->set_toggle_mode(true);
	tool_button[TOOL_MODE_LIST_SELECT]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_MODE_LIST_SELECT]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_TOOL_LIST_SELECT));
	tool_button[TOOL_MODE_LIST_SELECT]->set_tooltip_text(TTR("Show list of selectable nodes at position clicked.") + "\n" + TTR("Alt+RMB: Show list of all nodes at position clicked, including locked."));
	tool_button[TOOL_MODE_LIST_SELECT]->set_accessibility_name(TTRC("Show List of Selectable Nodes"));

	tool_button[TOOL_LOCK_SELECTED] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_LOCK_SELECTED]);
	tool_button[TOOL_LOCK_SELECTED]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_LOCK_SELECTED]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_LOCK_SELECTED));
	tool_button[TOOL_LOCK_SELECTED]->set_tooltip_text(TTRC("Lock selected node, preventing selection and movement."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	tool_button[TOOL_LOCK_SELECTED]->set_shortcut(ED_GET_SHORTCUT("editor/lock_selected_nodes"));
	tool_button[TOOL_LOCK_SELECTED]->set_accessibility_name(TTRC("Lock"));

	tool_button[TOOL_UNLOCK_SELECTED] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_UNLOCK_SELECTED]);
	tool_button[TOOL_UNLOCK_SELECTED]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_UNLOCK_SELECTED]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_UNLOCK_SELECTED));
	tool_button[TOOL_UNLOCK_SELECTED]->set_tooltip_text(TTRC("Unlock selected node, allowing selection and movement."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	tool_button[TOOL_UNLOCK_SELECTED]->set_shortcut(ED_GET_SHORTCUT("editor/unlock_selected_nodes"));
	tool_button[TOOL_UNLOCK_SELECTED]->set_accessibility_name(TTRC("Unlock"));

	tool_button[TOOL_GROUP_SELECTED] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_GROUP_SELECTED]);
	tool_button[TOOL_GROUP_SELECTED]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_GROUP_SELECTED]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_GROUP_SELECTED));
	tool_button[TOOL_GROUP_SELECTED]->set_tooltip_text(TTRC("Groups the selected node with its children. This selects the parent when any child node is clicked in 2D and 3D view."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	tool_button[TOOL_GROUP_SELECTED]->set_shortcut(ED_GET_SHORTCUT("editor/group_selected_nodes"));
	tool_button[TOOL_GROUP_SELECTED]->set_accessibility_name(TTRC("Group"));

	tool_button[TOOL_UNGROUP_SELECTED] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_UNGROUP_SELECTED]);
	tool_button[TOOL_UNGROUP_SELECTED]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button[TOOL_UNGROUP_SELECTED]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_UNGROUP_SELECTED));
	tool_button[TOOL_UNGROUP_SELECTED]->set_tooltip_text(TTRC("Ungroups the selected node from its children. Child nodes will be individual items in 2D and 3D view."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	tool_button[TOOL_UNGROUP_SELECTED]->set_shortcut(ED_GET_SHORTCUT("editor/ungroup_selected_nodes"));
	tool_button[TOOL_UNGROUP_SELECTED]->set_accessibility_name(TTRC("Ungroup"));

	tool_button[TOOL_RULER] = memnew(Button);
	main_menu_hbox->add_child(tool_button[TOOL_RULER]);
	tool_button[TOOL_RULER]->set_toggle_mode(true);
	tool_button[TOOL_RULER]->set_theme_type_variation("FlatButton");
	tool_button[TOOL_RULER]->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_menu_item_activated).bind(MENU_RULER));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	tool_button[TOOL_RULER]->set_shortcut(ED_SHORTCUT("spatial_editor/measure", TTRC("Ruler Mode"), Key::M));
	tool_button[TOOL_RULER]->set_accessibility_name(TTRC("Ruler Mode"));

	main_menu_hbox->add_child(memnew(VSeparator));
	main_menu_hbox = memnew(HBoxContainer);
	main_flow->add_child(main_menu_hbox);

	tool_option_button[TOOL_OPT_LOCAL_COORDS] = memnew(Button);
	main_menu_hbox->add_child(tool_option_button[TOOL_OPT_LOCAL_COORDS]);
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_toggle_mode(true);
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditor::_menu_item_toggled).bind(MENU_TOOL_LOCAL_COORDS));
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_shortcut(ED_SHORTCUT("spatial_editor/local_coords", TTRC("Use Local Space"), Key::T));
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_shortcut_context(this);
	tool_option_button[TOOL_OPT_LOCAL_COORDS]->set_accessibility_name(TTRC("Use Local Space"));

	tool_option_button[TOOL_OPT_USE_SNAP] = memnew(Button);
	main_menu_hbox->add_child(tool_option_button[TOOL_OPT_USE_SNAP]);
	tool_option_button[TOOL_OPT_USE_SNAP]->set_toggle_mode(true);
	tool_option_button[TOOL_OPT_USE_SNAP]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_option_button[TOOL_OPT_USE_SNAP]->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditor::_menu_item_toggled).bind(MENU_TOOL_USE_SNAP));
	tool_option_button[TOOL_OPT_USE_SNAP]->set_shortcut(ED_SHORTCUT("spatial_editor/snap", TTRC("Use Snap"), Key::Y));
	tool_option_button[TOOL_OPT_USE_SNAP]->set_shortcut_context(this);
	tool_option_button[TOOL_OPT_USE_SNAP]->set_accessibility_name(TTRC("Use Snap"));

	tool_option_button[TOOL_OPT_USE_TRACKBALL] = memnew(Button);
	main_menu_hbox->add_child(tool_option_button[TOOL_OPT_USE_TRACKBALL]);
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_toggle_mode(true);
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditor::_menu_item_toggled).bind(MENU_TOOL_USE_TRACKBALL));
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_shortcut(ED_SHORTCUT("spatial_editor/trackball", TTRC("Use Trackball"), Key::U));
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_shortcut_context(this);
	tool_option_button[TOOL_OPT_USE_TRACKBALL]->set_accessibility_name(TTRC("Use Trackball"));

	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM] = memnew(Button);
	main_menu_hbox->add_child(tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]);
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_toggle_mode(true);
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_theme_type_variation(SceneStringName(FlatButton));
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditor::_menu_item_toggled).bind(MENU_TOOL_PRESERVE_CHILDREN_TRANSFORM));
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_shortcut(ED_SHORTCUT("spatial_editor/preserve_children_transform", TTRC("Preserve Children Transform"), Key::P));
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_shortcut_context(this);
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_accessibility_name(TTRC("Preserve Children Transform"));
	tool_option_button[TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->set_tooltip_text(TTRC("When enabled, transforming a node will preserve the global transform of its children.\nThis also applies when editing transform properties in the Inspector."));

	main_menu_hbox->add_child(memnew(VSeparator));
	main_menu_hbox = memnew(HBoxContainer);
	main_flow->add_child(main_menu_hbox);
	sun_button = memnew(Button);
	sun_button->set_tooltip_text(TTRC("Toggle preview sunlight.\nIf a DirectionalLight3D node is added to the scene, preview sunlight is disabled."));
	sun_button->set_toggle_mode(true);
	sun_button->set_accessibility_name(TTRC("Toggle preview sunlight."));
	sun_button->set_theme_type_variation(SceneStringName(FlatButton));
	sun_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_update_preview_environment), CONNECT_DEFERRED);
	// Preview is enabled by default - ensure this applies on editor startup when there is no state yet.
	sun_button->set_pressed(true);

	main_menu_hbox->add_child(sun_button);

	environ_button = memnew(Button);
	environ_button->set_tooltip_text(TTRC("Toggle preview environment.\nIf a WorldEnvironment node is added to the scene, preview environment is disabled."));
	environ_button->set_toggle_mode(true);
	environ_button->set_accessibility_name(TTRC("Toggle preview environment."));
	environ_button->set_theme_type_variation(SceneStringName(FlatButton));
	environ_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_update_preview_environment), CONNECT_DEFERRED);
	// Preview is enabled by default - ensure this applies on editor startup when there is no state yet.
	environ_button->set_pressed(true);

	main_menu_hbox->add_child(environ_button);

	sun_environ_settings = memnew(Button);
	sun_environ_settings->set_tooltip_text(TTRC("Edit Sun and Environment settings."));
	sun_environ_settings->set_theme_type_variation(SceneStringName(FlatButton));
	sun_environ_settings->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_sun_environ_settings_pressed));

	main_menu_hbox->add_child(sun_environ_settings);

	main_menu_hbox->add_child(memnew(VSeparator));
	main_menu_hbox = memnew(HBoxContainer);
	main_flow->add_child(main_menu_hbox);

	// Drag and drop support;
	preview_node = memnew(Node3D);
	preview_bounds = AABB();

	ED_SHORTCUT("spatial_editor/bottom_view", TTRC("Bottom View"), KeyModifierMask::ALT + Key::KP_7);
	ED_SHORTCUT("spatial_editor/top_view", TTRC("Top View"), Key::KP_7);
	ED_SHORTCUT("spatial_editor/rear_view", TTRC("Rear View"), KeyModifierMask::ALT + Key::KP_1);
	ED_SHORTCUT("spatial_editor/front_view", TTRC("Front View"), Key::KP_1);
	ED_SHORTCUT("spatial_editor/left_view", TTRC("Left View"), KeyModifierMask::ALT + Key::KP_3);
	ED_SHORTCUT("spatial_editor/right_view", TTRC("Right View"), Key::KP_3);
	ED_SHORTCUT("spatial_editor/orbit_view_down", TTRC("Orbit View Down"), Key::KP_2);
	ED_SHORTCUT("spatial_editor/orbit_view_left", TTRC("Orbit View Left"), Key::KP_4);
	ED_SHORTCUT("spatial_editor/orbit_view_right", TTRC("Orbit View Right"), Key::KP_6);
	ED_SHORTCUT("spatial_editor/orbit_view_up", TTRC("Orbit View Up"), Key::KP_8);
	ED_SHORTCUT("spatial_editor/orbit_view_180", TTRC("Orbit View 180"), Key::KP_9);
	ED_SHORTCUT("spatial_editor/switch_perspective_orthogonal", TTRC("Switch Perspective/Orthogonal View"), Key::KP_5);
	ED_SHORTCUT("spatial_editor/insert_anim_key", TTRC("Insert Animation Key"), Key::K);
	ED_SHORTCUT("spatial_editor/focus_origin", TTRC("Focus Origin"), Key::O);
	ED_SHORTCUT("spatial_editor/focus_selection", TTRC("Focus Selection"), Key::F);
	ED_SHORTCUT_ARRAY("spatial_editor/align_transform_with_view", TTRC("Align Transform with View"),
			{ int32_t(KeyModifierMask::ALT | KeyModifierMask::CTRL | Key::KP_0),
					int32_t(KeyModifierMask::ALT | KeyModifierMask::CTRL | Key::M),
					int32_t(KeyModifierMask::ALT | KeyModifierMask::CTRL | Key::G) });
	ED_SHORTCUT_OVERRIDE_ARRAY("spatial_editor/align_transform_with_view", "macos",
			{ int32_t(KeyModifierMask::ALT | KeyModifierMask::META | Key::KP_0),
					int32_t(KeyModifierMask::ALT | KeyModifierMask::META | Key::G) });
	ED_SHORTCUT("spatial_editor/align_rotation_with_view", TTRC("Align Rotation with View"), KeyModifierMask::ALT + KeyModifierMask::CMD_OR_CTRL + Key::F);
	ED_SHORTCUT("spatial_editor/freelook_toggle", TTRC("Toggle Freelook"), KeyModifierMask::SHIFT + Key::F);
	ED_SHORTCUT("spatial_editor/decrease_fov", TTRC("Decrease Field of View"), KeyModifierMask::CMD_OR_CTRL + Key::EQUAL); // Usually direct access key for `KEY_PLUS`.
	ED_SHORTCUT("spatial_editor/increase_fov", TTRC("Increase Field of View"), KeyModifierMask::CMD_OR_CTRL + Key::MINUS);
	ED_SHORTCUT("spatial_editor/reset_fov", TTRC("Reset Field of View to Default"), KeyModifierMask::CMD_OR_CTRL + Key::KEY_0);

	PopupMenu *p;

	transform_menu = memnew(MenuButton);
	transform_menu->set_flat(false);
	transform_menu->set_theme_type_variation("FlatMenuButton");
	transform_menu->set_text(TTRC("Transform"));
	transform_menu->set_switch_on_hover(true);
	transform_menu->set_shortcut_context(this);
	main_menu_hbox->add_child(transform_menu);

	p = transform_menu->get_popup();
	p->add_shortcut(ED_SHORTCUT("spatial_editor/snap_to_floor", TTRC("Snap Object to Floor"), Key::PAGEDOWN), MENU_SNAP_TO_FLOOR);
	p->add_shortcut(ED_SHORTCUT("spatial_editor/transform_dialog", TTRC("Transform Dialog...")), MENU_TRANSFORM_DIALOG);

	p->add_separator();
	ED_SHORTCUT("spatial_editor/vertex_snap", TTRC("Vertex Snap"), Key::B);
	p->add_radio_check_item(TTRC("Snap Vertex to Vertex"), MENU_VERTEX_SNAP_BASE_VERTEX);
	p->set_item_checked(p->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX), true);
	p->add_radio_check_item(TTRC("Snap Origin to Vertex"), MENU_VERTEX_SNAP_BASE_ORIGIN);

	p->add_separator();
	p->add_radio_check_item(TTRC("Snap to Mesh Vertices"), MENU_VERTEX_SNAP_SOURCE_MESH);
	p->set_item_checked(p->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH), true);
	p->add_radio_check_item(TTRC("Snap to Collision Vertices"), MENU_VERTEX_SNAP_SOURCE_COLLISION);
	_update_vertex_snap_tooltips();

	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("spatial_editor/configure_snap", TTRC("Configure Snap...")), MENU_TRANSFORM_CONFIGURE_SNAP);

	p->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::_menu_item_activated));

	view_layout_menu = memnew(MenuButton);
	view_layout_menu->set_flat(false);
	view_layout_menu->set_theme_type_variation("FlatMenuButton");
	// TRANSLATORS: Noun, name of the 2D/3D View menus.
	view_layout_menu->set_text(TTRC("View"));
	view_layout_menu->set_switch_on_hover(true);
	view_layout_menu->set_shortcut_context(this);
	main_menu_hbox->add_child(view_layout_menu);

	main_menu_hbox->add_child(memnew(VSeparator));

	context_toolbar_panel = memnew(PanelContainer);
	context_toolbar_hbox = memnew(HBoxContainer);
	context_toolbar_panel->add_child(context_toolbar_hbox);
	main_flow->add_child(context_toolbar_panel);

	// Get the view menu popup and have it stay open when a checkable item is selected
	p = view_layout_menu->get_popup();
	p->set_hide_on_checkable_item_selection(false);

	accept = memnew(AcceptDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(accept);

	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/1_viewport", TTRC("1 Viewport"), KeyModifierMask::CMD_OR_CTRL + Key::KEY_1, true), MENU_VIEW_USE_1_VIEWPORT);
	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/2_viewports", TTRC("2 Viewports"), KeyModifierMask::CMD_OR_CTRL + Key::KEY_2, true), MENU_VIEW_USE_2_VIEWPORTS);
	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/2_viewports_alt", TTRC("2 Viewports (Alt)"), KeyModifierMask::ALT + KeyModifierMask::CMD_OR_CTRL + Key::KEY_2, true), MENU_VIEW_USE_2_VIEWPORTS_ALT);
	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/3_viewports", TTRC("3 Viewports"), KeyModifierMask::CMD_OR_CTRL + Key::KEY_3, true), MENU_VIEW_USE_3_VIEWPORTS);
	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/3_viewports_alt", TTRC("3 Viewports (Alt)"), KeyModifierMask::ALT + KeyModifierMask::CMD_OR_CTRL + Key::KEY_3, true), MENU_VIEW_USE_3_VIEWPORTS_ALT);
	p->add_radio_check_shortcut(ED_SHORTCUT("spatial_editor/4_viewports", TTRC("4 Viewports"), KeyModifierMask::CMD_OR_CTRL + Key::KEY_4, true), MENU_VIEW_USE_4_VIEWPORTS);

	p->add_separator();

	gizmos_menu = memnew(PopupMenu);
	gizmos_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	gizmos_menu->set_hide_on_checkable_item_selection(false);
	p->add_submenu_node_item(TTRC("Gizmos"), gizmos_menu);
	gizmos_menu->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::_menu_gizmo_toggled));

	p->add_separator();
	p->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_origin", TTRC("View Origin")), MENU_VIEW_ORIGIN);
	p->add_check_shortcut(ED_SHORTCUT("spatial_editor/view_grid", TTRC("View Grid"), Key::NUMBERSIGN), MENU_VIEW_GRID);

	p->add_separator();
	p->add_submenu_node_item(TTRC("Preview Translation"), memnew(EditorTranslationPreviewMenu));

	p->add_separator();
	p->add_item(TTRC("Create Camera from View"), MENU_VIEW_CAMERA_SNAPSHOT);
	p->set_item_tooltip(p->get_item_index(MENU_VIEW_CAMERA_SNAPSHOT), TTRC("Spawns a Camera3D from the current editor view."));

	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("spatial_editor/settings", TTRC("Settings...")), MENU_VIEW_CAMERA_SETTINGS);

	p->set_item_checked(p->get_item_index(MENU_VIEW_ORIGIN), true);
	p->set_item_checked(p->get_item_index(MENU_VIEW_GRID), true);

	p->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::_menu_item_activated));

	/* REST OF MENU */

	left_panel_split = memnew(HSplitContainer);
	left_panel_split->set_v_size_flags(SIZE_EXPAND_FILL);
	vbc->add_child(left_panel_split);

	right_panel_split = memnew(HSplitContainer);
	right_panel_split->set_v_size_flags(SIZE_EXPAND_FILL);
	left_panel_split->add_child(right_panel_split);

	shader_split = memnew(VSplitContainer);
	shader_split->set_h_size_flags(SIZE_EXPAND_FILL);
	right_panel_split->add_child(shader_split);
	viewport_base = memnew(Node3DEditorViewportContainer);
	shader_split->add_child(viewport_base);
	viewport_base->set_v_size_flags(SIZE_EXPAND_FILL);
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i] = memnew(Node3DEditorViewport(this, i));
		viewports[i]->connect("toggle_maximize_view", callable_mp(this, &Node3DEditor::_toggle_maximize_view));
		viewports[i]->connect("clicked", callable_mp(this, &Node3DEditor::_viewport_clicked).bind(i));
		viewports[i]->assign_pending_data_pointers(preview_node, &preview_bounds, accept);
		viewports[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		viewports[i]->set_v_size_flags(SIZE_EXPAND_FILL);
		viewports[i]->set_custom_minimum_size(Size2(39, 39));
		viewport_base->add_viewport(viewports[i], i);
	}

	/* SNAP DIALOG */

	snap_dialog = memnew(ConfirmationDialog);
	snap_dialog->set_title(TTRC("Snap Settings"));
	add_child(snap_dialog);
	snap_dialog->connect(SceneStringName(confirmed), callable_mp(this, &Node3DEditor::_snap_changed));
	snap_dialog->get_cancel_button()->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_snap_update));

	VBoxContainer *snap_dialog_vbc = memnew(VBoxContainer);
	snap_dialog->add_child(snap_dialog_vbc);

	snap_translate = memnew(EditorSpinSlider);
	snap_translate->set_min(0.0);
	snap_translate->set_step(EDITOR_GET("interface/inspector/default_float_step"));
	snap_translate->set_max(10.0);
	snap_translate->set_suffix("m");
	snap_translate->set_allow_greater(true);
	snap_translate->set_accessibility_name(TTRC("Translate Snap"));
	snap_dialog_vbc->add_margin_child(TTR("Translate Snap:"), snap_translate);

	snap_rotate = memnew(EditorSpinSlider);
	snap_rotate->set_min(0.0);
	snap_rotate->set_step(0.1);
	snap_rotate->set_max(360);
	snap_rotate->set_suffix(U"°");
	snap_rotate->set_accessibility_name(TTRC("Rotate Snap"));
	snap_dialog_vbc->add_margin_child(TTR("Rotate Snap:"), snap_rotate);

	snap_scale = memnew(EditorSpinSlider);
	snap_scale->set_min(0.0);
	snap_scale->set_step(1.0);
	snap_scale->set_max(100);
	snap_scale->set_suffix("%");
	snap_scale->set_accessibility_name(TTRC("Scale Snap"));
	snap_dialog_vbc->add_margin_child(TTR("Scale Snap:"), snap_scale);

	/* SETTINGS DIALOG */

	settings_dialog = memnew(ConfirmationDialog);
	settings_dialog->set_title(TTRC("Viewport Settings"));
	add_child(settings_dialog);
	settings_vbc = memnew(VBoxContainer);
	settings_vbc->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	settings_dialog->add_child(settings_vbc);

	settings_fov = memnew(SpinBox);
	settings_fov->set_max(MAX_FOV);
	settings_fov->set_min(MIN_FOV);
	settings_fov->set_step(0.1);
	settings_fov->set_value(EDITOR_GET("editors/3d/default_fov"));
	settings_fov->set_select_all_on_focus(true);
	settings_fov->set_tooltip_text(TTRC("FOV is defined as a vertical value, as the editor camera always uses the Keep Height aspect mode."));
	settings_fov->set_accessibility_name(TTRC("Perspective VFOV (deg.):"));
	settings_vbc->add_margin_child(TTRC("Perspective VFOV (deg.):"), settings_fov);

	settings_znear = memnew(SpinBox);
	settings_znear->set_max(MAX_Z);
	settings_znear->set_min(MIN_Z);
	settings_znear->set_step(0.01);
	settings_znear->set_accessibility_name(TTRC("View Z-Near:"));
	settings_znear->set_value(EDITOR_GET("editors/3d/default_z_near"));
	settings_znear->set_select_all_on_focus(true);
	settings_vbc->add_margin_child(TTRC("View Z-Near:"), settings_znear);

	settings_zfar = memnew(SpinBox);
	settings_zfar->set_max(MAX_Z);
	settings_zfar->set_min(MIN_Z);
	settings_zfar->set_step(0.1);
	settings_zfar->set_accessibility_name(TTRC("View Z-Far:"));
	settings_zfar->set_value(EDITOR_GET("editors/3d/default_z_far"));
	settings_zfar->set_select_all_on_focus(true);
	settings_vbc->add_margin_child(TTRC("View Z-Far:"), settings_zfar);

	for (uint32_t i = 0; i < VIEWPORTS_COUNT; ++i) {
		settings_dialog->connect(SceneStringName(confirmed), callable_mp(viewports[i], &Node3DEditorViewport::_view_settings_confirmed).bind(0.0));
	}

	/* XFORM DIALOG */

	xform_dialog = memnew(ConfirmationDialog);
	xform_dialog->set_title(TTRC("Transform Change"));
	add_child(xform_dialog);

	VBoxContainer *xform_vbc = memnew(VBoxContainer);
	xform_dialog->add_child(xform_vbc);

	HBoxContainer *translate_hb = memnew(HBoxContainer);
	xform_vbc->add_margin_child(TTRC("Translate:"), translate_hb);
	HBoxContainer *rotate_hb = memnew(HBoxContainer);
	xform_vbc->add_margin_child(TTRC("Rotate (deg.):"), rotate_hb);
	HBoxContainer *scale_hb = memnew(HBoxContainer);
	xform_vbc->add_margin_child(TTRC("Scale (ratio):"), scale_hb);

	for (int i = 0; i < 3; i++) {
		xform_translate[i] = memnew(LineEdit);
		xform_translate[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		xform_translate[i]->set_select_all_on_focus(true);
		translate_hb->add_child(xform_translate[i]);

		xform_rotate[i] = memnew(LineEdit);
		xform_rotate[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		xform_rotate[i]->set_select_all_on_focus(true);
		rotate_hb->add_child(xform_rotate[i]);

		xform_scale[i] = memnew(LineEdit);
		xform_scale[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		xform_scale[i]->set_select_all_on_focus(true);
		scale_hb->add_child(xform_scale[i]);
	}

	xform_type = memnew(OptionButton);
	xform_type->set_h_size_flags(SIZE_EXPAND_FILL);
	xform_type->set_accessibility_name(TTRC("Transform Type"));
	xform_type->add_item(TTRC("Pre"));
	xform_type->add_item(TTRC("Post"));
	xform_vbc->add_margin_child(TTRC("Transform Type"), xform_type);

	xform_dialog->connect(SceneStringName(confirmed), callable_mp(this, &Node3DEditor::_xform_dialog_action));

	selected = nullptr;

	set_process_shortcut_input(true);
	if (scene_visuals_owner == nullptr) {
		scene_visuals_owner = this;
		add_to_group(SceneStringName(_spatial_editor_group));
	}

	current_hover_gizmo_handle = -1;
	current_hover_gizmo_handle_secondary = false;
	{
		// Sun/preview environment popup.
		sun_environ_popup = memnew(PreviewSunEnvPopup);
		add_child(sun_environ_popup);

		HBoxContainer *sun_environ_hb = memnew(HBoxContainer);

		sun_environ_popup->add_child(sun_environ_hb);

		sun_vb = memnew(VBoxContainer);
		sun_environ_hb->add_child(sun_vb);
		sun_vb->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
		sun_vb->hide();

		sun_title = memnew(Label);
		sun_title->set_theme_type_variation("HeaderMedium");
		sun_vb->add_child(sun_title);
		sun_title->set_text(TTRC("Preview Sun"));
		sun_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		CenterContainer *sun_direction_center = memnew(CenterContainer);
		sun_direction = memnew(Control);
		sun_direction->set_custom_minimum_size(Size2(128, 128) * EDSCALE);
		sun_direction_center->add_child(sun_direction);
		sun_vb->add_margin_child(TTRC("Sun Direction"), sun_direction_center);
		sun_direction->connect(SceneStringName(gui_input), callable_mp(this, &Node3DEditor::_sun_direction_input));
		sun_direction->connect(SceneStringName(draw), callable_mp(this, &Node3DEditor::_sun_direction_draw));
		sun_direction->set_default_cursor_shape(CURSOR_MOVE);

		sun_direction_shader.instantiate();
		sun_direction_shader->set_code(R"(
// 3D editor Preview Sun direction shader.

shader_type canvas_item;

uniform vec3 sun_direction;
uniform vec3 sun_color;

void fragment() {
	vec3 n;
	n.xy = UV * 2.0 - 1.0;
	n.z = sqrt(max(0.0, 1.0 - dot(n.xy, n.xy)));
	COLOR.rgb = dot(n, sun_direction) * sun_color;
	COLOR.a = 1.0 - smoothstep(0.99, 1.0, length(n.xy));
}
)");
		sun_direction_material.instantiate();
		sun_direction_material->set_shader(sun_direction_shader);
		sun_direction_material->set_shader_parameter("sun_direction", Vector3(0, 0, 1));
		sun_direction_material->set_shader_parameter("sun_color", Vector3(1, 1, 1));
		sun_direction->set_material(sun_direction_material);

		HBoxContainer *sun_angle_hbox = memnew(HBoxContainer);
		sun_angle_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
		VBoxContainer *sun_angle_altitude_vbox = memnew(VBoxContainer);
		sun_angle_altitude_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
		Label *sun_angle_altitude_label = memnew(Label);
		sun_angle_altitude_label->set_text(TTRC("Angular Altitude"));
		sun_angle_altitude_vbox->add_child(sun_angle_altitude_label);
		sun_angle_altitude = memnew(EditorSpinSlider);
		sun_angle_altitude->set_suffix(U"\u00B0");
		sun_angle_altitude->set_max(90);
		sun_angle_altitude->set_min(-90);
		sun_angle_altitude->set_step(0.1);
		sun_angle_altitude->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_sun_direction_set_altitude));
		sun_angle_altitude_vbox->add_child(sun_angle_altitude);
		sun_angle_hbox->add_child(sun_angle_altitude_vbox);
		VBoxContainer *sun_angle_azimuth_vbox = memnew(VBoxContainer);
		sun_angle_azimuth_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
		sun_angle_azimuth_vbox->set_custom_minimum_size(Vector2(100, 0));
		Label *sun_angle_azimuth_label = memnew(Label);
		sun_angle_azimuth_label->set_text(TTRC("Azimuth"));
		sun_angle_azimuth_vbox->add_child(sun_angle_azimuth_label);
		sun_angle_azimuth = memnew(EditorSpinSlider);
		sun_angle_azimuth->set_suffix(U"\u00B0");
		sun_angle_azimuth->set_max(180);
		sun_angle_azimuth->set_min(-180);
		sun_angle_azimuth->set_step(0.1);
		sun_angle_azimuth->set_allow_greater(true);
		sun_angle_azimuth->set_allow_lesser(true);
		sun_angle_azimuth->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_sun_direction_set_azimuth));
		sun_angle_azimuth_vbox->add_child(sun_angle_azimuth);
		sun_angle_hbox->add_child(sun_angle_azimuth_vbox);
		sun_angle_hbox->add_theme_constant_override("separation", 10);
		sun_vb->add_child(sun_angle_hbox);

		sun_color = memnew(ColorPickerButton);
		sun_color->set_edit_alpha(false);
		sun_vb->add_margin_child(TTRC("Sun Color"), sun_color);
		sun_color->connect("color_changed", callable_mp(this, &Node3DEditor::_sun_set_color));
		sun_color->get_popup()->connect("about_to_popup", callable_mp(EditorNode::get_singleton(), &EditorNode::setup_color_picker).bind(sun_color->get_picker()));

		sun_energy = memnew(EditorSpinSlider);
		sun_energy->set_max(64.0);
		sun_energy->set_min(0);
		sun_energy->set_step(0.05);
		sun_vb->add_margin_child(TTRC("Sun Energy"), sun_energy);
		sun_energy->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_sun_set_energy));

		sun_shadow_max_distance = memnew(EditorSpinSlider);
		sun_vb->add_margin_child(TTRC("Shadow Max Distance"), sun_shadow_max_distance);
		sun_shadow_max_distance->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_sun_set_shadow_max_distance));
		sun_shadow_max_distance->set_min(1);
		sun_shadow_max_distance->set_max(4096);

		sun_add_to_scene = memnew(Button);
		sun_add_to_scene->set_text(TTRC("Add Sun to Scene"));
		sun_add_to_scene->set_tooltip_text(TTRC("Adds a DirectionalLight3D node matching the preview sun settings to the current scene.\nHold Shift while clicking to also add the preview environment to the current scene."));
		sun_add_to_scene->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_add_sun_to_scene).bind(false));
		sun_vb->add_spacer();
		sun_vb->add_child(sun_add_to_scene);

		sun_state = memnew(Label);
		sun_environ_hb->add_child(sun_state);
		sun_state->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		sun_state->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		sun_state->set_h_size_flags(SIZE_EXPAND_FILL);

		VSeparator *sc = memnew(VSeparator);
		sc->set_custom_minimum_size(Size2(10 * EDSCALE, 0));
		sc->set_v_size_flags(SIZE_EXPAND_FILL);
		sun_environ_hb->add_child(sc);

		environ_vb = memnew(VBoxContainer);
		sun_environ_hb->add_child(environ_vb);
		environ_vb->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
		environ_vb->hide();

		environ_title = memnew(Label);
		environ_title->set_theme_type_variation("HeaderMedium");

		environ_vb->add_child(environ_title);
		environ_title->set_text(TTRC("Preview Environment"));
		environ_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		environ_sky_color = memnew(ColorPickerButton);
		environ_sky_color->set_edit_alpha(false);
		environ_sky_color->connect("color_changed", callable_mp(this, &Node3DEditor::_environ_set_sky_color));
		environ_sky_color->get_popup()->connect("about_to_popup", callable_mp(EditorNode::get_singleton(), &EditorNode::setup_color_picker).bind(environ_sky_color->get_picker()));
		environ_vb->add_margin_child(TTRC("Sky Color"), environ_sky_color);
		environ_ground_color = memnew(ColorPickerButton);
		environ_ground_color->connect("color_changed", callable_mp(this, &Node3DEditor::_environ_set_ground_color));
		environ_ground_color->set_edit_alpha(false);
		environ_ground_color->get_popup()->connect("about_to_popup", callable_mp(EditorNode::get_singleton(), &EditorNode::setup_color_picker).bind(environ_ground_color->get_picker()));
		environ_vb->add_margin_child(TTRC("Ground Color"), environ_ground_color);
		environ_energy = memnew(EditorSpinSlider);
		environ_energy->set_max(8.0);
		environ_energy->set_min(0);
		environ_energy->set_step(0.05);
		environ_energy->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_environ_set_sky_energy));
		environ_vb->add_margin_child(TTRC("Sky Energy"), environ_energy);
		HBoxContainer *fx_vb = memnew(HBoxContainer);
		fx_vb->set_h_size_flags(SIZE_EXPAND_FILL);

		environ_ao_button = memnew(Button);
		environ_ao_button->set_text(TTRC("AO"));
		environ_ao_button->set_h_size_flags(SIZE_EXPAND_FILL);
		environ_ao_button->set_toggle_mode(true);
		environ_ao_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_environ_set_ao), CONNECT_DEFERRED);
		fx_vb->add_child(environ_ao_button);
		environ_glow_button = memnew(Button);
		environ_glow_button->set_text(TTRC("Glow"));
		environ_glow_button->set_h_size_flags(SIZE_EXPAND_FILL);
		environ_glow_button->set_toggle_mode(true);
		environ_glow_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_environ_set_glow), CONNECT_DEFERRED);
		fx_vb->add_child(environ_glow_button);
		environ_tonemap_button = memnew(Button);
		environ_tonemap_button->set_text(TTRC("Tonemap"));
		environ_tonemap_button->set_h_size_flags(SIZE_EXPAND_FILL);
		environ_tonemap_button->set_toggle_mode(true);
		environ_tonemap_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_environ_set_tonemap), CONNECT_DEFERRED);
		fx_vb->add_child(environ_tonemap_button);
		environ_gi_button = memnew(Button);
		environ_gi_button->set_text(TTRC("GI"));
		environ_gi_button->set_h_size_flags(SIZE_EXPAND_FILL);
		environ_gi_button->set_toggle_mode(true);
		environ_gi_button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_environ_set_gi), CONNECT_DEFERRED);
		fx_vb->add_child(environ_gi_button);
		environ_vb->add_margin_child(TTRC("Post Process"), fx_vb);

		environ_add_to_scene = memnew(Button);
		environ_add_to_scene->set_text(TTRC("Add Environment to Scene"));
		environ_add_to_scene->set_tooltip_text(TTRC("Adds a WorldEnvironment node matching the preview environment settings to the current scene.\nHold Shift while clicking to also add the preview sun to the current scene."));
		environ_add_to_scene->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_add_environment_to_scene).bind(false));
		environ_vb->add_spacer();
		environ_vb->add_child(environ_add_to_scene);

		environ_state = memnew(Label);
		sun_environ_hb->add_child(environ_state);
		environ_state->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		environ_state->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		environ_state->set_h_size_flags(SIZE_EXPAND_FILL);

		_ensure_preview_nodes();

		sun_environ_popup->set_process_shortcut_input(true);

		_load_default_preview_settings();
		_preview_settings_changed();
	}

	if (!EDITOR_GET("interface/editor/appearance/classic_viewport_toolbars")) {
		_arrange_chrome();
	}
	clear(); // Make sure values are initialized. Will call _snap_update() for us.
}

void Node3DEditor::_arrange_chrome() {
	// The same buttons the classic toolbar has, moved: nothing is built twice,
	// and nothing a plugin was handed - the context toolbar, the side panels,
	// the shader split, the menus - is replaced or reparented.
	HBoxContainer *tools_group = Object::cast_to<HBoxContainer>(tool_button[TOOL_MODE_TRANSFORM]->get_parent());
	HBoxContainer *selection_group = Object::cast_to<HBoxContainer>(tool_button[TOOL_MODE_LIST_SELECT]->get_parent());
	HBoxContainer *options_group = Object::cast_to<HBoxContainer>(tool_option_button[TOOL_OPT_LOCAL_COORDS]->get_parent());
	HBoxContainer *preview_group = Object::cast_to<HBoxContainer>(sun_button->get_parent());
	HBoxContainer *menus_group = Object::cast_to<HBoxContainer>(transform_menu->get_parent());
	ERR_FAIL_COND(!tools_group || !selection_group || !options_group || !preview_group || !menus_group);

	// The tools, down the left side of the viewports.
	tool_column_panel = memnew(PanelContainer);
	tool_column_panel->set_name("ToolColumn");
	tool_column = memnew(VBoxContainer);
	tool_column_panel->add_child(tool_column);
	const ToolMode column[] = {
		TOOL_MODE_TRANSFORM, TOOL_MODE_MOVE, TOOL_MODE_ROTATE, TOOL_MODE_SCALE, TOOL_MODE_SELECT, TOOL_MAX,
		TOOL_MODE_LIST_SELECT, TOOL_RULER, TOOL_MAX,
		TOOL_LOCK_SELECTED, TOOL_UNLOCK_SELECTED, TOOL_GROUP_SELECTED, TOOL_UNGROUP_SELECTED
	};
	tool_column->add_theme_constant_override("separation", 2 * EDSCALE);
	for (ToolMode tool : column) {
		if (tool == TOOL_MAX) {
			tool_column->add_child(memnew(HSeparator));
			continue;
		}
		tool_button[tool]->get_parent()->remove_child(tool_button[tool]);
		tool_button[tool]->set_custom_minimum_size(Size2(28, 28) * EDSCALE);
		tool_button[tool]->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		tool_column->add_child(tool_button[tool]);
	}
	// Only their separators are left.
	toolbar_flow->remove_child(tools_group);
	memdelete(tools_group);
	toolbar_flow->remove_child(selection_group);
	memdelete(selection_group);

	HBoxContainer *viewport_row = memnew(HBoxContainer);
	viewport_row->set_name("ViewportRow");
	viewport_row->set_v_size_flags(SIZE_EXPAND_FILL);
	viewport_row->add_theme_constant_override("separation", 0);
	const int at = viewport_base->get_index();
	shader_split->remove_child(viewport_base);
	shader_split->add_child(viewport_row);
	shader_split->move_child(viewport_row, at);
	viewport_row->add_child(tool_column_panel);
	// The viewports, with room over them for the sidebar.
	Control *viewport_stack = memnew(Control);
	viewport_stack->set_name("ViewportStack");
	viewport_stack->set_clip_contents(true);
	viewport_stack->set_h_size_flags(SIZE_EXPAND_FILL);
	viewport_stack->set_v_size_flags(SIZE_EXPAND_FILL);
	viewport_row->add_child(viewport_stack);
	viewport_base->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	viewport_stack->add_child(viewport_base);

	// The header: the menus first, then how a transform is done, then what
	// plugins add, and the view's display and lighting at the far end.
	toolbar_flow->move_child(menus_group, 0);
	toolbar_flow->move_child(options_group, 1);
	toolbar_flow->move_child(context_toolbar_panel, 2);

	Control *spacer = memnew(Control);
	spacer->set_h_size_flags(SIZE_EXPAND_FILL);
	spacer->set_mouse_filter(MOUSE_FILTER_IGNORE);
	toolbar_flow->add_child(spacer);

	header_end = preview_group;
	header_end->set_name("HeaderEnd");
	// Its separator was for something after it; nothing is now.
	Node *last = header_end->get_child(header_end->get_child_count() - 1);
	if (Object::cast_to<VSeparator>(last)) {
		header_end->remove_child(last);
		memdelete(last);
	}
	toolbar_flow->move_child(header_end, -1);

	// How the view draws, the way other 3D applications offer it: the modes
	// each viewport's own menu has, for all of this view's viewports at once.
	HBoxContainer *shading_group = memnew(HBoxContainer);
	shading_group->set_name("Shading");
	shading_group->add_theme_constant_override("separation", 0);
	Ref<ButtonGroup> shading_button_group;
	shading_button_group.instantiate();
	const char *shading_names[] = { TTRC("Display Wireframe"), TTRC("Display Unshaded"), TTRC("Display Lighting"), TTRC("Display Normal") };
	for (int i = 0; i < Node3DEditorChrome::SHADING_MAX; i++) {
		Button *button = memnew(Button);
		button->set_toggle_mode(true);
		button->set_button_group(shading_button_group);
		button->set_theme_type_variation(SceneStringName(FlatButton));
		button->set_tooltip_text(shading_names[i]);
		button->set_accessibility_name(shading_names[i]);
		button->connect(SceneStringName(pressed), callable_mp(this, &Node3DEditor::_shading_pressed).bind(i));
		shading_group->add_child(button);
		shading_buttons[i] = button;
	}
	shading_buttons[Node3DEditorChrome::SHADING_NORMAL]->set_pressed_no_signal(true);
	header_end->add_child(shading_group);
	header_end->move_child(shading_group, 0);

	overlays_menu = memnew(MenuButton);
	overlays_menu->set_name("Overlays");
	overlays_menu->set_flat(false);
	overlays_menu->set_theme_type_variation("FlatMenuButton");
	overlays_menu->set_tooltip_text(TTRC("Overlays"));
	overlays_menu->set_accessibility_name(TTRC("Overlays"));
	overlays_menu->set_shortcut_context(this);
	PopupMenu *overlays_popup = overlays_menu->get_popup();
	overlays_popup->set_hide_on_checkable_item_selection(false);
	overlays_popup->connect("about_to_popup", callable_mp(this, &Node3DEditor::_overlays_about_to_popup));
	overlays_popup->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::_overlays_id_pressed));
	overlays_gizmos_menu = memnew(PopupMenu);
	overlays_gizmos_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	overlays_gizmos_menu->set_hide_on_checkable_item_selection(false);
	overlays_gizmos_menu->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::_overlays_gizmo_pressed));
	// Owned from the start, whether the menu is ever opened or not.
	overlays_popup->add_child(overlays_gizmos_menu);
	header_end->add_child(overlays_menu);
	header_end->move_child(overlays_menu, 0);

	_build_sidebar(viewport_stack);

	display_menu = memnew(MenuButton);
	display_menu->set_name("DisplayModes");
	display_menu->set_flat(false);
	display_menu->set_theme_type_variation("FlatMenuButton");
	display_menu->set_tooltip_text(TTRC("All Display Modes"));
	display_menu->set_accessibility_name(TTRC("All Display Modes"));
	PopupMenu *display_popup = display_menu->get_popup();
	display_popup->connect("about_to_popup", callable_mp(this, &Node3DEditor::_display_menu_about_to_popup));
	display_popup->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::set_display_everywhere));
	display_advanced_menu = memnew(PopupMenu);
	display_advanced_menu->connect(SceneStringName(id_pressed), callable_mp(this, &Node3DEditor::set_display_everywhere));
	display_popup->add_child(display_advanced_menu);

	_group_header();
}

void Node3DEditor::_group_header() {
	// A group of the classic toolbar - an HBoxContainer ending in a separator
	// - put in a frame instead, where it was.
	auto frame = [](HBoxContainer *p_row) -> EditorViewHeaderGroup * {
		EditorViewHeaderGroup *group = memnew(EditorViewHeaderGroup);
		Vector<Control *> keep;
		for (int i = 0; i < p_row->get_child_count(); i++) {
			Control *child = Object::cast_to<Control>(p_row->get_child(i));
			if (child && !Object::cast_to<VSeparator>(child)) {
				keep.push_back(child);
			}
		}
		group->take(keep);
		Node *parent = p_row->get_parent();
		const int at = p_row->get_index();
		parent->remove_child(p_row);
		memdelete(p_row);
		parent->add_child(group);
		parent->move_child(group, at);
		return group;
	};
	frame(Object::cast_to<HBoxContainer>(transform_menu->get_parent()))->set_name("MenusGroup");
	frame(Object::cast_to<HBoxContainer>(tool_option_button[TOOL_OPT_LOCAL_COORDS]->get_parent()))->set_name("OptionsGroup");

	// The far end: how the view draws, its preview lighting, the sidebar.
	HBoxContainer *shading_row = Object::cast_to<HBoxContainer>(shading_buttons[0]->get_parent());
	EditorViewHeaderGroup *display_group = memnew(EditorViewHeaderGroup);
	display_group->set_name("DisplayGroup");
	display_group->take({ overlays_menu, shading_row, display_menu });
	EditorViewHeaderGroup *lighting_group = memnew(EditorViewHeaderGroup);
	lighting_group->set_name("LightingGroup");
	lighting_group->take({ sun_button, environ_button });
	// Their settings are the sidebar's Environment page now; one way to
	// them is enough. Kept, hidden, for anything that looks for it.
	sun_environ_settings->hide();
	EditorViewHeaderGroup *sidebar_group = memnew(EditorViewHeaderGroup);
	sidebar_group->set_name("SidebarGroup");
	sidebar_group->take({ sidebar_button });
	// What is left there are the separators the frames replace.
	for (int i = header_end->get_child_count() - 1; i >= 0; i--) {
		Node *left = header_end->get_child(i);
		if (Object::cast_to<VSeparator>(left)) {
			header_end->remove_child(left);
			memdelete(left);
		}
	}
	header_end->add_theme_constant_override("separation", 6 * EDSCALE);
	header_end->add_child(display_group);
	header_end->add_child(lighting_group);
	header_end->add_child(sidebar_group);
	header_end->move_child(sun_environ_settings, -1);
	toolbar_flow->add_theme_constant_override("h_separation", 6 * EDSCALE);
}

void Node3DEditor::_display_menu_about_to_popup() {
	PopupMenu *popup = display_menu->get_popup();
	popup->clear(false);
	const Node3DEditorViewport *viewport = viewports[CLAMP(last_used_viewport, 0, (int)VIEWPORTS_COUNT - 1)];
	const PopupMenu *source = viewport->view_display_menu->get_popup();
	const int ids[] = {
		Node3DEditorViewport::VIEW_DISPLAY_NORMAL,
		Node3DEditorViewport::VIEW_DISPLAY_WIREFRAME,
		Node3DEditorViewport::VIEW_DISPLAY_OVERDRAW,
		Node3DEditorViewport::VIEW_DISPLAY_LIGHTING,
		Node3DEditorViewport::VIEW_DISPLAY_UNSHADED,
	};
	for (int id : ids) {
		const int index = source->get_item_index(id);
		if (index < 0) {
			continue;
		}
		popup->add_radio_check_item(source->get_item_text(index), id);
		popup->set_item_checked(popup->get_item_count() - 1, source->is_item_checked(index));
	}
	// The advanced ones, as the viewport has them: which the renderer in use
	// can draw, and which is on.
	display_advanced_menu->clear();
	const PopupMenu *advanced = viewport->display_submenu;
	for (int i = 0; i < advanced->get_item_count(); i++) {
		if (advanced->is_item_separator(i)) {
			display_advanced_menu->add_separator();
			continue;
		}
		display_advanced_menu->add_radio_check_item(advanced->get_item_text(i), advanced->get_item_id(i));
		const int index = display_advanced_menu->get_item_count() - 1;
		display_advanced_menu->set_item_checked(index, advanced->is_item_checked(i));
		display_advanced_menu->set_item_disabled(index, advanced->is_item_disabled(i));
		display_advanced_menu->set_item_tooltip(index, advanced->get_item_tooltip(i));
	}
	popup->add_separator();
	popup->add_submenu_node_item(TTR("Display Advanced..."), display_advanced_menu);
}

void Node3DEditor::_build_sidebar(Control *p_over) {
	sidebar = memnew(EditorViewSidebar);
	p_over->add_child(sidebar);

	item_panel = memnew(Node3DEditorItemPanel);
	sidebar->add_page(TTR("Item"), item_panel);

	// What the View Settings dialog had, applied as it changes.
	settings_vbc->get_parent()->remove_child(settings_vbc);
	settings_vbc->set_custom_minimum_size(Size2());
	sidebar->add_page(TTR("View"), settings_vbc);
	settings_fov->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_view_settings_changed).unbind(1));
	settings_znear->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_view_settings_changed).unbind(1));
	settings_zfar->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_view_settings_changed).unbind(1));

	// What the Snap Settings dialog had, the same way.
	Control *snap_fields = Object::cast_to<Control>(snap_translate->get_parent()->get_parent());
	snap_fields->get_parent()->remove_child(snap_fields);
	sidebar->add_page(TTR("Snap"), snap_fields);
	snap_translate->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_snap_changed).unbind(1));
	snap_rotate->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_snap_changed).unbind(1));
	snap_scale->connect(SceneStringName(value_changed), callable_mp(this, &Node3DEditor::_snap_changed).unbind(1));

	// The preview sun and environment, one above the other rather than side
	// by side as in their popup.
	VBoxContainer *environment_page = memnew(VBoxContainer);
	Control *environment_parts[] = { sun_vb, sun_state, environ_vb, environ_state };
	for (Control *part : environment_parts) {
		part->get_parent()->remove_child(part);
		environment_page->add_child(part);
	}
	sun_state->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	environ_state->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	environment_page->add_child(memnew(HSeparator));
	environment_page->move_child(environment_page->get_child(environment_page->get_child_count() - 1), 2);
	sidebar->add_page(TTR("Environment"), environment_page);

	sidebar->connect(SNAME("fitted"), callable_mp(this, &Node3DEditor::_sidebar_fitted));
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->connect(SceneStringName(resized), callable_mp(this, &Node3DEditor::_sidebar_fitted));
		viewports[i]->connect(SceneStringName(visibility_changed), callable_mp(this, &Node3DEditor::_sidebar_fitted));
	}

	sidebar_button = memnew(Button);
	sidebar_button->set_name("SidebarButton");
	sidebar_button->set_toggle_mode(true);
	sidebar_button->set_theme_type_variation(SceneStringName(FlatButton));
	sidebar_button->set_tooltip_text(TTRC("Sidebar: the selected nodes, the view, snapping and the preview lighting."));
	sidebar_button->set_accessibility_name(TTRC("Toggle Sidebar"));
	sidebar_button->set_shortcut(ED_SHORTCUT("spatial_editor/toggle_sidebar", TTRC("Toggle Sidebar"), Key::N));
	sidebar_button->set_shortcut_context(this);
	sidebar_button->connect(SceneStringName(toggled), callable_mp(this, &Node3DEditor::_sidebar_button_toggled));
	header_end->add_child(sidebar_button);

	addon_mirror = memnew(EditorButtonMirror);
	// Pressing a copy acts in this view, on its document, as pressing
	// anything else in it does.
	addon_mirror->set_before_press(callable_mp(this, &Node3DEditor::_activate_for_user));
	addon_mirror->set_shortcut_context(this);

	hints = memnew(EditorViewHints);
	hints->set_visible(EditorSettings::get_singleton()->get_project_metadata("3d_editor", "key_hints", false));
	add_child(hints);
	// Said as soon as it is on screen, not at the next tick.
	hints->connect(SceneStringName(visibility_changed), callable_mp(this, &Node3DEditor::_update_hints), CONNECT_DEFERRED);
	// Often enough to follow a drag starting or a flight ending, and only
	// while the line is on screen.
	Timer *hints_timer = memnew(Timer);
	hints_timer->set_wait_time(0.15);
	hints_timer->set_autostart(true);
	hints_timer->connect("timeout", callable_mp(this, &Node3DEditor::_chrome_tick));
	add_child(hints_timer);
}

void Node3DEditor::_chrome_tick() {
	_update_hints();
	_sync_addon_mirrors();
}

void Node3DEditor::_activate_for_user() {
	make_active();
	EditorMainScreen *main_screen = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_main_screen() : nullptr;
	if (main_screen) {
		main_screen->view_activated(this);
	}
}

void Node3DEditor::_queue_addon_mirror_rebuild() {
	if (addon_mirrors_queued) {
		return;
	}
	addon_mirrors_queued = true;
	callable_mp_static(&Node3DEditor::_rebuild_all_addon_mirrors).call_deferred();
}

void Node3DEditor::_rebuild_all_addon_mirrors() {
	addon_mirrors_queued = false;
	for (Node3DEditor *instance : instances) {
		instance->_rebuild_addon_mirrors();
	}
}

void Node3DEditor::_rebuild_addon_mirrors() {
	if (!addon_mirror || !primary_instance || primary_instance == this) {
		return;
	}
	addon_mirror->clear();
	while (context_toolbar_hbox->get_child_count() > 0) {
		Node *copy = context_toolbar_hbox->get_child(0);
		context_toolbar_hbox->remove_child(copy);
		memdelete(copy);
	}
	addon_mirror->mirror_all(primary_instance->context_toolbar_hbox, context_toolbar_hbox);
	_sync_addon_mirrors();
}

void Node3DEditor::_sync_addon_mirrors() {
	if (!addon_mirror || !primary_instance || primary_instance == this) {
		return;
	}
	addon_mirror->sync();
	bool any = false;
	for (int i = 0; i < context_toolbar_hbox->get_child_count() && !any; i++) {
		const Control *copy = Object::cast_to<Control>(context_toolbar_hbox->get_child(i));
		any = copy && copy->is_visible();
	}
	context_toolbar_panel->set_visible(any);
}

void Node3DEditor::_update_hints() {
	if (!hints || !hints->is_visible_in_tree()) {
		return;
	}
	using Hint = EditorViewHints::Hint;
	Vector<Hint> list;
	const Node3DEditorViewport *viewport = viewports[CLAMP(last_used_viewport, 0, (int)VIEWPORTS_COUNT - 1)];
	const String ctrl = keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL);
	const String lmb = EditorViewHints::mouse_button_name(MouseButton::LEFT);
	const String shift = keycode_get_string((Key)KeyModifierMask::SHIFT) + "+";

	if (viewport->_edit.mode != Node3DEditorViewport::TRANSFORM_NONE) {
		const String axes = ED_GET_SHORTCUT("spatial_editor/lock_transform_x")->get_as_text() + " " + ED_GET_SHORTCUT("spatial_editor/lock_transform_y")->get_as_text() + " " + ED_GET_SHORTCUT("spatial_editor/lock_transform_z")->get_as_text();
		list.push_back(Hint{ axes, TTR("Lock to axis") });
		list.push_back(Hint{ shift + axes, TTR("Lock to plane") });
		list.push_back(Hint{ ctrl, TTR("Snap") });
		list.push_back(Hint{ ED_GET_SHORTCUT("spatial_editor/cancel_transform")->get_as_text(), TTR("Cancel") });
		hints->set_hints(list);
		return;
	}

	if (viewport->view_3d_controller.is_valid() && viewport->view_3d_controller->is_freelook_enabled()) {
		const String move = EditorViewHints::action_key("spatial_editor/freelook_forward") + EditorViewHints::action_key("spatial_editor/freelook_left") + EditorViewHints::action_key("spatial_editor/freelook_backwards") + EditorViewHints::action_key("spatial_editor/freelook_right");
		list.push_back(Hint{ move, TTR("Fly") });
		list.push_back(Hint{ EditorViewHints::action_key("spatial_editor/freelook_down") + " " + EditorViewHints::action_key("spatial_editor/freelook_up"), TTR("Down, up") });
		list.push_back(Hint{ EditorViewHints::action_key("spatial_editor/freelook_speed_modifier"), TTR("Faster") });
		list.push_back(Hint{ EditorViewHints::action_key("spatial_editor/freelook_slow_modifier"), TTR("Slower") });
		list.push_back(Hint{ TTR("Wheel"), TTR("Speed") });
		hints->set_hints(list);
		return;
	}

	switch (tool_mode) {
		case TOOL_MODE_SELECT: {
			list.push_back(Hint{ lmb, TTR("Select") });
			list.push_back(Hint{ shift + lmb, TTR("Add to selection") });
			list.push_back(Hint{ vformat(TTR("%s drag"), lmb), TTR("Box select") });
		} break;
		case TOOL_MODE_MOVE:
		case TOOL_MODE_ROTATE:
		case TOOL_MODE_SCALE: {
			static const char *actions[] = { TTRC("Move"), TTRC("Rotate"), TTRC("Scale") };
			list.push_back(Hint{ vformat(TTR("%s drag"), lmb), TTRGET(actions[tool_mode - TOOL_MODE_MOVE]) });
			list.push_back(Hint{ ctrl, TTR("Snap") });
		} break;
		case TOOL_MODE_TRANSFORM: {
			list.push_back(Hint{ lmb, TTR("Select, or drag the manipulator") });
			list.push_back(Hint{ vformat(TTR("%s drag"), ctrl), TTR("Rotate around pivot") });
		} break;
		case TOOL_RULER: {
			list.push_back(Hint{ vformat(TTR("%s drag"), lmb), TTR("Measure") });
			list.push_back(Hint{ shift + vformat(TTR("%s drag"), lmb), TTR("Each axis") });
		} break;
		default: {
		} break;
	}

	// Getting around, the way the navigation settings have it.
	static const MouseButton buttons[] = { MouseButton::LEFT, MouseButton::MIDDLE, MouseButton::RIGHT, MouseButton::MB_XBUTTON1, MouseButton::MB_XBUTTON2 };
	struct Navigation {
		const char *setting;
		const char *modifier;
		const char *action;
	};
	static const Navigation navigation[] = {
		{ "editors/3d/navigation/orbit_mouse_button", "spatial_editor/viewport_orbit_modifier_1", TTRC("Orbit") },
		{ "editors/3d/navigation/pan_mouse_button", "spatial_editor/viewport_pan_modifier_1", TTRC("Pan") },
		{ "editors/3d/navigation/zoom_mouse_button", "spatial_editor/viewport_zoom_modifier_1", TTRC("Zoom") },
	};
	for (const Navigation &way : navigation) {
		const int button = CLAMP((int)EDITOR_GET(way.setting), 0, 4);
		const String modifier = EditorViewHints::action_key(way.modifier);
		list.push_back(Hint{ (modifier.is_empty() ? String() : modifier + "+") + EditorViewHints::mouse_button_name(buttons[button]), TTRGET(way.action) });
	}
	list.push_back(Hint{ vformat(TTR("%s hold"), EditorViewHints::mouse_button_name(MouseButton::RIGHT)), TTR("Fly") });
	list.push_back(Hint{ ED_GET_SHORTCUT("spatial_editor/focus_selection")->get_as_text(), TTR("Focus") });
	if (sidebar_button) {
		list.push_back(Hint{ sidebar_button->get_shortcut()->get_as_text(), TTR("Sidebar") });
	}
	hints->set_hints(list);
}

void Node3DEditor::_sidebar_button_toggled(bool p_pressed) {
	if (sidebar->is_open() != p_pressed) {
		sidebar->toggle();
	}
}

void Node3DEditor::_sidebar_fitted() {
	if (!sidebar) {
		return;
	}
	sidebar_button->set_pressed_no_signal(sidebar->is_open());
	// The navigation gizmo of a viewport the card covers the corner of moves
	// out from under it.
	const Rect2 card = sidebar->is_visible_in_tree() ? sidebar->get_global_rect() : Rect2();
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		real_t clearance = 0.0;
		if (card.has_area() && viewports[i]->is_visible_in_tree()) {
			const Rect2 area = viewports[i]->get_global_rect();
			if (area.intersects(card) && card.position.y < area.position.y + 200 * EDSCALE) {
				clearance = MAX((real_t)0.0, area.get_end().x - card.position.x);
			}
		}
		viewports[i]->set_top_right_clearance(clearance);
	}
}

void Node3DEditor::_view_settings_changed() {
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->_view_settings_confirmed(0.0);
	}
}

void Node3DEditor::set_shading(int p_shading) {
	_shading_pressed(p_shading);
}

void Node3DEditor::set_display_everywhere(int p_display_option) {
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->_menu_option(p_display_option);
	}
}

void Node3DEditor::toggle_overlay(int p_overlay) {
	_overlays_id_pressed(p_overlay);
}

void Node3DEditor::toggle_tool_option(int p_option) {
	ERR_FAIL_INDEX(p_option, TOOL_OPT_MAX);
	tool_option_button[p_option]->set_pressed(!tool_option_button[p_option]->is_pressed());
}

bool Node3DEditor::is_tool_option_on(int p_option) const {
	ERR_FAIL_INDEX_V(p_option, TOOL_OPT_MAX, false);
	return tool_option_button[p_option]->is_pressed();
}

void Node3DEditor::show_snap_settings() {
	_menu_item_activated(MENU_TRANSFORM_CONFIGURE_SNAP);
}

void Node3DEditor::_shading_pressed(int p_shading) {
	ERR_FAIL_INDEX(p_shading, Node3DEditorChrome::SHADING_MAX);
	static const int shading_display_options[Node3DEditorChrome::SHADING_MAX] = {
		Node3DEditorViewport::VIEW_DISPLAY_WIREFRAME,
		Node3DEditorViewport::VIEW_DISPLAY_UNSHADED,
		Node3DEditorViewport::VIEW_DISPLAY_LIGHTING,
		Node3DEditorViewport::VIEW_DISPLAY_NORMAL,
	};
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		viewports[i]->_menu_option(shading_display_options[p_shading]);
	}
}

const Node3DEditor::OverlayItem *Node3DEditor::_overlay_items(int &r_count) {
	// In the order of Overlay.
	static const OverlayItem items[] = {
		{ TTRC("Grid"), -1, Node3DEditorViewport::VIEW_GRID },
		{ TTRC("Origin"), MENU_VIEW_ORIGIN, -1 },
		{ TTRC("Gizmos"), -1, Node3DEditorViewport::VIEW_GIZMOS },
		{ TTRC("Transform Gizmo"), -1, Node3DEditorViewport::VIEW_TRANSFORM_GIZMO },
		{ TTRC("Information"), -1, Node3DEditorViewport::VIEW_INFORMATION },
		{ TTRC("Frame Time"), -1, Node3DEditorViewport::VIEW_FRAME_TIME },
		{ TTRC("Environment"), -1, Node3DEditorViewport::VIEW_ENVIRONMENT },
		// The view's own: the line of hints under it.
		{ TTRC("Key Hints"), -1, -1 },
	};
	static_assert(std::size(items) == OVERLAY_MAX);
	r_count = std::size(items);
	return items;
}

bool Node3DEditor::_overlay_shown_in(int p_overlay, int p_viewport) const {
	int count = 0;
	const OverlayItem *items = _overlay_items(count);
	ERR_FAIL_INDEX_V(p_overlay, count, false);
	const OverlayItem &item = items[p_overlay];
	if (p_overlay == OVERLAY_KEY_HINTS) {
		return hints && hints->is_visible();
	}
	if (item.layout_option >= 0) {
		const PopupMenu *popup = view_layout_menu->get_popup();
		return popup->is_item_checked(popup->get_item_index(item.layout_option));
	}
	const PopupMenu *popup = viewports[p_viewport]->view_display_menu->get_popup();
	return popup->is_item_checked(popup->get_item_index(item.viewport_option));
}

bool Node3DEditor::is_overlay_shown_everywhere(Overlay p_overlay) const {
	for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
		if (!_overlay_shown_in(p_overlay, i)) {
			return false;
		}
	}
	return true;
}

void Node3DEditor::_overlays_about_to_popup() {
	// Built each time it opens, from the menus that hold the state: those can
	// change it too, by their own items and shortcuts.
	PopupMenu *popup = overlays_menu->get_popup();
	// Not freeing the gizmos submenu, which is kept.
	popup->clear(false);
	int count = 0;
	const OverlayItem *items = _overlay_items(count);
	const int viewport = CLAMP(last_used_viewport, 0, (int)VIEWPORTS_COUNT - 1);
	for (int i = 0; i < count; i++) {
		popup->add_check_item(TTRGET(items[i].name), i);
		popup->set_item_checked(popup->get_item_index(i), _overlay_shown_in(i, viewport));
	}
	popup->add_separator();
	_fill_gizmos_menu(overlays_gizmos_menu);
	popup->add_submenu_node_item(TTR("Gizmo Types"), overlays_gizmos_menu);
}

void Node3DEditor::_overlays_id_pressed(int p_overlay) {
	int count = 0;
	const OverlayItem *items = _overlay_items(count);
	ERR_FAIL_INDEX(p_overlay, count);
	const OverlayItem &item = items[p_overlay];
	const int viewport = CLAMP(last_used_viewport, 0, (int)VIEWPORTS_COUNT - 1);
	const bool show = !_overlay_shown_in(p_overlay, viewport);
	if (p_overlay == OVERLAY_KEY_HINTS) {
		if (hints) {
			hints->set_visible(show);
			EditorSettings::get_singleton()->set_project_metadata("3d_editor", "key_hints", show);
			_update_hints();
		}
	} else if (item.layout_option >= 0) {
		_menu_item_activated(item.layout_option);
	} else {
		// Every viewport of the view ends up the same, whatever each had.
		for (uint32_t i = 0; i < VIEWPORTS_COUNT; i++) {
			if (_overlay_shown_in(p_overlay, i) != show) {
				viewports[i]->_menu_option(item.viewport_option);
			}
		}
	}
	if (overlays_menu) {
		PopupMenu *popup = overlays_menu->get_popup();
		const int index = popup->get_item_index(p_overlay);
		if (index >= 0) {
			popup->set_item_checked(index, _overlay_shown_in(p_overlay, viewport));
		}
	}
}

void Node3DEditor::_overlays_gizmo_pressed(int p_gizmo) {
	// The View menu's Gizmos submenu keeps the state; this one shows it.
	_menu_gizmo_toggled(p_gizmo);
	_fill_gizmos_menu(overlays_gizmos_menu);
}

void Node3DEditor::update_shading_buttons() {
	if (!shading_buttons[0]) {
		return;
	}
	int shown = -1;
	switch (viewports[CLAMP(last_used_viewport, 0, (int)VIEWPORTS_COUNT - 1)]->viewport->get_debug_draw()) {
		case Viewport::DEBUG_DRAW_WIREFRAME: {
			shown = Node3DEditorChrome::SHADING_WIREFRAME;
		} break;
		case Viewport::DEBUG_DRAW_UNSHADED: {
			shown = Node3DEditorChrome::SHADING_UNSHADED;
		} break;
		case Viewport::DEBUG_DRAW_LIGHTING: {
			shown = Node3DEditorChrome::SHADING_LIGHTING;
		} break;
		case Viewport::DEBUG_DRAW_DISABLED: {
			shown = Node3DEditorChrome::SHADING_NORMAL;
		} break;
		default: {
			// Overdraw or one of the advanced modes: none of the four.
		} break;
	}
	for (int i = 0; i < Node3DEditorChrome::SHADING_MAX; i++) {
		shading_buttons[i]->set_pressed_no_signal(i == shown);
	}
}
Node3DEditor::~Node3DEditor() {
	instances.erase(this);
	if (primary_instance == this) {
		primary_instance = nullptr;
	}
	if (addon_mirror) {
		memdelete(addon_mirror);
		addon_mirror = nullptr;
	}
	_release_world_visuals();
	_release_preview_owner();
	if (active_instance == this) {
		// Hand the context to another open space rather than leaving it dangling.
		active_instance = instances.is_empty() ? nullptr : instances[0];
	}
	if (scene_visuals_owner == this) {
		// Another open view takes over, or nodes entering the tree would stop
		// getting gizmos and the grid would stay gone: this view's EXIT_TREE has
		// already freed it.
		// The grid and origin lines outlive this view - EXIT_TREE only frees them
		// when the last one closes - so the new owner simply inherits them.
		scene_visuals_owner = instances.is_empty() ? nullptr : instances[0];
		if (scene_visuals_owner) {
			scene_visuals_owner->add_to_group(SceneStringName(_spatial_editor_group));
		}
	}

	if (instances.is_empty()) {
		// Nothing is left that would show them.
		_finish_indicators();

		// What is held above any one view has to go with the last of them. These
		// were members before views could exist more than once, and were freed
		// with the only view there was; shared, nothing was freeing them, and
		// they outlived the servers that own what they point at.
		gizmo_plugins_by_priority.clear();
		gizmo_plugins_by_name.clear();
		built_in_gizmos_registered = false;
		origin_mat.unref();
		for (int i = 0; i < 3; i++) {
			grid_mat[i].unref();
		}
	}
	memdelete(preview_node);
	// The preview nodes are taken along rather than left lighting a scene for
	// nobody - unless the document they were parked in was torn down first, at
	// editor exit or when its tab was closed, and took them with it.
	_drop_freed_preview_nodes();
	if (preview_sun) {
		if (preview_sun->get_parent()) {
			preview_sun->get_parent()->remove_child(preview_sun);
		}
		memdelete(preview_sun);
	}
	if (preview_environment) {
		if (preview_environment->get_parent()) {
			preview_environment->get_parent()->remove_child(preview_environment);
		}
		memdelete(preview_environment);
	}
}

bool Node3DEditorPlugin::release_main_screen_view(Control *p_view) {
	// Only the one the editor built for itself comes back here; every other is
	// the pane's own and the pane frees it.
	if (p_view != spatial_editor || !parked_parent) {
		return false;
	}
	if (spatial_editor->get_parent()) {
		spatial_editor->get_parent()->remove_child(spatial_editor);
	}
	parked_parent->add_child(spatial_editor);
	spatial_editor->hide();
	return true;
}

Control *Node3DEditorPlugin::create_main_screen_view() {
	// The first view asked for is the one already standing in the main screen
	// with nothing showing it - a pane is exactly something to show it in.
	// Another is built only once that one is spoken for.
	if (spatial_editor && spatial_editor->get_parent() && spatial_editor->get_parent() == parked_parent) {
		parked_parent->remove_child(spatial_editor);
		spatial_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		spatial_editor->set_process(true);
		spatial_editor->set_physics_process(true);
		spatial_editor->show();
		return spatial_editor;
	}

	// Everything a second view needs is already shared above any one of them:
	// the gizmo plugins, the grid and the origin lines. What it does not share
	// is the document it edits, which is the point of having it.
	Node3DEditor *view = memnew(Node3DEditor);
	view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	view->set_process(true);
	view->set_physics_process(true);
	return view;
}

void Node3DEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		spatial_editor->show();
		spatial_editor->set_process(true);
		spatial_editor->set_physics_process(true);
		spatial_editor->refresh_dirty_gizmos();
	} else {
		spatial_editor->hide();
		spatial_editor->set_process(false);
		spatial_editor->set_physics_process(false);
	}
}

void Node3DEditorPlugin::edit(Object *p_object) {
	spatial_editor->edit(Object::cast_to<Node3D>(p_object));
}

bool Node3DEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("Node3D");
}

Dictionary Node3DEditorPlugin::get_state() const {
	Node3DEditor *view = _view_following_current_document();
	if (view) {
		return view->get_state();
	}
	// No view is following the current document, so none of them has anything to
	// say about it. Repeat what was said last time rather than dropping the
	// camera positions the document remembers.
	EditorData &editor_data = EditorNode::get_editor_data();
	const int idx = editor_data.get_edited_scene();
	if (idx >= 0 && idx < editor_data.get_edited_scene_count()) {
		const Dictionary states = editor_data.get_scene_editor_states(idx);
		if (states.has(get_plugin_name())) {
			return states[get_plugin_name()];
		}
	}
	return Dictionary();
}

Node3DEditor *Node3DEditorPlugin::_view_following_current_document() const {
	for (Node3DEditor *editor : Node3DEditor::get_instances()) {
		if (editor->get_bound_document() < 0) {
			return editor;
		}
	}
	return nullptr;
}

void Node3DEditorPlugin::set_state(const Dictionary &p_state) {
	// A document's saved state carries the camera positions it was last looked
	// at from. Handing them to a view that is held to a document of its own
	// moves a camera nobody touched, because some other pane changed which
	// document is current - which is what made the first pane's camera jump.
	Node3DEditor *view = _view_following_current_document();
	if (view) {
		view->set_state(p_state);
	}
}

Size2i Node3DEditor::get_camera_viewport_size(Camera3D *p_camera) {
	Viewport *viewport = p_camera->get_viewport();

	Window *window = Object::cast_to<Window>(viewport);
	if (window) {
		return window->get_size();
	}

	SubViewport *sub_viewport = Object::cast_to<SubViewport>(viewport);
	ERR_FAIL_NULL_V(sub_viewport, Size2i());

	// Any document's root reports the project resolution; a SubViewport placed
	// inside a scene reports its own size. Static, so it cannot go through a
	// view's binding and asks the document list directly.
	EditorData &ed = EditorNode::get_editor_data();
	for (int i = 0; i < ed.get_edited_scene_count(); i++) {
		if (sub_viewport == ed.get_scene_root_viewport(i)) {
			return Size2(GLOBAL_GET("display/window/size/viewport_width"), GLOBAL_GET("display/window/size/viewport_height"));
		}
	}

	return sub_viewport->get_size();
}

Vector3 Node3DEditor::snap_point(Vector3 p_target, Vector3 p_start) const {
	if (is_snap_enabled()) {
		real_t snap = get_translate_snap();
		p_target.snapf(snap);
	}
	return p_target;
}

bool Node3DEditor::is_gizmo_visible() const {
	if (selected) {
		return gizmo.visible && selected->is_transform_gizmo_visible();
	}
	return gizmo.visible;
}

bool Node3DEditor::is_vertex_snap_use_collision() const {
	return vertex_snap_use_collision != Input::get_singleton()->is_key_pressed(Key::SHIFT);
}

real_t Node3DEditor::get_translate_snap() const {
	real_t snap_value = snap_translate_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 10.0f;
	}
	return snap_value;
}

real_t Node3DEditor::get_rotate_snap() const {
	real_t snap_value = snap_rotate_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 3.0f;
	}
	return snap_value;
}

real_t Node3DEditor::get_scale_snap() const {
	real_t snap_value = snap_scale_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 2.0f;
	}
	return snap_value;
}

struct _GizmoPluginPriorityComparator {
	bool operator()(const Ref<EditorNode3DGizmoPlugin> &p_a, const Ref<EditorNode3DGizmoPlugin> &p_b) const {
		if (p_a->get_priority() == p_b->get_priority()) {
			return p_a->get_gizmo_name() < p_b->get_gizmo_name();
		}
		return p_a->get_priority() > p_b->get_priority();
	}
};

struct _GizmoPluginNameComparator {
	bool operator()(const Ref<EditorNode3DGizmoPlugin> &p_a, const Ref<EditorNode3DGizmoPlugin> &p_b) const {
		return p_a->get_gizmo_name() < p_b->get_gizmo_name();
	}
};

void Node3DEditor::add_gizmo_plugin(Ref<EditorNode3DGizmoPlugin> p_plugin) {
	ERR_FAIL_COND(p_plugin.is_null());

	gizmo_plugins_by_priority.push_back(p_plugin);
	gizmo_plugins_by_priority.sort_custom<_GizmoPluginPriorityComparator>();

	gizmo_plugins_by_name.push_back(p_plugin);
	gizmo_plugins_by_name.sort_custom<_GizmoPluginNameComparator>();

	_update_all_gizmos_menus();
}

void Node3DEditor::remove_gizmo_plugin(Ref<EditorNode3DGizmoPlugin> p_plugin) {
	gizmo_plugins_by_priority.erase(p_plugin);
	gizmo_plugins_by_name.erase(p_plugin);
	_update_all_gizmos_menus();
}

void Node3DEditor::_update_all_gizmos_menus() {
	// The plugin set is shared, so every open view's Gizmos menu follows it.
	// Every view is rebuilt, in the tree or not: modules register their gizmo
	// plugins while a view may still be outside it, and a menu left un-rebuilt
	// has no item for them, which _update_gizmos_menu_theme() would then look
	// up and get -1 for.
	for (int i = 0; i < instances.size(); i++) {
		instances[i]->_update_gizmos_menu();
	}
}

int Node3DEditor::acquire_gizmo_layer(const RID &p_scenario) {
	if (p_scenario.is_null()) {
		return Node3DEditorViewport::GIZMO_BASE_LAYER;
	}
	uint32_t &taken = gizmo_layers_in_use[p_scenario];
	for (int i = 0; i < Node3DEditorViewport::GIZMO_VIEW_LAYER_COUNT; i++) {
		const int layer = Node3DEditorViewport::GIZMO_VIEW_LAYERS[i];
		if (!(taken & (1 << i))) {
			taken |= 1 << i;
			return layer;
		}
	}
	// More views are showing this one document than there are layers to keep
	// their manipulators apart. Sharing one means seeing a neighbour's
	// manipulator, which is worth more than refusing to open the view.
	WARN_PRINT_ONCE("More than " + itos(Node3DEditorViewport::GIZMO_VIEW_LAYER_COUNT) + " 3D views are open on one scene; their manipulators will be drawn in each other's views.");
	return Node3DEditorViewport::GIZMO_BASE_LAYER;
}

void Node3DEditor::release_gizmo_layer(const RID &p_scenario, int p_layer) {
	HashMap<RID, uint32_t>::Iterator taken = gizmo_layers_in_use.find(p_scenario);
	if (!taken) {
		return;
	}
	for (int i = 0; i < Node3DEditorViewport::GIZMO_VIEW_LAYER_COUNT; i++) {
		if (Node3DEditorViewport::GIZMO_VIEW_LAYERS[i] == p_layer) {
			taken->value &= ~(1 << i);
			break;
		}
	}
	if (taken->value == 0) {
		gizmo_layers_in_use.remove(taken);
	}
}

DynamicBVH::ID Node3DEditor::insert_gizmo_bvh_node(Node3D *p_node, const AABB &p_aabb) {
	return gizmo_bvh.insert(p_aabb, p_node);
}

void Node3DEditor::update_gizmo_bvh_node(DynamicBVH::ID p_id, const AABB &p_aabb) {
	gizmo_bvh.update(p_id, p_aabb);
	gizmo_bvh.optimize_incremental(1);
}

void Node3DEditor::remove_gizmo_bvh_node(DynamicBVH::ID p_id) {
	gizmo_bvh.remove(p_id);
}

// Documents stay live in roots of their own now, so gizmos created for a scene
// that was current earlier are still in the BVH. Picking must not reach them:
// the view would hand a node of another scene to the scene in front of it.
static Vector<Node3D *> _keep_nodes_of_scene(const Vector<Node3D *> &p_nodes, const Node *p_edited_scene) {
	if (!p_edited_scene) {
		return Vector<Node3D *>();
	}
	Vector<Node3D *> kept;
	for (Node3D *node : p_nodes) {
		if (node && (node == p_edited_scene || p_edited_scene->is_ancestor_of(node))) {
			kept.push_back(node);
		}
	}
	return kept;
}

Vector<Node3D *> Node3DEditor::gizmo_bvh_ray_query(const Vector3 &p_ray_start, const Vector3 &p_ray_end) {
	struct Result {
		Vector<Node3D *> nodes;
		bool operator()(void *p_data) {
			nodes.append((Node3D *)p_data);
			return false;
		}
	} result;

	gizmo_bvh.ray_query(p_ray_start, p_ray_end, result);

	return _keep_nodes_of_scene(result.nodes, get_edited_scene());
}

Vector<Node3D *> Node3DEditor::gizmo_bvh_frustum_query(const Vector<Plane> &p_frustum) {
	Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&p_frustum[0], p_frustum.size());

	struct Result {
		Vector<Node3D *> nodes;
		bool operator()(void *p_data) {
			nodes.append((Node3D *)p_data);
			return false;
		}
	} result;

	gizmo_bvh.convex_query(p_frustum.ptr(), p_frustum.size(), points.ptr(), points.size(), result);

	return _keep_nodes_of_scene(result.nodes, get_edited_scene());
}

Node3DEditorPlugin::Node3DEditorPlugin() {
	{
		// Named once here, built by anything that arranges panels. Nothing that
		// places a 3D view has to know this class exists.
		EditorPanelRegistry::PanelType type;
		type.id = "view_3d";
		type.title = TTRC("3D");
		type.icon = "3D";
		type.binding = EditorPanelRegistry::BINDING_DOCUMENT;
		type.featured = true;
		type.create = callable_mp(this, &Node3DEditorPlugin::create_main_screen_view);
		type.release = callable_mp(this, &Node3DEditorPlugin::release_main_screen_view);
		type.bind = callable_mp_static(&EditorDocumentView::bind_panel);
		EditorPanelRegistry::register_type(type);
	}

	spatial_editor = memnew(Node3DEditor);
	spatial_editor->make_primary();
	spatial_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	parked_parent = EditorNode::get_singleton()->get_editor_main_screen()->get_control();
	parked_parent->add_child(spatial_editor);

	spatial_editor->hide();
}
