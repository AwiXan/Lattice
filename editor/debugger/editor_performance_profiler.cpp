/**************************************************************************/
/*  editor_performance_profiler.cpp                                       */
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

#include "editor_performance_profiler.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "main/performance.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

EditorPerformanceProfiler::Monitor::Monitor(const String &p_name, const String &p_base, int p_frame_index, Performance::MonitorType p_type, TreeItem *p_item) {
	type = p_type;
	item = p_item;
	frame_index = p_frame_index;
	name = p_name;
	base = p_base;
	history_sum = 0.0f;
	max = 0.0f;
}

void EditorPerformanceProfiler::Monitor::reset() {
	history.clear();
	max = 0.0f;
	history_sum = 0.0f;
	if (item) {
		item->set_text(1, "");
		item->set_tooltip_text(1, "");
		item->set_text(2, "");
		item->set_tooltip_text(2, "");
		item->set_text(3, "");
		item->set_tooltip_text(3, "");
	}
}

String EditorPerformanceProfiler::_format_label(float p_value, Performance::MonitorType p_type) const {
	const String &lang = _get_locale();
	const TranslationServer *ts = TranslationServer::get_singleton();

	switch (p_type) {
		case Performance::MONITOR_TYPE_QUANTITY: {
			return ts->format_number(itos(p_value), lang);
		}
		case Performance::MONITOR_TYPE_MEMORY: {
			return String::humanize_size(p_value);
		}
		case Performance::MONITOR_TYPE_TIME: {
			return ts->format_number(rtos(p_value * 1000).pad_decimals(2), lang) + " " + TTR("ms");
		}
		case Performance::MONITOR_TYPE_PERCENTAGE: {
			return ts->format_number(rtos(p_value * 100).pad_decimals(2), lang) + "%";
		}
		default: {
			return ts->format_number(rtos(p_value), lang);
		}
	}
}

void EditorPerformanceProfiler::_update_monitor_value(Monitor *p_monitor, float p_value) {
	TreeItem *item = p_monitor->item;
	ERR_FAIL_NULL(item);

	const String label = EditorPerformanceProfiler::_format_label(p_value, p_monitor->type);
	item->set_text(1, label);

	if (p_value > p_monitor->max) {
		p_monitor->max = p_value;
	}

	float avg = p_monitor->history.size() > 0 ? (p_monitor->history_sum / float(p_monitor->history.size())) : 0.0f;
	item->set_text(2, _format_label(avg, p_monitor->type));
	item->set_text(3, _format_label(p_monitor->max, p_monitor->type));

	switch (p_monitor->type) {
		case Performance::MONITOR_TYPE_MEMORY:
		case Performance::MONITOR_TYPE_TIME: {
			item->set_tooltip_text(1, label);
			item->set_tooltip_text(2, vformat(TTR("Average: %s"), _format_label(avg, p_monitor->type)));
			item->set_tooltip_text(3, vformat(TTR("Peak: %s"), _format_label(p_monitor->max, p_monitor->type)));
		} break;

		default: {
			item->set_tooltip_text(1, label + " " + item->get_text(0));
			item->set_tooltip_text(2, vformat(TTR("Average: %s %s"), _format_label(avg, p_monitor->type), item->get_text(0)));
			item->set_tooltip_text(3, vformat(TTR("Peak: %s %s"), _format_label(p_monitor->max, p_monitor->type), item->get_text(0)));
		} break;
	}
}

void EditorPerformanceProfiler::_monitor_select() {
	monitor_draw->queue_redraw();
}

