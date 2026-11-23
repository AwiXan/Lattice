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

void EditorPerformanceProfiler::_bind_methods() {
	ADD_SIGNAL(MethodInfo("detailed_profiling_toggled", PropertyInfo(Variant::BOOL, "enabled")));
}

void EditorPerformanceProfiler::_on_call_chains_toggled(bool p_pressed) {
	detailed_profiling_enabled = p_pressed;
	emit_signal(SNAME("detailed_profiling_toggled"), p_pressed);
}

void EditorPerformanceProfiler::add_server_profile_metric(const EditorProfiler::Metric &p_metric) {
	last_server_metric = p_metric;
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
		if (detailed_profiling_enabled) {
			emit_signal(SNAME("detailed_profiling_toggled"), true);
		}
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
	DisplayServer::get_singleton()->clipboard_set(_generate_breakdown_report(detailed_profiling_enabled));
}

void EditorPerformanceProfiler::_on_clear_pressed() {
	reset();
}

void EditorPerformanceProfiler::_export_item_subtree(TreeItem *p_item, int p_depth, String &r_text) const {
	if (!p_item) {
		return;
	}
	String indent = String("    ").repeat(p_depth);
	String line;
	if (p_item->has_meta("_row_text")) {
		line = p_item->get_meta("_row_text");
	} else {
		line = p_item->get_text(0);
		if (!p_item->get_text(2).is_empty()) {
			line += " | " + p_item->get_text(2);
		}
		if (!p_item->get_text(3).is_empty()) {
			line += " | " + p_item->get_text(3);
		}
		if (!p_item->get_text(4).is_empty()) {
			line += " | " + p_item->get_text(4);
		}
	}
	r_text += indent + line + "\n";

	TreeItem *child = p_item->get_first_child();
	while (child) {
		_export_item_subtree(child, p_depth + 1, r_text);
		child = child->get_next();
	}
}

void EditorPerformanceProfiler::_copy_selected_item() {
	TreeItem *selected = breakdown_tree->get_selected();
	if (!selected) {
		return;
	}
	if (selected->get_first_child()) {
		String text;
		_export_item_subtree(selected, 0, text);
		DisplayServer::get_singleton()->clipboard_set(text.strip_edges(false, true));
	} else {
		String text_to_copy;
		if (selected->has_meta("_row_text")) {
			text_to_copy = selected->get_meta("_row_text");
		} else if (selected->has_meta("_func_name")) {
			text_to_copy = selected->get_meta("_func_name");
		} else {
			text_to_copy = selected->get_text(0);
		}
		if (!text_to_copy.is_empty()) {
			DisplayServer::get_singleton()->clipboard_set(text_to_copy);
		}
	}
}

void EditorPerformanceProfiler::_breakdown_tree_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::RIGHT) {
		_on_breakdown_tree_rmb(mb->get_position(), MouseButton::RIGHT);
		accept_event();
		return;
	}
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->is_command_or_control_pressed() && k->get_keycode() == Key::C) {
			_copy_selected_item();
			accept_event();
		}
	}
}

void EditorPerformanceProfiler::_on_breakdown_tree_rmb(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	breakdown_tree->grab_focus();
	TreeItem *item = breakdown_tree->get_item_at_position(p_pos);
	if (!item) {
		item = breakdown_tree->get_selected();
	}
	if (!item) {
		item = breakdown_tree->get_root();
	}
	if (!item) {
		return;
	}
	item->select(0);

	if (!breakdown_context_menu) {
		return;
	}

	breakdown_context_menu->clear();
	breakdown_context_menu->reset_size();

	String row_text = item->has_meta("_row_text") ? String(item->get_meta("_row_text")) : item->get_text(0);
	String func_name = item->has_meta("_func_name") ? String(item->get_meta("_func_name")) : item->get_text(0);
	String call_chain = item->has_meta("_call_chain") ? String(item->get_meta("_call_chain")) : func_name;

	if (item->get_first_child()) {
		String cat_display = func_name;
		if (cat_display.length() > 35) {
			cat_display = cat_display.substr(0, 32) + "...";
		}
		breakdown_context_menu->add_item(vformat(TTR("Copy Category and Sub-items ('%s')"), cat_display), CONTEXT_COPY_ROW);
		breakdown_context_menu->add_item(TTR("Copy Only Category Header"), CONTEXT_COPY_CATEGORY_HEADER_ONLY);
	} else {
		String row_display = row_text;
		if (row_display.length() > 45) {
			row_display = row_display.substr(0, 42) + "...";
		}
		breakdown_context_menu->add_item(vformat(TTR("Copy '%s'"), row_display), CONTEXT_COPY_ROW);
		if (!func_name.is_empty() && func_name != row_text) {
			breakdown_context_menu->add_item(vformat(TTR("Copy Function Name ('%s')"), func_name), CONTEXT_COPY_FUNC_NAME);
		}
		if (!call_chain.is_empty() && call_chain != func_name && call_chain != row_text) {
			breakdown_context_menu->add_item(TTR("Copy Call Chain"), CONTEXT_COPY_CALL_CHAIN);
		}
	}
	breakdown_context_menu->add_separator();
	breakdown_context_menu->add_item(TTR("Copy Report with Call Chains"), CONTEXT_COPY_REPORT_WITH_CHAINS);
	breakdown_context_menu->add_item(TTR("Copy Summary Report"), CONTEXT_COPY_REPORT_SUMMARY);

	breakdown_context_menu->set_position(breakdown_tree->get_screen_position() + p_pos);
	breakdown_context_menu->popup();
}