void EditorPerformanceProfiler::_monitor_draw() {
	Vector<StringName> active;
	for (const KeyValue<StringName, Monitor> &E : monitors) {
		if (E.value.item && E.value.item->is_checked(0)) {
			active.push_back(E.key);
		}
	}

	if (active.is_empty()) {
		info_message->show();
		return;
	}

	info_message->hide();

	Ref<StyleBox> graph_style_box = get_theme_stylebox(CoreStringName(normal), SNAME("TextEdit"));
	Ref<Font> graph_font = get_theme_font(SceneStringName(font), SNAME("TextEdit"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("TextEdit"));

	int columns = int(Math::ceil(Math::sqrt(float(active.size()))));
	int rows = int(Math::ceil(float(active.size()) / float(columns)));
	if (active.size() == 1) {
		rows = 1;
	}
	Size2i cell_size = Size2i(monitor_draw->get_size()) / Size2i(columns, rows);
	float spacing = float(POINT_SEPARATION) / float(columns);
	float value_multiplier = EditorThemeManager::is_dark_theme() ? 1.4f : 0.55f;
	float hue_shift = 1.0f / float(monitors.size());

	for (int i = 0; i < active.size(); i++) {
		Monitor &current = monitors[active[i]];
		Rect2i rect(Point2i(i % columns, i / columns) * cell_size + Point2i(MARGIN, MARGIN), cell_size - Point2i(MARGIN, MARGIN) * 2);
		monitor_draw->draw_style_box(graph_style_box, rect);

		rect.position += graph_style_box->get_offset();
		rect.size -= graph_style_box->get_minimum_size();
		Color draw_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		draw_color.set_hsv(Math::fmod(hue_shift * float(current.frame_index), 0.9f), draw_color.get_s() * 0.9f, draw_color.get_v() * value_multiplier, 0.6f);
		monitor_draw->draw_string(graph_font, rect.position + Point2(0, graph_font->get_ascent(font_size)), current.item->get_text(0), HORIZONTAL_ALIGNMENT_LEFT, rect.size.x, font_size, draw_color);

		draw_color.a = 0.9f;
		float value_position = rect.size.width - graph_font->get_string_size(current.item->get_text(1), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).width;
		if (value_position < 0) {
			value_position = 0;
		}
		monitor_draw->draw_string(graph_font, rect.position + Point2(value_position, graph_font->get_ascent(font_size)), current.item->get_text(1), HORIZONTAL_ALIGNMENT_LEFT, rect.size.x, font_size, draw_color);

		rect.position.y += graph_font->get_height(font_size);
		rect.size.height -= graph_font->get_height(font_size);

		int line_count = rect.size.height / (graph_font->get_height(font_size) * 2);
		if (line_count > 5) {
			line_count = 5;
		}
		if (line_count > 0) {
			Color horizontal_line_color;
			horizontal_line_color.set_hsv(draw_color.get_h(), draw_color.get_s() * 0.5f, draw_color.get_v() * 0.5f, 0.3f);
			monitor_draw->draw_line(rect.position, rect.position + Vector2(rect.size.width, 0), horizontal_line_color, Math::round(EDSCALE));
			monitor_draw->draw_string(graph_font, rect.position + Vector2(0, graph_font->get_ascent(font_size)), _format_label(current.max, current.type), HORIZONTAL_ALIGNMENT_LEFT, rect.size.width, font_size, horizontal_line_color);

			for (int j = 0; j < line_count; j++) {
				Vector2 y_offset = Vector2(0, rect.size.height * (1.0f - float(j) / float(line_count)));
				monitor_draw->draw_line(rect.position + y_offset, rect.position + Vector2(rect.size.width, 0) + y_offset, horizontal_line_color, Math::round(EDSCALE));
				monitor_draw->draw_string(graph_font, rect.position - Vector2(0, graph_font->get_descent(font_size)) + y_offset, _format_label(current.max * float(j) / float(line_count), current.type), HORIZONTAL_ALIGNMENT_LEFT, rect.size.width, font_size, horizontal_line_color);
			}
		}

		float from = rect.size.width;
		float prev = -1.0f;
		int count = 0;
		List<float>::Element *e = current.history.front();

		while (from >= 0 && e) {
			float m = current.max;
			float h2 = 0;
			if (m != 0) {
				h2 = (e->get() / m);
			}
			h2 = (1.0f - h2) * float(rect.size.y);
			if (e != current.history.front()) {
				monitor_draw->draw_line(rect.position + Point2(from, h2), rect.position + Point2(from + spacing, prev), draw_color, Math::round(EDSCALE));
			}

			if (marker_key == active[i] && count == marker_frame) {
				Color line_color;
				line_color.set_hsv(draw_color.get_h(), draw_color.get_s() * 0.8f, draw_color.get_v(), 0.5f);
				monitor_draw->draw_line(rect.position + Point2(from, 0), rect.position + Point2(from, rect.size.y), line_color, Math::round(EDSCALE));

				String label = _format_label(e->get(), current.type);
				Size2 size = graph_font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
				Vector2 text_top_left_position = Vector2(from, h2) - (size + Vector2(MARKER_MARGIN, MARKER_MARGIN));
				if (text_top_left_position.x < 0) {
					text_top_left_position.x = from + MARKER_MARGIN;
				}
				if (text_top_left_position.y < 0) {
					text_top_left_position.y = h2 + MARKER_MARGIN;
				}
				monitor_draw->draw_string(graph_font, rect.position + text_top_left_position + Point2(0, graph_font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, rect.size.x, font_size, line_color);
			}
			prev = h2;
			e = e->next();
			from -= spacing;
			count++;
		}
	}
}

void EditorPerformanceProfiler::_build_monitor_tree() {
	HashSet<StringName> monitor_checked;
	for (KeyValue<StringName, Monitor> &E : monitors) {
		if (E.value.item && E.value.item->is_checked(0)) {
			monitor_checked.insert(E.key);
		}
	}

	base_map.clear();
	monitor_tree->get_root()->clear_children();

	for (KeyValue<StringName, Monitor> &E : monitors) {
		TreeItem *base = _get_monitor_base(E.value.base);
		TreeItem *item = _create_monitor_item(E.value.name, base);
		item->set_checked(0, monitor_checked.has(E.key));
		E.value.item = item;
		if (!E.value.history.is_empty()) {
			_update_monitor_value(&E.value, E.value.history.front()->get());
		}
	}

	_apply_filter();
}

TreeItem *EditorPerformanceProfiler::_get_monitor_base(const StringName &p_base_name) {
	if (base_map.has(p_base_name)) {
		return base_map[p_base_name];
	}

	TreeItem *base = monitor_tree->create_item(monitor_tree->get_root());
	base->set_text(0, EditorPropertyNameProcessor::get_singleton()->process_name(p_base_name, EditorPropertyNameProcessor::get_settings_style()));
	base->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
	base->set_editable(0, false);
	base->set_selectable(0, false);
	base->set_expand_right(0, true);
	if (is_inside_tree()) {
		base->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	}

	String base_str = String(p_base_name).to_lower();
	StringName icon_name;
	if (base_str == "time") {
		icon_name = SNAME("Timer");
	} else if (base_str == "memory") {
		icon_name = SNAME("Memory");
	} else if (base_str == "raster") {
		icon_name = SNAME("VisualShader");
	} else if (base_str == "video") {
		icon_name = SNAME("Image");
	} else if (base_str.begins_with("physics_2d")) {
		icon_name = SNAME("CollisionShape2D");
	} else if (base_str.begins_with("physics")) {
		icon_name = SNAME("CollisionShape3D");
	} else if (base_str.begins_with("navigation")) {
		icon_name = SNAME("NavigationRegion3D");
	} else if (base_str == "audio") {
		icon_name = SNAME("AudioStreamPlayer");
	} else if (base_str == "object") {
		icon_name = SNAME("Object");
	}

	if (icon_name != StringName() && has_theme_icon(icon_name, SNAME("EditorIcons"))) {
		base->set_icon(0, get_theme_icon(icon_name, SNAME("EditorIcons")));
	}

	base_map.insert(p_base_name, base);
	return base;
}

TreeItem *EditorPerformanceProfiler::_create_monitor_item(const StringName &p_monitor_name, TreeItem *p_base) {
	TreeItem *item = monitor_tree->create_item(p_base);
	item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	item->set_editable(0, true);
	item->set_selectable(0, false);
	item->set_selectable(1, false);
	item->set_selectable(2, false);
	item->set_selectable(3, false);
	item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
	item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
	item->set_text(0, EditorPropertyNameProcessor::get_singleton()->process_name(p_monitor_name, EditorPropertyNameProcessor::get_settings_style()));
	return item;
}

void EditorPerformanceProfiler::_marker_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		Vector<StringName> active;
		for (KeyValue<StringName, Monitor> &E : monitors) {
			if (E.value.item && E.value.item->is_checked(0)) {
				active.push_back(E.key);
			}
		}
		if (active.size() > 0) {
			int columns = int(Math::ceil(Math::sqrt(float(active.size()))));
			int rows = int(Math::ceil(float(active.size()) / float(columns)));
			if (active.size() == 1) {
				rows = 1;
			}
			Size2i cell_size = Size2i(monitor_draw->get_size()) / Size2i(columns, rows);
			Vector2i index = mb->get_position() / cell_size;
			Rect2i rect(index * cell_size + Point2i(MARGIN, MARGIN), cell_size - Point2i(MARGIN, MARGIN) * 2);
			if (rect.has_point(mb->get_position())) {
				if (index.x + index.y * columns < active.size()) {
					marker_key = active[index.x + index.y * columns];
				} else {
					marker_key = "";
				}
				Ref<StyleBox> graph_style_box = get_theme_stylebox(CoreStringName(normal), SNAME("TextEdit"));
				rect.position += graph_style_box->get_offset();
				rect.size -= graph_style_box->get_minimum_size();
				Vector2 point = mb->get_position() - rect.position;
				if (point.x >= rect.size.x) {
					marker_frame = 0;
				} else {
					int point_sep = 5;
					float spacing = float(point_sep) / float(columns);
					marker_frame = (rect.size.x - point.x) / spacing;
				}
				monitor_draw->queue_redraw();
				return;
			}
		}
		marker_key = "";
		monitor_draw->queue_redraw();
	}
}

void EditorPerformanceProfiler::_on_search_changed(const String &p_text) {
	search_filter = p_text.strip_edges().to_lower();
	_apply_filter();
}

void EditorPerformanceProfiler::_apply_filter() {
	for (KeyValue<StringName, TreeItem *> &E : base_map) {
		TreeItem *base = E.value;
		bool base_matches = search_filter.is_empty() || String(E.key).to_lower().contains(search_filter);
		int visible_children = 0;

		TreeItem *child = base->get_first_child();
		while (child) {
			String child_name = child->get_text(0).to_lower();
			bool child_matches = search_filter.is_empty() || base_matches || child_name.contains(search_filter);
			child->set_visible(child_matches);
			if (child_matches) {
				visible_children++;
			}
			child = child->get_next();
		}

		base->set_visible(visible_children > 0 || base_matches);
	}
}

void EditorPerformanceProfiler::_on_preset_selected(int p_id) {
	if (p_id == PRESET_CLEAR_ALL) {
		for (KeyValue<StringName, Monitor> &E : monitors) {
			if (E.value.item) {
				E.value.item->set_checked(0, false);
			}
		}
		_monitor_select();
		return;
	}
	if (p_id == PRESET_SELECT_ALL) {
		for (KeyValue<StringName, Monitor> &E : monitors) {
			if (E.value.item) {
				E.value.item->set_checked(0, true);
			}
		}
		_monitor_select();
		return;
	}

	for (KeyValue<StringName, Monitor> &E : monitors) {
		if (E.value.item) {
			E.value.item->set_checked(0, false);
		}
	}

	HashSet<String> targets;
	switch (p_id) {
		case PRESET_FRAMETIME: {
			targets.insert("time/fps");
			targets.insert("time/process");
			targets.insert("time/physics_process");
			targets.insert("time/navigation_process");
		} break;
		case PRESET_RENDERING: {
			targets.insert("raster/total_objects_drawn");
			targets.insert("raster/total_primitives_drawn");
			targets.insert("raster/total_draw_calls");
			targets.insert("video/video_mem");
			targets.insert("video/texture_mem");
			targets.insert("video/buffer_mem");
		} break;
		case PRESET_MEMORY: {
			targets.insert("memory/static");
			targets.insert("memory/static_max");
			targets.insert("video/video_mem");
			targets.insert("video/texture_mem");
			targets.insert("video/buffer_mem");
			targets.insert("object/objects");
			targets.insert("object/resources");
			targets.insert("object/nodes");
		} break;
		case PRESET_PHYSICS: {
			targets.insert("time/physics_process");
			targets.insert("physics_2d/active_objects");
			targets.insert("physics_2d/collision_pairs");
			targets.insert("physics_2d/islands");
			targets.insert("physics_3d/active_objects");
			targets.insert("physics_3d/collision_pairs");
			targets.insert("physics_3d/islands");
		} break;
		case PRESET_AUDIO: {
			targets.insert("audio/driver/output_latency");
		} break;
	}

	for (const String &t : targets) {
		if (monitors.has(t) && monitors[t].item) {
			monitors[t].item->set_checked(0, true);
		}
	}
	_monitor_select();
}

void EditorPerformanceProfiler::_on_view_mode_selected(int p_mode) {
	view_mode = ViewMode(p_mode);
	if (view_mode == VIEW_GRAPHS) {
		monitor_draw->show();
		breakdown_tree->hide();
		monitor_draw->queue_redraw();
	} else {
		monitor_draw->hide();
		breakdown_tree->show();
	}
}