void EditorPerformanceProfiler::_on_context_menu_id_pressed(int p_id) {
	TreeItem *selected = breakdown_tree->get_selected();
	switch (p_id) {
		case CONTEXT_COPY_ROW: {
			_copy_selected_item();
		} break;
		case CONTEXT_COPY_CATEGORY_HEADER_ONLY: {
			if (selected) {
				String text = selected->has_meta("_row_text") ? String(selected->get_meta("_row_text")) : selected->get_text(0);
				DisplayServer::get_singleton()->clipboard_set(text);
			}
		} break;
		case CONTEXT_COPY_FUNC_NAME: {
			if (selected) {
				String text = selected->has_meta("_func_name") ? String(selected->get_meta("_func_name")) : selected->get_text(0);
				DisplayServer::get_singleton()->clipboard_set(text);
			}
		} break;
		case CONTEXT_COPY_CALL_CHAIN: {
			if (selected) {
				String text = selected->has_meta("_call_chain") ? String(selected->get_meta("_call_chain")) : selected->get_text(0);
				DisplayServer::get_singleton()->clipboard_set(text);
			}
		} break;
		case CONTEXT_COPY_REPORT_WITH_CHAINS: {
			DisplayServer::get_singleton()->clipboard_set(_generate_breakdown_report(true));
		} break;
		case CONTEXT_COPY_REPORT_SUMMARY: {
			DisplayServer::get_singleton()->clipboard_set(_generate_breakdown_report(false));
		} break;
	}
}

void EditorPerformanceProfiler::_export_report_children(TreeItem *p_parent, int p_indent, bool p_include_call_chains, String &r_text) const {
	if (!p_parent) {
		return;
	}
	TreeItem *child = p_parent->get_first_child();
	while (child) {
		String c_name = child->get_text(0);
		String c_cat = child->get_text(1);
		String c_time = child->get_text(2);
		String c_pct = child->get_text(3);
		String c_extra = child->get_text(4);

		String prefix = String("    ").repeat(p_indent);
		String extra_str = c_extra.is_empty() ? "" : " [" + c_extra + "]";

		if (c_cat == "STAT") {
			r_text += vformat("%s• %-34s : %s\n", prefix, c_name, c_time);
		} else if (!c_time.is_empty()) {
			r_text += vformat("%s• %-34s : %8s (%5s)%s\n", prefix, c_name, c_time, c_pct, extra_str);
		} else {
			r_text += vformat("%s• %s\n", prefix, c_name);
		}

		if (p_include_call_chains && child->get_first_child()) {
			_export_report_children(child, p_indent + 1, p_include_call_chains, r_text);
		}
		child = child->get_next();
	}
}

String EditorPerformanceProfiler::_generate_breakdown_report(bool p_include_call_chains) const {
	String text;
	text += "======================= FRAME PERFORMANCE REPORT =======================\n";
	if (breakdown_tree && breakdown_tree->get_root()) {
		TreeItem *root = breakdown_tree->get_root();
		text += vformat("%s | %s | %s\n\n", root->get_text(0), root->get_text(2), root->get_text(4));
	}

	if (Performance::get_singleton()) {
		Dictionary d = Performance::get_singleton()->get_frame_breakdown();
		double cpu_ms = d["total_cpu_ms"];
		double gpu_ms = d["total_gpu_ms"];
		double duration = d["frame_duration_ms"];
		double cpu_pct = duration > 0.0 ? (cpu_ms / duration) * 100.0 : 0.0;
		double gpu_pct = duration > 0.0 ? (gpu_ms / duration) * 100.0 : 0.0;
		int draw_calls = d["draw_calls"];
		int primitives = d["primitives"];
		uint64_t vram = d["video_mem_bytes"];

		text += "--- SUMMARY ---\n";
		text += vformat("CPU Time:   %6s ms (%5s%%)\n", rtos(cpu_ms).pad_decimals(2), rtos(cpu_pct).pad_decimals(1));
		text += vformat("GPU Time:   %6s ms (%5s%%)\n", rtos(gpu_ms).pad_decimals(2), rtos(gpu_pct).pad_decimals(1));
		text += vformat("Draw Calls: %d calls | Primitives: %s | VRAM: %s\n\n", draw_calls, String::num_real(primitives / 1000.0).pad_decimals(1) + "k", String::humanize_size(vram));
	}

	if (!breakdown_tree || !breakdown_tree->get_root()) {
		text += "========================================================================\n";
		return text;
	}

	// Traverse the breakdown tree sections:
	TreeItem *root = breakdown_tree->get_root();
	TreeItem *sec = root->get_first_child();
	while (sec) {
		String sec_title = sec->get_text(0);
		text += vformat("--- %s ---\n", sec_title.to_upper());
		_export_report_children(sec, 1, p_include_call_chains, text);
		text += "\n";
		sec = sec->get_next();
	}

	text += "========================================================================\n";
	return text;
}