void EditorPerformanceProfiler::_on_pause_toggled(bool p_pressed) {
	is_paused = p_pressed;
	if (btn_pause) {
		btn_pause->set_text(is_paused ? TTR("Resume") : TTR("Pause"));
		if (has_theme_icon(is_paused ? SNAME("Play") : SNAME("Pause"), SNAME("EditorIcons"))) {
			btn_pause->set_button_icon(get_theme_icon(is_paused ? SNAME("Play") : SNAME("Pause"), SNAME("EditorIcons")));
		}
		btn_pause->set_tooltip_text(is_paused ? TTR("Resume live monitoring") : TTR("Pause live monitoring to inspect current frame"));
	}
}

void EditorPerformanceProfiler::_on_copy_pressed() {
	if (Performance::get_singleton()) {
		String report = Performance::get_singleton()->get_frame_breakdown_text();
		DisplayServer::get_singleton()->clipboard_set(report);
	}
}

void EditorPerformanceProfiler::_on_clear_pressed() {
	reset();
}

void EditorPerformanceProfiler::_update_frame_breakdown(const Vector<float> &p_values) {
	float fps = 0.0f;
	float process_sec = 0.0f;
	float physics_sec = 0.0f;
	float nav_sec = 0.0f;
	float render_cpu_sec = 0.0f;
	float shader_wait_sec = 0.0f;
	float draw_calls = 0.0f;
	float primitives = 0.0f;
	float objects = 0.0f;
	float vram = 0.0f;

	if (p_values.size() > Performance::TIME_FPS) {
		fps = p_values[Performance::TIME_FPS];
	}
	if (p_values.size() > Performance::TIME_PROCESS) {
		process_sec = p_values[Performance::TIME_PROCESS];
	}
	if (p_values.size() > Performance::TIME_PHYSICS_PROCESS) {
		physics_sec = p_values[Performance::TIME_PHYSICS_PROCESS];
	}
	if (p_values.size() > Performance::TIME_NAVIGATION_PROCESS) {
		nav_sec = p_values[Performance::TIME_NAVIGATION_PROCESS];
	}
	if (Performance::get_singleton()) {
		render_cpu_sec = double(Performance::get_singleton()->get_frame_render_usec()) / 1000000.0;
		shader_wait_sec = double(Performance::get_singleton()->get_last_shader_wait_usec()) / 1000000.0;
	}
	if (p_values.size() > Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME) {
		draw_calls = p_values[Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME];
	}
	if (p_values.size() > Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME) {
		primitives = p_values[Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME];
	}
	if (p_values.size() > Performance::RENDER_TOTAL_OBJECTS_IN_FRAME) {
		objects = p_values[Performance::RENDER_TOTAL_OBJECTS_IN_FRAME];
	}
	if (p_values.size() > Performance::RENDER_VIDEO_MEM_USED) {
		vram = p_values[Performance::RENDER_VIDEO_MEM_USED];
	}

	Vector<RenderingServerTypes::FrameProfileArea> gpu_areas = last_gpu_areas;
	if (gpu_areas.is_empty() && RenderingServer::get_singleton()) {
		gpu_areas = RenderingServer::get_singleton()->get_frame_profile();
	}

	double total_gpu_ms = 0.0;
	for (int i = 0; i < gpu_areas.size(); i++) {
		if (gpu_areas[i].name.begins_with(">") || gpu_areas[i].name.begins_with("<")) {
			continue;
		}
		total_gpu_ms += gpu_areas[i].gpu_msec;
	}

	double process_ms = process_sec * 1000.0;
	double physics_ms = physics_sec * 1000.0;
	double nav_ms = nav_sec * 1000.0;
	double render_cpu_ms = render_cpu_sec * 1000.0;
	double shader_wait_ms = shader_wait_sec * 1000.0;

	double total_cpu_ms = process_ms;
	if (total_cpu_ms <= 0.0) {
		total_cpu_ms = render_cpu_ms;
	}

	double frame_duration_ms = MAX(total_cpu_ms, total_gpu_ms);
	if (frame_duration_ms <= 0.0 && fps > 0.0) {
		frame_duration_ms = 1000.0 / fps;
	}

	// Update diagnosis header pill:
	String bottleneck_text;
	Color pill_bg;
	Color pill_border;
	if (total_gpu_ms > total_cpu_ms * 1.15 && total_gpu_ms > 1.0) {
		bottleneck_text = vformat(TTR("[GPU-BOUND] %s ms GPU vs %s ms CPU (%d FPS)"), rtos(total_gpu_ms).pad_decimals(1), rtos(total_cpu_ms).pad_decimals(1), int(Math::round(fps)));
		pill_bg = Color(0.85, 0.45, 0.1, 0.25);
		pill_border = Color(0.9, 0.5, 0.15);
	} else if (total_cpu_ms > total_gpu_ms * 1.15 && total_cpu_ms > 1.0) {
		bottleneck_text = vformat(TTR("[CPU-BOUND] %s ms CPU vs %s ms GPU (%d FPS)"), rtos(total_cpu_ms).pad_decimals(1), rtos(total_gpu_ms).pad_decimals(1), int(Math::round(fps)));
		pill_bg = Color(0.1, 0.45, 0.85, 0.25);
		pill_border = Color(0.15, 0.55, 0.95);
	} else {
		bottleneck_text = vformat(TTR("[BALANCED] %s ms Frame (%d FPS)"), rtos(frame_duration_ms).pad_decimals(1), int(Math::round(fps)));
		pill_bg = Color(0.15, 0.65, 0.3, 0.25);
		pill_border = Color(0.2, 0.8, 0.4);
	}
	diagnosis_label->set_text(bottleneck_text);
	Ref<StyleBoxFlat> sb = diagnosis_panel->get_theme_stylebox(SNAME("panel"));
	if (sb.is_valid()) {
		sb->set_bg_color(pill_bg);
		sb->set_border_color(pill_border);
	}

	// Only update Breakdown Tree periodically or when paused:
	uint64_t now = OS::get_singleton()->get_ticks_msec();
	if (!is_paused && (now - last_breakdown_update_msec < 150 || !breakdown_tree->is_visible_in_tree())) {
		return;
	}
	last_breakdown_update_msec = now;

	breakdown_tree->clear();
	TreeItem *root = breakdown_tree->create_item();
	root->set_text(0, vformat(TTR("Frame Total (%d FPS)"), int(Math::round(fps))));
	root->set_text(1, "TOTAL");
	root->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
	root->set_text(2, vformat("%s ms", rtos(frame_duration_ms).pad_decimals(2)));
	root->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	root->set_text(3, "100.0%");
	root->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
	root->set_text(4, total_gpu_ms > total_cpu_ms ? TTR("[GPU-BOUND]") : TTR("[CPU-BOUND]"));

	struct HotspotEntry {
		String name;
		String category;
		double duration_ms = 0.0;
		double percent = 0.0;
	};
	LocalVector<HotspotEntry> hotspot_list;

	if (process_ms > 0.05) {
		hotspot_list.push_back({ "Scripts & Logic (_process)", "CPU", process_ms, 0.0 });
	}
	if (physics_ms > 0.05) {
		hotspot_list.push_back({ "Physics Step (_physics_process)", "CPU", physics_ms, 0.0 });
	}
	if (render_cpu_ms > 0.05) {
		hotspot_list.push_back({ "Render Prep & Culling", "CPU", render_cpu_ms, 0.0 });
	}
	if (nav_ms > 0.05) {
		hotspot_list.push_back({ "Navigation Server", "CPU", nav_ms, 0.0 });
	}
	if (shader_wait_ms > 0.1) {
		hotspot_list.push_back({ "Shader Compilation Stalls", "CPU", shader_wait_ms, 0.0 });
	}
	for (int i = 0; i < gpu_areas.size(); i++) {
		if (gpu_areas[i].name.begins_with(">") || gpu_areas[i].name.begins_with("<")) {
			continue;
		}
		if (gpu_areas[i].gpu_msec > 0.02) {
			hotspot_list.push_back({ gpu_areas[i].name, "GPU", gpu_areas[i].gpu_msec, 0.0 });
		}
	}

	double denom = frame_duration_ms > 0.001 ? frame_duration_ms : 1.0;
	for (uint32_t i = 0; i < hotspot_list.size(); i++) {
		hotspot_list[i].percent = (hotspot_list[i].duration_ms / denom) * 100.0;
	}
	for (uint32_t i = 1; i < hotspot_list.size(); i++) {
		for (uint32_t j = i; j > 0 && hotspot_list[j - 1].duration_ms < hotspot_list[j].duration_ms; j--) {
			SWAP(hotspot_list[j - 1], hotspot_list[j]);
		}
	}

	// 1. Hotspots Section:
	if (hotspot_list.size() > 0) {
		TreeItem *sec_hotspots = breakdown_tree->create_item(root);
		sec_hotspots->set_text(0, TTR("Top Bottlenecks (Sorted)"));
		sec_hotspots->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
		if (has_theme_icon(SNAME("StatusError"), SNAME("EditorIcons"))) {
			sec_hotspots->set_icon(0, get_theme_icon(SNAME("StatusError"), SNAME("EditorIcons")));
		}

		int limit = MIN(5, (int)hotspot_list.size());
		for (int i = 0; i < limit; i++) {
			TreeItem *item = breakdown_tree->create_item(sec_hotspots);
			item->set_text(0, hotspot_list[i].name);
			item->set_text(1, hotspot_list[i].category);
			item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			item->set_text(2, vformat("%s ms", rtos(hotspot_list[i].duration_ms).pad_decimals(2)));
			item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			item->set_text(3, vformat("%s%%", rtos(hotspot_list[i].percent).pad_decimals(1)));
			item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);

			if (hotspot_list[i].percent > 25.0) {
				item->set_text(4, "CRITICAL (>25%)");
				item->set_custom_color(4, Color(1.0, 0.35, 0.35));
			} else if (hotspot_list[i].percent > 12.0) {
				item->set_text(4, "HEAVY (>12%)");
				item->set_custom_color(4, Color(1.0, 0.75, 0.2));
			} else {
				item->set_text(4, "MODERATE");
				item->set_custom_color(4, Color(0.4, 0.85, 0.4));
			}
		}
	}

	// 2. CPU Stages Section:
	TreeItem *sec_cpu = breakdown_tree->create_item(root);
	sec_cpu->set_text(0, vformat(TTR("CPU Logic & Subsystems (%s ms)"), rtos(total_cpu_ms).pad_decimals(2)));
	sec_cpu->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	if (has_theme_icon(SNAME("Timer"), SNAME("EditorIcons"))) {
		sec_cpu->set_icon(0, get_theme_icon(SNAME("Timer"), SNAME("EditorIcons")));
	}

	auto add_cpu_item = [&](const String &name, double ms) {
		TreeItem *item = breakdown_tree->create_item(sec_cpu);
		item->set_text(0, name);
		item->set_text(1, "CPU");
		item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
		item->set_text(2, vformat("%s ms", rtos(ms).pad_decimals(2)));
		item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
		double pct = (ms / denom) * 100.0;
		item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
		item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
		if (pct > 25.0) {
			item->set_text(4, "HOTSPOT");
			item->set_custom_color(4, Color(1.0, 0.35, 0.35));
		}
	};

	add_cpu_item("Scripts & Logic (_process)", process_ms);
	add_cpu_item("Physics Step (_physics_process)", physics_ms);
	add_cpu_item("Render Prep & Culling (CPU)", render_cpu_ms);
	if (nav_ms > 0.01) {
		add_cpu_item("Navigation Server", nav_ms);
	}
	if (shader_wait_ms > 0.05) {
		add_cpu_item("Shader Compilation Stalls", shader_wait_ms);
	}

	// 3. GPU Passes Section:
	TreeItem *sec_gpu = breakdown_tree->create_item(root);
	sec_gpu->set_text(0, vformat(TTR("GPU Rendering Passes (%s ms)"), rtos(total_gpu_ms).pad_decimals(2)));
	sec_gpu->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	if (has_theme_icon(SNAME("VisualShader"), SNAME("EditorIcons"))) {
		sec_gpu->set_icon(0, get_theme_icon(SNAME("VisualShader"), SNAME("EditorIcons")));
	}

	if (gpu_areas.is_empty()) {
		TreeItem *empty_item = breakdown_tree->create_item(sec_gpu);
		empty_item->set_text(0, TTR("(Run project with Clustered / Forward+ to profile GPU passes)"));
	} else {
		for (int i = 0; i < gpu_areas.size(); i++) {
			if (gpu_areas[i].name.begins_with(">") || gpu_areas[i].name.begins_with("<")) {
				continue;
			}
			TreeItem *item = breakdown_tree->create_item(sec_gpu);
			item->set_text(0, gpu_areas[i].name);
			item->set_text(1, "GPU");
			item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			item->set_text(2, vformat("%s ms", rtos(gpu_areas[i].gpu_msec).pad_decimals(2)));
			item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (gpu_areas[i].gpu_msec / denom) * 100.0;
			item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			if (pct > 25.0) {
				item->set_text(4, "HOTSPOT");
				item->set_custom_color(4, Color(1.0, 0.35, 0.35));
			}
		}
	}

	// 4. Draw & Memory Metrics:
	TreeItem *sec_res = breakdown_tree->create_item(root);
	sec_res->set_text(0, TTR("Draw & Memory Metrics"));
	sec_res->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	if (has_theme_icon(SNAME("Image"), SNAME("EditorIcons"))) {
		sec_res->set_icon(0, get_theme_icon(SNAME("Image"), SNAME("EditorIcons")));
	}

	auto add_metric_item = [&](const String &name, const String &val) {
		TreeItem *item = breakdown_tree->create_item(sec_res);
		item->set_text(0, name);
		item->set_text(1, "STAT");
		item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
		item->set_text(2, val);
		item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	};

	add_metric_item("Draw Calls in Frame", vformat("%d calls", int(draw_calls)));
	add_metric_item("Primitives in Frame", vformat("%s k", String::num_real(primitives / 1000.0).pad_decimals(1)));
	add_metric_item("Objects Drawn in Frame", vformat("%d objects", int(objects)));
	add_metric_item("Video Memory Used", String::humanize_size(uint64_t(vram)));
}

void EditorPerformanceProfiler::reset() {
	HashMap<StringName, Monitor>::Iterator E = monitors.begin();
	while (E != monitors.end()) {
		HashMap<StringName, Monitor>::Iterator N = E;
		++N;
		if (String(E->key).begins_with("custom:")) {
			monitors.remove(E);
		} else {
			E->value.reset();
		}
		E = N;
	}

	_build_monitor_tree();
	marker_key = "";
	marker_frame = 0;
	if (breakdown_tree) {
		breakdown_tree->clear();
	}
	monitor_draw->queue_redraw();
}

void EditorPerformanceProfiler::update_monitors(const Vector<StringName> &p_names, const PackedInt32Array &p_types) {
	HashMap<StringName, int> names;
	for (int i = 0; i < p_names.size(); i++) {
		names.insert("custom:" + p_names[i], Performance::MONITOR_MAX + i);
	}

	{
		HashMap<StringName, Monitor>::Iterator E = monitors.begin();
		while (E != monitors.end()) {
			HashMap<StringName, Monitor>::Iterator N = E;
			++N;
			if (String(E->key).begins_with("custom:")) {
				if (!names.has(E->key)) {
					monitors.remove(E);
				} else {
					E->value.frame_index = names[E->key];
					names.erase(E->key);
				}
			}
			E = N;
		}
	}

	int index = 0;
	for (const KeyValue<StringName, int> &E : names) {
		String name = String(E.key).replace_first("custom:", "");
		String base = "Custom";
		if (name.get_slice_count("/") == 2) {
			base = name.get_slicec('/', 0);
			name = name.get_slicec('/', 1);
		}
		Performance::MonitorType type = Performance::MonitorType(p_types[index]);
		monitors.insert(E.key, Monitor(name, base, E.value, type, nullptr));
		index++;
	}

	_build_monitor_tree();
}