void EditorPerformanceProfiler::_collect_collapsed_items(TreeItem *p_item, HashSet<String> &r_collapsed) {
	if (!p_item) {
		return;
	}
	if (p_item->is_collapsed() && p_item->has_meta("_key")) {
		r_collapsed.insert(p_item->get_meta("_key"));
	}
	TreeItem *child = p_item->get_first_child();
	while (child) {
		_collect_collapsed_items(child, r_collapsed);
		child = child->get_next();
	}
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
	if (p_values.size() > Performance::TIME_RENDER_CPU) {
		render_cpu_sec = p_values[Performance::TIME_RENDER_CPU];
	} else if (Performance::get_singleton()) {
		render_cpu_sec = double(Performance::get_singleton()->get_frame_render_usec()) / 1000000.0;
	}
	if (p_values.size() > Performance::TIME_SHADER_STALL) {
		shader_wait_sec = p_values[Performance::TIME_SHADER_STALL];
	} else if (Performance::get_singleton()) {
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
	if (gpu_areas.size() > 0) {
		total_gpu_ms = MAX(0.0, gpu_areas[gpu_areas.size() - 1].gpu_msec);
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

	if (breakdown_context_menu && breakdown_context_menu->is_visible()) {
		return;
	}

	// Only update Breakdown Tree periodically or when paused:
	uint64_t now = OS::get_singleton()->get_ticks_msec();
	if (!is_paused && (now - last_breakdown_update_msec < 150 || !breakdown_tree->is_visible_in_tree())) {
		return;
	}
	last_breakdown_update_msec = now;

	String selected_key;
	TreeItem *prev_selected = breakdown_tree->get_selected();
	if (prev_selected && prev_selected->has_meta("_key")) {
		selected_key = prev_selected->get_meta("_key");
	}
	HashSet<String> collapsed_keys;
	_collect_collapsed_items(breakdown_tree->get_root(), collapsed_keys);

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
	root->set_meta("_key", "frame_total");
	root->set_meta("_func_name", "Frame Total");
	root->set_meta("_call_chain", "Frame Total");
	root->set_meta("_row_text", vformat("Frame Total (%d FPS) | %sms | 100.0%%", int(Math::round(fps)), rtos(frame_duration_ms).pad_decimals(2)));
	if (collapsed_keys.has("frame_total")) {
		root->set_collapsed(true);
	}
	if (selected_key == "frame_total") {
		root->select(0);
	}

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
		double d_ms = (i + 1 < gpu_areas.size()) ? MAX(0.0, gpu_areas[i + 1].gpu_msec - gpu_areas[i].gpu_msec) : 0.0;
		if (d_ms > 0.05) {
			hotspot_list.push_back({ gpu_areas[i].name, "GPU", d_ms, 0.0 });
		}
	}

	double denom = frame_duration_ms > 0.001 ? frame_duration_ms : 1.0;

	// Detailed functions and C++ call chains:
	struct NativeCallEntry {
		String target_name;
		double duration_ms = 0.0;
		int calls = 0;
	};

	struct ScriptFuncEntry {
		String script_file;
		String func_name;
		double duration_ms = 0.0;
		int calls = 0;
		int line = 0;
		LocalVector<NativeCallEntry> native_calls;
	};

	LocalVector<ScriptFuncEntry> process_funcs;
	LocalVector<ScriptFuncEntry> physics_funcs;
	LocalVector<NativeCallEntry> server_render_calls;
	LocalVector<NativeCallEntry> server_physics_calls;

	if (last_server_metric.valid && detailed_profiling_enabled) {
		for (int c_idx = 0; c_idx < last_server_metric.categories.size(); c_idx++) {
			const EditorProfiler::Metric::Category &cat = last_server_metric.categories[c_idx];

			if (cat.signature == "script_functions" || cat.name == "Script Functions") {
				HashMap<String, int> process_func_indices;
				HashMap<String, int> physics_func_indices;

				for (int i = 0; i < cat.items.size(); i++) {
					const EditorProfiler::Metric::Category::Item &it = cat.items[i];
					if (it.name.contains(" -> ")) {
						continue;
					}
					ScriptFuncEntry sfe;
					sfe.script_file = it.script.get_file();
					sfe.func_name = it.name;
					sfe.duration_ms = it.total * 1000.0;
					sfe.calls = it.calls;
					sfe.line = it.line;

					String key = sfe.script_file + "::" + sfe.func_name;
					if (sfe.func_name == "_physics_process") {
						physics_func_indices[key] = physics_funcs.size();
						physics_funcs.push_back(sfe);
					} else {
						process_func_indices[key] = process_funcs.size();
						process_funcs.push_back(sfe);
					}
				}

				for (int i = 0; i < cat.items.size(); i++) {
					const EditorProfiler::Metric::Category::Item &it = cat.items[i];
					if (!it.name.contains(" -> ")) {
						continue;
					}
					String caller = it.name.get_slice(" -> ", 0).strip_edges();
					String target = it.name.get_slice(" -> ", 1).strip_edges();
					String script_file = it.script.get_file();
					String key = script_file + "::" + caller;

					NativeCallEntry nce;
					nce.target_name = target;
					nce.duration_ms = it.total * 1000.0;
					nce.calls = it.calls;

					if (caller == "_physics_process") {
						if (physics_func_indices.has(key)) {
							physics_funcs[physics_func_indices[key]].native_calls.push_back(nce);
						} else {
							ScriptFuncEntry sfe;
							sfe.script_file = script_file;
							sfe.func_name = caller;
							sfe.duration_ms = nce.duration_ms;
							sfe.calls = it.calls;
							sfe.line = it.line;
							sfe.native_calls.push_back(nce);
							physics_func_indices[key] = physics_funcs.size();
							physics_funcs.push_back(sfe);
						}
					} else {
						if (process_func_indices.has(key)) {
							process_funcs[process_func_indices[key]].native_calls.push_back(nce);
						} else {
							ScriptFuncEntry sfe;
							sfe.script_file = script_file;
							sfe.func_name = caller;
							sfe.duration_ms = nce.duration_ms;
							sfe.calls = it.calls;
							sfe.line = it.line;
							sfe.native_calls.push_back(nce);
							process_func_indices[key] = process_funcs.size();
							process_funcs.push_back(sfe);
						}
					}

					double pct = (nce.duration_ms / denom) * 100.0;
					if (nce.duration_ms > 0.08 && pct > 4.0) {
						hotspot_list.push_back({ vformat("%s: %s", script_file, it.name), "C++", nce.duration_ms, pct });
					}
				}
			} else if (String(cat.signature).begins_with("categ::")) {
				String srv_name = cat.name;
				for (int i = 0; i < cat.items.size(); i++) {
					const EditorProfiler::Metric::Category::Item &it = cat.items[i];
					NativeCallEntry nce;
					nce.target_name = vformat("%s.%s", srv_name, it.name);
					nce.duration_ms = it.total * 1000.0;
					nce.calls = it.calls;
					if (nce.duration_ms > 0.05) {
						if (srv_name.contains("Rendering")) {
							server_render_calls.push_back(nce);
						} else if (srv_name.contains("Physics")) {
							server_physics_calls.push_back(nce);
						}
					}
				}
			}
		}
	}

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
		sec_hotspots->set_meta("_key", "sec_hotspots");
		sec_hotspots->set_meta("_func_name", "Top Bottlenecks");
		sec_hotspots->set_meta("_call_chain", "Top Bottlenecks");
		sec_hotspots->set_meta("_row_text", "Top Bottlenecks (Sorted)");
		if (collapsed_keys.has("sec_hotspots")) {
			sec_hotspots->set_collapsed(true);
		}
		if (selected_key == "sec_hotspots") {
			sec_hotspots->select(0);
		}

		int limit = MIN(7, (int)hotspot_list.size());
		for (int i = 0; i < limit; i++) {
			TreeItem *item = breakdown_tree->create_item(sec_hotspots);
			item->set_text(0, hotspot_list[i].name);
			item->set_text(1, hotspot_list[i].category);
			item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			item->set_text(2, vformat("%s ms", rtos(hotspot_list[i].duration_ms).pad_decimals(2)));
			item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			item->set_text(3, vformat("%s%%", rtos(hotspot_list[i].percent).pad_decimals(1)));
			item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);

			if (hotspot_list[i].category == "C++") {
				item->set_custom_color(0, Color(0.35, 0.85, 1.0));
				item->set_custom_color(1, Color(0.35, 0.85, 1.0));
			}

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

			String func_name = hotspot_list[i].name;
			if (func_name.contains(" -> ")) {
				func_name = func_name.get_slice(" -> ", 1).strip_edges();
			} else if (func_name.contains(": ")) {
				func_name = func_name.get_slice(": ", 1).strip_edges();
			} else if (func_name.contains("(") && func_name.contains(")")) {
				int s = func_name.find("(");
				int e = func_name.rfind(")");
				if (s >= 0 && e > s) {
					func_name = func_name.substr(s + 1, e - s - 1);
				}
			}
			String h_key = "hotspot_" + hotspot_list[i].name;
			item->set_meta("_key", h_key);
			item->set_meta("_func_name", func_name);
			item->set_meta("_call_chain", hotspot_list[i].name);
			item->set_meta("_row_text", vformat("%s | %sms | %s%%", hotspot_list[i].name, rtos(hotspot_list[i].duration_ms).pad_decimals(2), rtos(hotspot_list[i].percent).pad_decimals(1)));
			if (collapsed_keys.has(h_key)) {
				item->set_collapsed(true);
			}
			if (selected_key == h_key) {
				item->select(0);
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
	sec_cpu->set_meta("_key", "sec_cpu");
	sec_cpu->set_meta("_func_name", "CPU Logic & Subsystems");
	sec_cpu->set_meta("_call_chain", "CPU Logic & Subsystems");
	sec_cpu->set_meta("_row_text", vformat("CPU Logic & Subsystems (%s ms)", rtos(total_cpu_ms).pad_decimals(2)));
	if (collapsed_keys.has("sec_cpu")) {
		sec_cpu->set_collapsed(true);
	}
	if (selected_key == "sec_cpu") {
		sec_cpu->select(0);
	}

	auto add_cpu_item = [&](const String &name, double ms) -> TreeItem * {
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
		String func_name = name;
		if (func_name.contains("(") && func_name.contains(")")) {
			int s = func_name.find("(");
			int e = func_name.rfind(")");
			if (s >= 0 && e > s) {
				func_name = func_name.substr(s + 1, e - s - 1);
			}
		}
		String c_key = "cpu_" + name;
		item->set_meta("_key", c_key);
		item->set_meta("_func_name", func_name);
		item->set_meta("_call_chain", name);
		item->set_meta("_row_text", vformat("%s | %sms | %s%%", name, rtos(ms).pad_decimals(2), rtos(pct).pad_decimals(1)));
		if (collapsed_keys.has(c_key)) {
			item->set_collapsed(true);
		}
		if (selected_key == c_key) {
			item->select(0);
		}
		return item;
	};

	TreeItem *item_process = add_cpu_item("Scripts & Logic (_process)", process_ms);
	if (detailed_profiling_enabled && process_funcs.size() > 0) {
		for (const ScriptFuncEntry &sfe : process_funcs) {
			TreeItem *f_item = breakdown_tree->create_item(item_process);
			f_item->set_text(0, vformat("%s: %s()", sfe.script_file, sfe.func_name));
			f_item->set_tooltip_text(0, vformat("%s:%d\nCalls: %d", sfe.script_file, sfe.line, sfe.calls));
			f_item->set_text(1, "Script");
			f_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			f_item->set_text(2, vformat("%s ms", rtos(sfe.duration_ms).pad_decimals(2)));
			f_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (sfe.duration_ms / denom) * 100.0;
			f_item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			f_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			f_item->set_text(4, vformat("%d calls", sfe.calls));

			String f_key = vformat("proc_func_%s_%s", sfe.script_file, sfe.func_name);
			f_item->set_meta("_key", f_key);
			f_item->set_meta("_func_name", sfe.func_name);
			f_item->set_meta("_call_chain", vformat("%s: %s()", sfe.script_file, sfe.func_name));
			f_item->set_meta("_row_text", vformat("%s: %s | %sms | %s%% | %d calls", sfe.script_file, sfe.func_name, rtos(sfe.duration_ms).pad_decimals(2), rtos(pct).pad_decimals(1), sfe.calls));
			if (collapsed_keys.has(f_key)) {
				f_item->set_collapsed(true);
			}
			if (selected_key == f_key) {
				f_item->select(0);
			}

			for (const NativeCallEntry &nce : sfe.native_calls) {
				TreeItem *nat_item = breakdown_tree->create_item(f_item);
				nat_item->set_text(0, vformat("↳ [C++] %s", nce.target_name));
				nat_item->set_text(1, "C++");
				nat_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
				nat_item->set_text(2, vformat("%s ms", rtos(nce.duration_ms).pad_decimals(2)));
				nat_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
				double nat_pct = (nce.duration_ms / denom) * 100.0;
				nat_item->set_text(3, vformat("%s%%", rtos(nat_pct).pad_decimals(1)));
				nat_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
				nat_item->set_text(4, vformat("[C++ Native] %d calls", nce.calls));
				nat_item->set_custom_color(0, Color(0.35, 0.85, 1.0));
				nat_item->set_custom_color(1, Color(0.35, 0.85, 1.0));

				String nat_key = vformat("proc_nat_%s_%s_%s", sfe.script_file, sfe.func_name, nce.target_name);
				nat_item->set_meta("_key", nat_key);
				nat_item->set_meta("_func_name", nce.target_name);
				nat_item->set_meta("_call_chain", vformat("%s: %s() -> %s", sfe.script_file, sfe.func_name, nce.target_name));
				nat_item->set_meta("_row_text", vformat("%s: %s -> %s | %sms | %s%% | %d calls", sfe.script_file, sfe.func_name, nce.target_name, rtos(nce.duration_ms).pad_decimals(2), rtos(nat_pct).pad_decimals(1), nce.calls));
				if (collapsed_keys.has(nat_key)) {
					nat_item->set_collapsed(true);
				}
				if (selected_key == nat_key) {
					nat_item->select(0);
				}
			}
		}
	} else if (!detailed_profiling_enabled) {
		TreeItem *hint = breakdown_tree->create_item(item_process);
		hint->set_text(0, TTR("(Check 'Call Chains' in toolbar to see function breakdown)"));
		hint->set_custom_color(0, Color(0.6, 0.6, 0.6));
	}

	TreeItem *item_physics = add_cpu_item("Physics Step (_physics_process)", physics_ms);
	if (detailed_profiling_enabled && (physics_funcs.size() > 0 || server_physics_calls.size() > 0)) {
		for (const ScriptFuncEntry &sfe : physics_funcs) {
			TreeItem *f_item = breakdown_tree->create_item(item_physics);
			f_item->set_text(0, vformat("%s: %s()", sfe.script_file, sfe.func_name));
			f_item->set_tooltip_text(0, vformat("%s:%d\nCalls: %d", sfe.script_file, sfe.line, sfe.calls));
			f_item->set_text(1, "Script");
			f_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			f_item->set_text(2, vformat("%s ms", rtos(sfe.duration_ms).pad_decimals(2)));
			f_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (sfe.duration_ms / denom) * 100.0;
			f_item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			f_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			f_item->set_text(4, vformat("%d calls", sfe.calls));

			String f_key = vformat("phys_func_%s_%s", sfe.script_file, sfe.func_name);
			f_item->set_meta("_key", f_key);
			f_item->set_meta("_func_name", sfe.func_name);
			f_item->set_meta("_call_chain", vformat("%s: %s()", sfe.script_file, sfe.func_name));
			f_item->set_meta("_row_text", vformat("%s: %s | %sms | %s%% | %d calls", sfe.script_file, sfe.func_name, rtos(sfe.duration_ms).pad_decimals(2), rtos(pct).pad_decimals(1), sfe.calls));
			if (collapsed_keys.has(f_key)) {
				f_item->set_collapsed(true);
			}
			if (selected_key == f_key) {
				f_item->select(0);
			}

			for (const NativeCallEntry &nce : sfe.native_calls) {
				TreeItem *nat_item = breakdown_tree->create_item(f_item);
				nat_item->set_text(0, vformat("↳ [C++] %s", nce.target_name));
				nat_item->set_text(1, "C++");
				nat_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
				nat_item->set_text(2, vformat("%s ms", rtos(nce.duration_ms).pad_decimals(2)));
				nat_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
				double nat_pct = (nce.duration_ms / denom) * 100.0;
				nat_item->set_text(3, vformat("%s%%", rtos(nat_pct).pad_decimals(1)));
				nat_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
				nat_item->set_text(4, vformat("[C++ Native] %d calls", nce.calls));
				nat_item->set_custom_color(0, Color(0.35, 0.85, 1.0));
				nat_item->set_custom_color(1, Color(0.35, 0.85, 1.0));

				String nat_key = vformat("phys_nat_%s_%s_%s", sfe.script_file, sfe.func_name, nce.target_name);
				nat_item->set_meta("_key", nat_key);
				nat_item->set_meta("_func_name", nce.target_name);
				nat_item->set_meta("_call_chain", vformat("%s: %s() -> %s", sfe.script_file, sfe.func_name, nce.target_name));
				nat_item->set_meta("_row_text", vformat("%s: %s -> %s | %sms | %s%% | %d calls", sfe.script_file, sfe.func_name, nce.target_name, rtos(nce.duration_ms).pad_decimals(2), rtos(nat_pct).pad_decimals(1), nce.calls));
				if (collapsed_keys.has(nat_key)) {
					nat_item->set_collapsed(true);
				}
				if (selected_key == nat_key) {
					nat_item->select(0);
				}
			}
		}
		for (const NativeCallEntry &nce : server_physics_calls) {
			TreeItem *srv_item = breakdown_tree->create_item(item_physics);
			srv_item->set_text(0, vformat("↳ [C++] %s", nce.target_name));
			srv_item->set_text(1, "C++");
			srv_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			srv_item->set_text(2, vformat("%s ms", rtos(nce.duration_ms).pad_decimals(2)));
			srv_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (nce.duration_ms / denom) * 100.0;
			srv_item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			srv_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			srv_item->set_text(4, "[Server C++]");
			srv_item->set_custom_color(0, Color(0.35, 0.85, 1.0));
			srv_item->set_custom_color(1, Color(0.35, 0.85, 1.0));

			String srv_key = "srv_phys_" + nce.target_name;
			srv_item->set_meta("_key", srv_key);
			srv_item->set_meta("_func_name", nce.target_name);
			srv_item->set_meta("_call_chain", nce.target_name);
			srv_item->set_meta("_row_text", vformat("[C++] %s | %sms | %s%% | %d calls", nce.target_name, rtos(nce.duration_ms).pad_decimals(2), rtos(pct).pad_decimals(1), nce.calls));
			if (collapsed_keys.has(srv_key)) {
				srv_item->set_collapsed(true);
			}
			if (selected_key == srv_key) {
				srv_item->select(0);
			}
		}
	}

	TreeItem *item_render = add_cpu_item("Render Prep & Culling (CPU)", render_cpu_ms);
	if (detailed_profiling_enabled && server_render_calls.size() > 0) {
		for (const NativeCallEntry &nce : server_render_calls) {
			TreeItem *srv_item = breakdown_tree->create_item(item_render);
			srv_item->set_text(0, vformat("↳ [C++] %s", nce.target_name));
			srv_item->set_text(1, "C++");
			srv_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			srv_item->set_text(2, vformat("%s ms", rtos(nce.duration_ms).pad_decimals(2)));
			srv_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (nce.duration_ms / denom) * 100.0;
			srv_item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			srv_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			srv_item->set_text(4, "[Server C++]");
			srv_item->set_custom_color(0, Color(0.35, 0.85, 1.0));
			srv_item->set_custom_color(1, Color(0.35, 0.85, 1.0));

			String srv_key = "srv_render_" + nce.target_name;
			srv_item->set_meta("_key", srv_key);
			srv_item->set_meta("_func_name", nce.target_name);
			srv_item->set_meta("_call_chain", nce.target_name);
			srv_item->set_meta("_row_text", vformat("[C++] %s | %sms | %s%% | %d calls", nce.target_name, rtos(nce.duration_ms).pad_decimals(2), rtos(pct).pad_decimals(1), nce.calls));
			if (collapsed_keys.has(srv_key)) {
				srv_item->set_collapsed(true);
			}
			if (selected_key == srv_key) {
				srv_item->select(0);
			}
		}
	}

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
	sec_gpu->set_meta("_key", "sec_gpu");
	sec_gpu->set_meta("_func_name", "GPU Rendering Passes");
	sec_gpu->set_meta("_call_chain", "GPU Rendering Passes");
	sec_gpu->set_meta("_row_text", vformat("GPU Rendering Passes (%s ms)", rtos(total_gpu_ms).pad_decimals(2)));
	if (collapsed_keys.has("sec_gpu")) {
		sec_gpu->set_collapsed(true);
	}
	if (selected_key == "sec_gpu") {
		sec_gpu->select(0);
	}

	if (gpu_areas.is_empty()) {
		TreeItem *empty_item = breakdown_tree->create_item(sec_gpu);
		empty_item->set_text(0, TTR("(Run project with Clustered / Forward+ to profile GPU passes)"));
	} else {
		List<TreeItem *> gpu_stack;
		List<String> path_stack;

		for (int i = 0; i < gpu_areas.size(); i++) {
			String name = gpu_areas[i].name;
			double duration_ms = 0.0;
			if (i + 1 < gpu_areas.size()) {
				duration_ms = MAX(0.0, gpu_areas[i + 1].gpu_msec - gpu_areas[i].gpu_msec);
			}

			if (name.begins_with(">")) {
				String cat_name = name.substr(1);
				TreeItem *parent = gpu_stack.size() ? gpu_stack.back()->get() : sec_gpu;
				TreeItem *cat_item = breakdown_tree->create_item(parent);
				cat_item->set_text(0, cat_name);
				cat_item->set_text(1, "GPU");
				cat_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
				cat_item->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));

				cat_item->set_meta("_accum_gpu_ms", 0.0);
				cat_item->set_meta("_cat_name", cat_name);

				String full_path = (path_stack.size() ? path_stack.back()->get() + "/" : "") + cat_name;
				path_stack.push_back(full_path);
				String key = "gpu_cat_" + full_path;
				cat_item->set_meta("_key", key);
				cat_item->set_meta("_func_name", cat_name);
				cat_item->set_meta("_call_chain", full_path);

				if (collapsed_keys.has(key)) {
					cat_item->set_collapsed(true);
				}
				if (selected_key == key) {
					cat_item->select(0);
				}

				gpu_stack.push_back(cat_item);
				continue;
			}

			if (name.begins_with("<")) {
				if (gpu_stack.size()) {
					TreeItem *cat_item = gpu_stack.back()->get();
					double accum_ms = cat_item->has_meta("_accum_gpu_ms") ? double(cat_item->get_meta("_accum_gpu_ms")) : 0.0;
					double cat_pct = (accum_ms / denom) * 100.0;
					cat_item->set_text(2, vformat("%s ms", rtos(accum_ms).pad_decimals(2)));
					cat_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
					cat_item->set_text(3, vformat("%s%%", rtos(cat_pct).pad_decimals(1)));
					cat_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
					if (cat_pct > 25.0) {
						cat_item->set_text(4, "HOTSPOT");
						cat_item->set_custom_color(4, Color(1.0, 0.35, 0.35));
					} else if (cat_pct > 12.0) {
						cat_item->set_text(4, "HEAVY");
						cat_item->set_custom_color(4, Color(1.0, 0.75, 0.2));
					}
					String cat_name = cat_item->get_meta("_cat_name");
					cat_item->set_meta("_row_text", vformat("%s | %sms | %s%%", cat_name, rtos(accum_ms).pad_decimals(2), rtos(cat_pct).pad_decimals(1)));

					gpu_stack.pop_back();
				}
				if (path_stack.size()) {
					path_stack.pop_back();
				}
				continue;
			}

			// Leaf pass:
			TreeItem *parent = gpu_stack.size() ? gpu_stack.back()->get() : sec_gpu;
			TreeItem *item = breakdown_tree->create_item(parent);
			item->set_text(0, name);
			item->set_text(1, "GPU");
			item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
			item->set_text(2, vformat("%s ms", rtos(duration_ms).pad_decimals(2)));
			item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			double pct = (duration_ms / denom) * 100.0;
			item->set_text(3, vformat("%s%%", rtos(pct).pad_decimals(1)));
			item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			if (pct > 25.0) {
				item->set_text(4, "HOTSPOT");
				item->set_custom_color(4, Color(1.0, 0.35, 0.35));
			} else if (pct > 12.0) {
				item->set_text(4, "HEAVY");
				item->set_custom_color(4, Color(1.0, 0.75, 0.2));
			}

			// Accumulate into all parent scopes on stack:
			for (TreeItem *ancestor : gpu_stack) {
				double a_ms = ancestor->has_meta("_accum_gpu_ms") ? double(ancestor->get_meta("_accum_gpu_ms")) : 0.0;
				ancestor->set_meta("_accum_gpu_ms", a_ms + duration_ms);
			}

			String full_path = (path_stack.size() ? path_stack.back()->get() + "/" : "") + name;
			String gpu_key = "gpu_" + full_path;
			item->set_meta("_key", gpu_key);
			item->set_meta("_func_name", name);
			item->set_meta("_call_chain", full_path);
			item->set_meta("_row_text", vformat("%s | %sms | %s%%", name, rtos(duration_ms).pad_decimals(2), rtos(pct).pad_decimals(1)));
			if (collapsed_keys.has(gpu_key)) {
				item->set_collapsed(true);
			}
			if (selected_key == gpu_key) {
				item->select(0);
			}
		}

		while (gpu_stack.size()) {
			TreeItem *cat_item = gpu_stack.back()->get();
			double accum_ms = cat_item->has_meta("_accum_gpu_ms") ? double(cat_item->get_meta("_accum_gpu_ms")) : 0.0;
			double cat_pct = (accum_ms / denom) * 100.0;
			cat_item->set_text(2, vformat("%s ms", rtos(accum_ms).pad_decimals(2)));
			cat_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			cat_item->set_text(3, vformat("%s%%", rtos(cat_pct).pad_decimals(1)));
			cat_item->set_text_alignment(3, HORIZONTAL_ALIGNMENT_RIGHT);
			String cat_name = cat_item->get_meta("_cat_name");
			cat_item->set_meta("_row_text", vformat("%s | %sms | %s%%", cat_name, rtos(accum_ms).pad_decimals(2), rtos(cat_pct).pad_decimals(1)));
			gpu_stack.pop_back();
		}
	}

	// 4. Draw & Memory Metrics:
	TreeItem *sec_res = breakdown_tree->create_item(root);
	sec_res->set_text(0, TTR("Draw & Memory Metrics"));
	sec_res->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	if (has_theme_icon(SNAME("Image"), SNAME("EditorIcons"))) {
		sec_res->set_icon(0, get_theme_icon(SNAME("Image"), SNAME("EditorIcons")));
	}
	sec_res->set_meta("_key", "sec_res");
	sec_res->set_meta("_func_name", "Draw & Memory Metrics");
	sec_res->set_meta("_call_chain", "Draw & Memory Metrics");
	sec_res->set_meta("_row_text", "Draw & Memory Metrics");
	if (collapsed_keys.has("sec_res")) {
		sec_res->set_collapsed(true);
	}
	if (selected_key == "sec_res") {
		sec_res->select(0);
	}

	auto add_metric_item = [&](const String &name, const String &val) {
		TreeItem *item = breakdown_tree->create_item(sec_res);
		item->set_text(0, name);
		item->set_text(1, "STAT");
		item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
		item->set_text(2, val);
		item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
		String m_key = "stat_" + name;
		item->set_meta("_key", m_key);
		item->set_meta("_func_name", name);
		item->set_meta("_call_chain", name);
		item->set_meta("_row_text", vformat("%s | STAT | %s", name, val));
		if (collapsed_keys.has(m_key)) {
			item->set_collapsed(true);
		}
		if (selected_key == m_key) {
			item->select(0);
		}
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

	btn_call_chains = memnew(CheckBox(TTR("Call Chains")));
	btn_call_chains->set_pressed(detailed_profiling_enabled);
	btn_call_chains->set_tooltip_text(TTR("Profile script functions and C++ call chains in real-time"));
	btn_call_chains->connect("toggled", callable_mp(this, &EditorPerformanceProfiler::_on_call_chains_toggled));
	header_bar->add_child(btn_call_chains);

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

	breakdown_context_menu = memnew(PopupMenu);
	add_child(breakdown_context_menu);
	breakdown_context_menu->connect("id_pressed", callable_mp(this, &EditorPerformanceProfiler::_on_context_menu_id_pressed));

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
	breakdown_tree->set_select_mode(Tree::SELECT_ROW);
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
	breakdown_tree->set_allow_rmb_select(true);
	breakdown_tree->set_allow_reselect(true);
	breakdown_tree->connect("item_mouse_selected", callable_mp(this, &EditorPerformanceProfiler::_on_breakdown_tree_rmb));
	breakdown_tree->connect("empty_clicked", callable_mp(this, &EditorPerformanceProfiler::_on_breakdown_tree_rmb));
	breakdown_tree->connect(SceneStringName(gui_input), callable_mp(this, &EditorPerformanceProfiler::_breakdown_tree_gui_input));
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