void EditorPerformanceProfiler::add_profile_frame(const Vector<float> &p_values) {
	if (is_paused) {
		return;
	}

	for (KeyValue<StringName, Monitor> &E : monitors) {
		float value = 0.0f;
		if (E.value.frame_index >= 0 && E.value.frame_index < p_values.size()) {
			value = p_values[E.value.frame_index];
		}
		E.value.history.push_front(value);
		E.value.history_sum += value;
		if (E.value.history.size() > MAX_HISTORY) {
			E.value.history_sum -= E.value.history.back()->get();
			E.value.history.pop_back();
		}
		_update_monitor_value(&E.value, value);
	}
	marker_frame++;

	_update_frame_breakdown(p_values);

	if (view_mode == VIEW_GRAPHS) {
		monitor_draw->queue_redraw();
	}
}

void EditorPerformanceProfiler::add_visual_profile_frame(const Vector<RenderingServerTypes::FrameProfileArea> &p_areas) {
	last_gpu_areas = p_areas;
}

List<float> *EditorPerformanceProfiler::get_monitor_data(const StringName &p_name) {
	if (monitors.has(p_name)) {
		return &monitors[p_name].history;
	}
	return nullptr;
}

void EditorPerformanceProfiler::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (is_ready()) {
				_build_monitor_tree();
				if (monitor_draw->is_visible_in_tree()) {
					monitor_draw->queue_redraw();
				}
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			for (KeyValue<StringName, TreeItem *> &E : base_map) {
				E.value->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
				String base_str = String(E.key).to_lower();
				StringName icon_name;
				if (base_str == "time") {
					icon_name = SNAME("Timer");
				} else if (base_str == "memory") {
					icon_name = SNAME("Memory");
				} else if (base_str == "raster") {
					icon_name = SNAME("VisualShader");
				} else if (base_str == "video") {
					icon_name = SNAME("Image");
				} else if (base_str.begins_with("physics_2d")) {
					icon_name = SNAME("CollisionShape2D");
				} else if (base_str.begins_with("physics")) {
					icon_name = SNAME("CollisionShape3D");
				} else if (base_str.begins_with("navigation")) {
					icon_name = SNAME("NavigationRegion3D");
				} else if (base_str == "audio") {
					icon_name = SNAME("AudioStreamPlayer");
				} else if (base_str == "object") {
					icon_name = SNAME("Object");
				}
				if (icon_name != StringName() && has_theme_icon(icon_name, SNAME("EditorIcons"))) {
					E.value->set_icon(0, get_theme_icon(icon_name, SNAME("EditorIcons")));
				}
			}
			if (btn_pause && has_theme_icon(is_paused ? SNAME("Play") : SNAME("Pause"), SNAME("EditorIcons"))) {
				btn_pause->set_button_icon(get_theme_icon(is_paused ? SNAME("Play") : SNAME("Pause"), SNAME("EditorIcons")));
			}
			if (btn_copy && has_theme_icon(SNAME("ActionCopy"), SNAME("EditorIcons"))) {
				btn_copy->set_button_icon(get_theme_icon(SNAME("ActionCopy"), SNAME("EditorIcons")));
			}
			if (btn_clear && has_theme_icon(SNAME("Clear"), SNAME("EditorIcons"))) {
				btn_clear->set_button_icon(get_theme_icon(SNAME("Clear"), SNAME("EditorIcons")));
			}
			if (presets_button && has_theme_icon(SNAME("Favorites"), SNAME("EditorIcons"))) {
				presets_button->set_button_icon(get_theme_icon(SNAME("Favorites"), SNAME("EditorIcons")));
			}
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/localization/localize_settings")) {
				_build_monitor_tree();
			}
		} break;
	}
}

EditorPerformanceProfiler::EditorPerformanceProfiler() {
	set_name(TTRC("Monitors"));
	set_split_offset(380 * EDSCALE);

	// Left panel: Search filter + Presets + Monitors Tree
	left_vbox = memnew(VBoxContainer);
	left_vbox->set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	left_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	left_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(left_vbox);

	filter_hbox = memnew(HBoxContainer);
	left_vbox->add_child(filter_hbox);

	search_box = memnew(LineEdit);
	search_box->set_placeholder(TTR("Filter monitors..."));
	search_box->set_clear_button_enabled(true);
	search_box->set_h_size_flags(SIZE_EXPAND_FILL);
	search_box->connect(SceneStringName(text_changed), callable_mp(this, &EditorPerformanceProfiler::_on_search_changed));
	filter_hbox->add_child(search_box);

	presets_button = memnew(MenuButton);
	presets_button->set_text(TTR("Presets"));
	presets_button->set_flat(false);
	presets_button->set_theme_type_variation("FlatMenuButton");
	PopupMenu *preset_popup = presets_button->get_popup();
	preset_popup->add_item(TTR("Frametime & FPS"), PRESET_FRAMETIME);
	preset_popup->add_item(TTR("Rendering & GPU"), PRESET_RENDERING);
	preset_popup->add_item(TTR("Memory & VRAM"), PRESET_MEMORY);
	preset_popup->add_item(TTR("Physics & Collisions"), PRESET_PHYSICS);
	preset_popup->add_item(TTR("Audio Latency"), PRESET_AUDIO);
	preset_popup->add_separator();
	preset_popup->add_item(TTR("Select All"), PRESET_SELECT_ALL);
	preset_popup->add_item(TTR("Clear All"), PRESET_CLEAR_ALL);
	preset_popup->connect("id_pressed", callable_mp(this, &EditorPerformanceProfiler::_on_preset_selected));
	filter_hbox->add_child(presets_button);

	monitor_tree = memnew(Tree);
	monitor_tree->set_columns(4);
	monitor_tree->set_column_title(0, TTRC("Monitor"));
	monitor_tree->set_column_expand(0, true);
	monitor_tree->set_column_custom_minimum_width(0, 140 * EDSCALE);

	monitor_tree->set_column_title(1, TTRC("Value"));
	monitor_tree->set_column_custom_minimum_width(1, 75 * EDSCALE);
	monitor_tree->set_column_expand(1, false);

	monitor_tree->set_column_title(2, TTRC("Avg"));
	monitor_tree->set_column_custom_minimum_width(2, 75 * EDSCALE);
	monitor_tree->set_column_expand(2, false);

	monitor_tree->set_column_title(3, TTRC("Max"));
	monitor_tree->set_column_custom_minimum_width(3, 75 * EDSCALE);
	monitor_tree->set_column_expand(3, false);

	monitor_tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
	monitor_tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
	monitor_tree->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	monitor_tree->set_column_title_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
	monitor_tree->set_column_titles_visible(true);
	monitor_tree->connect("item_edited", callable_mp(this, &EditorPerformanceProfiler::_monitor_select));
	monitor_tree->create_item();
	monitor_tree->set_hide_root(true);
	monitor_tree->set_theme_type_variation("TreeSecondary");
	monitor_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	left_vbox->add_child(monitor_tree);

	// Right panel: Header toolbar + Graphs/Breakdown
	right_vbox = memnew(VBoxContainer);
	right_vbox->set_custom_minimum_size(Size2(340, 0) * EDSCALE);
	right_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	right_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(right_vbox);

	header_bar = memnew(HBoxContainer);
	right_vbox->add_child(header_bar);

	diagnosis_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_corner_radius_all(4 * EDSCALE);
	sb->set_border_width_all(1 * EDSCALE);
	sb->set_content_margin_individual(8 * EDSCALE, 4 * EDSCALE, 8 * EDSCALE, 4 * EDSCALE);
	sb->set_bg_color(Color(0.15, 0.65, 0.3, 0.25));
	sb->set_border_color(Color(0.2, 0.8, 0.4));
	diagnosis_panel->add_theme_style_override(SNAME("panel"), sb);

	diagnosis_label = memnew(Label);
	diagnosis_label->set_text(TTR("BALANCED (Analyzing...)"));
	diagnosis_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	diagnosis_panel->add_child(diagnosis_label);
	header_bar->add_child(diagnosis_panel);

	Control *spacer = memnew(Control);
	spacer->set_h_size_flags(SIZE_EXPAND_FILL);
	header_bar->add_child(spacer);

	view_mode_opt = memnew(OptionButton);
	view_mode_opt->add_item(TTR("Graphs View"), VIEW_GRAPHS);
	view_mode_opt->add_item(TTR("Frame Breakdown Tree"), VIEW_BREAKDOWN);
	view_mode_opt->connect("item_selected", callable_mp(this, &EditorPerformanceProfiler::_on_view_mode_selected));
	header_bar->add_child(view_mode_opt);

	btn_pause = memnew(Button);
	btn_pause->set_toggle_mode(true);
	btn_pause->set_text(TTR("Pause"));
	btn_pause->set_tooltip_text(TTR("Pause live monitoring to inspect current frame"));
	btn_pause->connect("toggled", callable_mp(this, &EditorPerformanceProfiler::_on_pause_toggled));
	header_bar->add_child(btn_pause);

	btn_copy = memnew(Button);
	btn_copy->set_text(TTR("Copy Report"));
	btn_copy->set_tooltip_text(TTR("Copy detailed frame performance report to clipboard"));
	btn_copy->connect(SceneStringName(pressed), callable_mp(this, &EditorPerformanceProfiler::_on_copy_pressed));
	header_bar->add_child(btn_copy);

	btn_clear = memnew(Button);
	btn_clear->set_text(TTR("Clear"));
	btn_clear->set_tooltip_text(TTR("Clear monitor history and reset peak/average stats"));
	btn_clear->connect(SceneStringName(pressed), callable_mp(this, &EditorPerformanceProfiler::_on_clear_pressed));
	header_bar->add_child(btn_clear);

	content_stack = memnew(Control);
	content_stack->set_h_size_flags(SIZE_EXPAND_FILL);
	content_stack->set_v_size_flags(SIZE_EXPAND_FILL);
	right_vbox->add_child(content_stack);

	monitor_draw = memnew(Control);
	monitor_draw->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	monitor_draw->set_clip_contents(true);
	monitor_draw->connect(SceneStringName(draw), callable_mp(this, &EditorPerformanceProfiler::_monitor_draw));
	monitor_draw->connect(SceneStringName(gui_input), callable_mp(this, &EditorPerformanceProfiler::_marker_input));
	content_stack->add_child(monitor_draw);

	info_message = memnew(Label);
	info_message->set_focus_mode(FOCUS_ACCESSIBILITY);
	info_message->set_text(TTRC("Pick one or more items from the list to display the graph."));
	info_message->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	info_message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	info_message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	info_message->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
	info_message->set_anchors_and_offsets_preset(PRESET_FULL_RECT, PRESET_MODE_KEEP_SIZE, 8 * EDSCALE);
	monitor_draw->add_child(info_message);

	breakdown_tree = memnew(Tree);
	breakdown_tree->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	breakdown_tree->set_columns(5);
	breakdown_tree->set_column_title(0, TTRC("Stage / Pass"));
	breakdown_tree->set_column_expand(0, true);
	breakdown_tree->set_column_custom_minimum_width(0, 220 * EDSCALE);

	breakdown_tree->set_column_title(1, TTRC("Category"));
	breakdown_tree->set_column_expand(1, false);
	breakdown_tree->set_column_custom_minimum_width(1, 75 * EDSCALE);

	breakdown_tree->set_column_title(2, TTRC("Time"));
	breakdown_tree->set_column_expand(2, false);
	breakdown_tree->set_column_custom_minimum_width(2, 85 * EDSCALE);

	breakdown_tree->set_column_title(3, TTRC("Share"));
	breakdown_tree->set_column_expand(3, false);
	breakdown_tree->set_column_custom_minimum_width(3, 75 * EDSCALE);

	breakdown_tree->set_column_title(4, TTRC("Status / Hotspot"));
	breakdown_tree->set_column_expand(4, false);
	breakdown_tree->set_column_custom_minimum_width(4, 120 * EDSCALE);

	breakdown_tree->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
	breakdown_tree->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
	breakdown_tree->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	breakdown_tree->set_column_title_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
	breakdown_tree->set_column_title_alignment(4, HORIZONTAL_ALIGNMENT_LEFT);
	breakdown_tree->set_column_titles_visible(true);
	breakdown_tree->set_hide_root(false);
	breakdown_tree->set_theme_type_variation("TreeSecondary");
	breakdown_tree->hide();
	content_stack->add_child(breakdown_tree);

	for (int i = 0; i < Performance::MONITOR_MAX; i++) {
		const Performance::Monitor monitor = Performance::Monitor(i);
		const String path = Performance::get_singleton()->get_monitor_name(monitor);
		const String base = path.get_slicec('/', 0);
		const String name = path.get_slicec('/', 1);
		monitors.insert(path, Monitor(name, base, i, Performance::get_singleton()->get_monitor_type(monitor), nullptr));
	}

	_build_monitor_tree();
}
