/**************************************************************************/
/*  editor_profiler.cpp                                                   */
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

#include "editor_profiler.h"

#include "core/io/image.h"
#include "core/object/callable_mp.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/resources/image_texture.h"
#include "servers/display/display_server.h"

void EditorProfiler::_make_metric_ptrs(Metric &m) {
	for (int i = 0; i < m.categories.size(); i++) {
		m.category_ptrs[m.categories[i].signature] = &m.categories.write[i];
		for (int j = 0; j < m.categories[i].items.size(); j++) {
			m.item_ptrs[m.categories[i].items[j].signature] = &m.categories.write[i].items.write[j];
		}
	}
}

const EditorProfiler::Metric &EditorProfiler::_get_frame_metric(int index) const {
	return frame_metrics[(frame_metrics.size() + last_metric - (total_metrics - 1) + index) % frame_metrics.size()];
}

void EditorProfiler::add_frame_metric(const Metric &p_metric, bool p_final) {
	++last_metric;
	if (last_metric >= frame_metrics.size()) {
		last_metric = 0;
	}

	total_metrics++;
	if (total_metrics > frame_metrics.size()) {
		total_metrics = frame_metrics.size();
	}

	frame_metrics.write[last_metric] = p_metric;
	frame_metrics.write[last_metric].nodes = last_node_frame;
	_make_metric_ptrs(frame_metrics.write[last_metric]);

	updating_frame = true;
	clear_button->set_disabled(false);
	cursor_metric_edit->set_editable(true);
	cursor_metric_edit->set_max(p_metric.frame_number);
	cursor_metric_edit->set_min(_get_frame_metric(0).frame_number);

	if (!seeking) {
		cursor_metric_edit->set_value(p_metric.frame_number);
	}

	updating_frame = false;

	if (frame_delay->is_stopped()) {
		frame_delay->set_wait_time(p_final ? 0.1 : 1);
		frame_delay->start();
	}

	if (plot_delay->is_stopped()) {
		plot_delay->set_wait_time(0.1);
		plot_delay->start();
	}
}

void EditorProfiler::add_node_frame_metric(const Vector<NodeMetric> &p_nodes) {
	last_node_frame = p_nodes;
}

void EditorProfiler::clear() {
	int metric_size = EDITOR_GET("debugger/profiler_frame_history_size");
	metric_size = CLAMP(metric_size, 60, 10000);
	frame_metrics.clear();
	frame_metrics.resize(metric_size);
	last_node_frame.clear();
	total_metrics = 0;
	last_metric = -1;
	variables->clear();
	activate->set_pressed(false);
	_update_button_text();
}

String EditorProfiler::_get_time_as_text(const Metric &m, float p_time, int p_calls) {
	const String &lang = _get_locale();
	const TranslationServer *ts = TranslationServer::get_singleton();

	switch (display_mode->get_selected()) {
		case DISPLAY_FRAME_TIME: {
			return ts->format_number(rtos(p_time * 1000).pad_decimals(2), lang) + " " + TTR("ms");
		}
		case DISPLAY_AVERAGE_TIME: {
			if (p_calls == 0) {
				return ts->format_number(rtos(0), lang) + " " + TTR("ms");
			} else {
				return ts->format_number(rtos((p_time / p_calls) * 1000).pad_decimals(2), lang) + " " + TTR("ms");
			}
		}
		case DISPLAY_FRAME_PERCENT: {
			if (m.frame_time == 0) {
				return ts->format_number(rtos(0), lang) + " %";
			} else {
				return ts->format_number(rtos((p_time / m.frame_time) * 100).pad_decimals(2), lang) + " %";
			}
		}
		case DISPLAY_PHYSICS_FRAME_PERCENT: {
			if (m.physics_frame_time == 0) {
				return ts->format_number(rtos(0), lang) + " %";
			} else {
				return ts->format_number(rtos((p_time / m.physics_frame_time) * 100).pad_decimals(2), lang) + " %";
			}
		}
		default: {
			return "";
		}
	}
}

Color EditorProfiler::_get_color_from_signature(const StringName &p_signature) const {
	Color c = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	c.set_hsv(Math::fmod(float(p_signature.hash()) / 1000.0f, 0.9f), c.get_s() * 0.85f, c.get_v());
	return c;
}

int EditorProfiler::_get_zoom_left_border() const {
	const int max_profiles_shown = frame_metrics.size() / Math::exp(graph_zoom);
	return CLAMP(zoom_center - max_profiles_shown / 2, 0, frame_metrics.size() - max_profiles_shown);
}

void EditorProfiler::_item_edited() {
	if (updating_frame) {
		return;
	}

	TreeItem *item = variables->get_edited();
	if (!item) {
		return;
	}
	StringName signature = item->get_metadata(0);
	bool checked = item->is_checked(0);

	if (checked) {
		plot_sigs.insert(signature);
	} else {
		plot_sigs.erase(signature);
	}

	if (!frame_delay->is_processing()) {
		frame_delay->set_wait_time(0.1);
		frame_delay->start();
	}

	_update_plot();
}

void EditorProfiler::_item_activated() {
	TreeItem *selected = variables->get_selected();
	if (!selected) {
		return;
	}
	if (selected->has_meta("_node_id")) {
		ObjectID id = ObjectID(uint64_t(selected->get_meta("_node_id")));
		if (id.is_valid()) {
			emit_signal(SNAME("inspect_node"), id);
		}
	}
}

void EditorProfiler::_on_search_changed(const String &p_text) {
	search_filter = p_text.strip_edges().to_lower();
	_update_frame();
}

void EditorProfiler::_on_target_changed(int p_target) {
	_update_frame();
}

void EditorProfiler::_on_copy_pressed() {
	if (total_metrics == 0) {
		return;
	}
	int cursor_metric = cursor_metric_edit->get_value() - _get_frame_metric(0).frame_number;
	const Metric &m = _get_frame_metric(cursor_metric);

	String text;
	text += "======================= CPU / PROFILER REPORT =======================\n";
	text += vformat("Frame #: %d | Frame Time: %s ms | Physics Frame: %s ms\n\n", m.frame_number, rtos(m.frame_time * 1000.0f).pad_decimals(2), rtos(m.physics_frame_time * 1000.0f).pad_decimals(2));

	if (m.categories.size() > 0) {
		text += "--- TOP FUNCTIONS ---\n";
		for (int i = 0; i < m.categories.size(); i++) {
			for (int j = 0; j < m.categories[i].items.size(); j++) {
				const Metric::Category::Item &it = m.categories[i].items[j];
				float time_ms = it.total * 1000.0f;
				if (time_ms > 0.05f) {
					text += vformat(" [%s] %-35s : %6s ms (%d calls)\n", m.categories[i].name, it.name, rtos(time_ms).pad_decimals(2), it.calls);
				}
			}
		}
	}

	if (m.nodes.size() > 0) {
		text += "\n--- TOP SCENE NODES (Ranked by CPU Process/Physics) ---\n";
		int limit = MIN(15, (int)m.nodes.size());
		for (int i = 0; i < limit; i++) {
			const NodeMetric &nm = m.nodes[i];
			float total_ms = nm.total_time;
			float pct = m.frame_time > 0.0001f ? (total_ms / (m.frame_time * 1000.0f)) * 100.0f : 0.0f;
			text += vformat(" %2d. %-40s [%-18s] : %6s ms (%4s%%) [%d calls]\n", i + 1, String(nm.path), nm.class_name, rtos(total_ms).pad_decimals(2), rtos(pct).pad_decimals(1), nm.process_calls + nm.physics_calls);
		}
	}
	text += "=====================================================================\n";
	DisplayServer::get_singleton()->clipboard_set(text);
}

void EditorProfiler::_update_plot() {
	const int w = MAX(1, graph->get_size().width);
	const int h = MAX(1, graph->get_size().height);
	bool reset_texture = false;
	const int desired_len = w * h * 4;

	if (graph_image.size() != desired_len) {
		reset_texture = true;
		graph_image.resize(desired_len);
	}

	uint8_t *wr = graph_image.ptrw();
	const Color background_color = get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor));

	for (int i = 0; i < desired_len; i += 4) {
		wr[i + 0] = Math::fast_ftoi(background_color.r * 255);
		wr[i + 1] = Math::fast_ftoi(background_color.g * 255);
		wr[i + 2] = Math::fast_ftoi(background_color.b * 255);
		wr[i + 3] = 255;
	}

	const bool use_self = display_time->get_selected() == DISPLAY_SELF_TIME;
	float highest = 0;

	for (int i = 0; i < total_metrics; i++) {
		const Metric &m = _get_frame_metric(i);

		for (const StringName &E : plot_sigs) {
			HashMap<StringName, Metric::Category *>::ConstIterator F = m.category_ptrs.find(E);
			if (F) {
				highest = MAX(F->value->total_time, highest);
			}

			HashMap<StringName, Metric::Category::Item *>::ConstIterator G = m.item_ptrs.find(E);
			if (G) {
				if (use_self) {
					highest = MAX(G->value->self, highest);
				} else {
					highest = MAX(G->value->total, highest);
				}
			}

			if (String(E).begins_with("node::")) {
				String node_path = String(E).replace_first("node::", "");
				for (int n_idx = 0; n_idx < m.nodes.size(); n_idx++) {
					if (m.nodes[n_idx].path == node_path) {
						highest = MAX(m.nodes[n_idx].total_time / 1000.0f, highest);
						break;
					}
				}
			}
		}
	}

	if (highest > 0) {
		highest *= 1.2;
		graph_height = highest;

		Vector<int> columnv;
		columnv.resize(h * 4);
		int *column = columnv.ptrw();

		HashMap<StringName, int> prev_plots;

		const int max_profiles_shown = frame_metrics.size() / Math::exp(graph_zoom);
		const int left_border = _get_zoom_left_border();
		const int profiles_drawn = CLAMP(total_metrics - left_border, 0, max_profiles_shown);
		const int pixel_cols = (profiles_drawn * w) / max_profiles_shown - 1;

		for (int i = 0; i < pixel_cols; i++) {
			for (int j = 0; j < h * 4; j++) {
				column[j] = 0;
			}

			int current = (i * max_profiles_shown / w) + left_border;

			for (const StringName &E : plot_sigs) {
				const Metric &m = _get_frame_metric(current);

				float value = 0;

				HashMap<StringName, Metric::Category *>::ConstIterator F = m.category_ptrs.find(E);
				if (F) {
					value = F->value->total_time;
				}

				HashMap<StringName, Metric::Category::Item *>::ConstIterator G = m.item_ptrs.find(E);
				if (G) {
					if (use_self) {
						value = G->value->self;
					} else {
						value = G->value->total;
					}
				}

				if (String(E).begins_with("node::")) {
					String node_path = String(E).replace_first("node::", "");
					for (int n_idx = 0; n_idx < m.nodes.size(); n_idx++) {
						if (m.nodes[n_idx].path == node_path) {
							value = m.nodes[n_idx].total_time / 1000.0f;
							break;
						}
					}
				}

				int plot_pos = CLAMP(int(value * h / highest), 0, h - 1);

				int prev_plot = plot_pos;
				HashMap<StringName, int>::Iterator H = prev_plots.find(E);
				if (H) {
					prev_plot = H->value;
					H->value = plot_pos;
				} else {
					prev_plots[E] = plot_pos;
				}

				plot_pos = h - plot_pos - 1;
				prev_plot = h - prev_plot - 1;

				if (prev_plot > plot_pos) {
					SWAP(prev_plot, plot_pos);
				}

				Color col = _get_color_from_signature(E);

				for (int j = prev_plot; j <= plot_pos; j++) {
					column[j * 4 + 0] += Math::fast_ftoi(CLAMP(col.r * 255, 0, 255));
					column[j * 4 + 1] += Math::fast_ftoi(CLAMP(col.g * 255, 0, 255));
					column[j * 4 + 2] += Math::fast_ftoi(CLAMP(col.b * 255, 0, 255));
					column[j * 4 + 3] += 1;
				}
			}

			for (int j = 0; j < h * 4; j += 4) {
				const int a = column[j + 3];
				if (a > 0) {
					column[j + 0] /= a;
					column[j + 1] /= a;
					column[j + 2] /= a;
				}

				const uint8_t red = uint8_t(column[j + 0]);
				const uint8_t green = uint8_t(column[j + 1]);
				const uint8_t blue = uint8_t(column[j + 2]);
				const bool is_filled = red >= 1 || green >= 1 || blue >= 1;
				const int widx = ((j >> 2) * w + i) * 4;

				wr[widx + 0] = is_filled ? red : Math::fast_ftoi(background_color.r * 255);
				wr[widx + 1] = is_filled ? green : Math::fast_ftoi(background_color.g * 255);
				wr[widx + 2] = is_filled ? blue : Math::fast_ftoi(background_color.b * 255);
				wr[widx + 3] = 255;
			}
		}
	}

	Ref<Image> img = Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, graph_image);

	if (reset_texture) {
		if (graph_texture.is_null()) {
			graph_texture.instantiate();
		}
		graph_texture->set_image(img);
	}

	graph_texture->update(img);
	graph->set_texture(graph_texture);
	graph->queue_redraw();
}

void EditorProfiler::_update_frame() {
	int cursor_metric = cursor_metric_edit->get_value() - _get_frame_metric(0).frame_number;

	updating_frame = true;
	variables->clear();

	TreeItem *root = variables->create_item();
	const Metric &m = _get_frame_metric(cursor_metric);

	int dtime = display_time->get_selected();
	int target = profile_target ? profile_target->get_selected() : TARGET_FUNCTIONS;

	// 1. Script & Server Functions:
	if (target == TARGET_FUNCTIONS || target == TARGET_ALL) {
		for (int i = 0; i < m.categories.size(); i++) {
			bool cat_matches = search_filter.is_empty() || String(m.categories[i].name).to_lower().contains(search_filter);

			bool any_child_matches = false;
			if (!cat_matches && !search_filter.is_empty()) {
				for (int j = 0; j < m.categories[i].items.size(); j++) {
					if (m.categories[i].items[j].name.to_lower().contains(search_filter) || m.categories[i].items[j].script.to_lower().contains(search_filter)) {
						any_child_matches = true;
						break;
					}
				}
			}

			if (!cat_matches && !any_child_matches && !search_filter.is_empty()) {
				continue;
			}

			TreeItem *category = variables->create_item(root);
			category->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
			category->set_editable(0, true);
			category->set_metadata(0, m.categories[i].signature);
			category->set_text(0, String(m.categories[i].name));
			category->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
			category->set_text(1, _get_time_as_text(m, m.categories[i].total_time, 1));
			category->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);

			if (plot_sigs.has(m.categories[i].signature)) {
				category->set_checked(0, true);
				category->set_custom_color(0, _get_color_from_signature(m.categories[i].signature));
			}

			for (int j = 0; j < m.categories[i].items.size(); j++) {
				const Metric::Category::Item &it = m.categories[i].items[j];

				if (it.internal == it.total && !display_internal_profiles->is_pressed() && m.categories[i].name == "Script Functions") {
					continue;
				}

				if (!search_filter.is_empty() && !cat_matches) {
					if (!it.name.to_lower().contains(search_filter) && !it.script.to_lower().contains(search_filter)) {
						continue;
					}
				}

				TreeItem *item = variables->create_item(category);
				item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
				item->set_editable(0, true);
				item->set_text(0, it.name);
				item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
				item->set_metadata(0, it.signature);
				item->set_metadata(1, it.script);
				item->set_metadata(2, it.line);
				item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
				item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
				item->set_tooltip_text(0, it.name + "\n" + it.script + ":" + itos(it.line));

				float time = dtime == DISPLAY_SELF_TIME ? it.self : it.total;
				if (dtime == DISPLAY_SELF_TIME && !display_internal_profiles->is_pressed()) {
					time += it.internal;
				}

				item->set_text(1, _get_time_as_text(m, time, it.calls));
				item->set_text(2, itos(it.calls));

				float denom = m.frame_time > 0.0001f ? m.frame_time : 1.0f;
				float pct = (time / denom) * 100.0f;
				if (pct > 20.0f) {
					item->set_text(3, "CRITICAL (>20%)");
					item->set_custom_color(3, Color(1.0f, 0.35f, 0.35f));
					item->set_icon(3, get_editor_theme_icon(SNAME("StatusError")));
				} else if (pct > 10.0f) {
					item->set_text(3, "HEAVY (>10%)");
					item->set_custom_color(3, Color(1.0f, 0.75f, 0.2f));
					item->set_icon(3, get_editor_theme_icon(SNAME("StatusWarning")));
				} else {
					item->set_text(3, "OK");
					item->set_custom_color(3, Color(0.4f, 0.85f, 0.4f));
					item->set_icon(3, get_editor_theme_icon(SNAME("StatusSuccess")));
				}

				if (plot_sigs.has(it.signature)) {
					item->set_checked(0, true);
					item->set_custom_color(0, _get_color_from_signature(it.signature));
				}
			}
		}
	}

	// 2. Active Scene Nodes:
	if ((target == TARGET_NODES || target == TARGET_ALL) && m.nodes.size() > 0) {
		TreeItem *nodes_cat = variables->create_item(root);
		nodes_cat->set_text(0, TTR("Scene Nodes (Active Instances)"));
		nodes_cat->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
		nodes_cat->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
		if (has_theme_icon(SNAME("NodeTree"), SNAME("EditorIcons"))) {
			nodes_cat->set_icon(0, get_theme_icon(SNAME("NodeTree"), SNAME("EditorIcons")));
		}

		for (int i = 0; i < m.nodes.size(); i++) {
			const NodeMetric &nm = m.nodes[i];
			String path_str = String(nm.path);
			String class_str = nm.class_name;

			if (!search_filter.is_empty()) {
				if (!path_str.to_lower().contains(search_filter) && !class_str.to_lower().contains(search_filter)) {
					continue;
				}
			}

			TreeItem *node_item = variables->create_item(nodes_cat);
			node_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
			node_item->set_editable(0, true);
			node_item->set_text(0, path_str);
			node_item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);

			StringName sig = "node::" + path_str;
			node_item->set_metadata(0, sig);
			node_item->set_meta("_node_id", nm.id);

			if (has_theme_icon(nm.class_name, SNAME("EditorIcons"))) {
				node_item->set_icon(0, get_theme_icon(nm.class_name, SNAME("EditorIcons")));
			}

			node_item->set_tooltip_text(0, vformat(TTR("Node Path: %s\nClass: %s\nProcess Time: %s ms (%d calls)\nPhysics Time: %s ms (%d calls)\n(Double click to inspect)"),
					path_str, class_str, rtos(nm.process_time).pad_decimals(2), nm.process_calls, rtos(nm.physics_time).pad_decimals(2), nm.physics_calls));

			float total_ms = nm.total_time;
			int total_calls = nm.process_calls + nm.physics_calls;

			node_item->set_text(1, _get_time_as_text(m, total_ms / 1000.0f, total_calls));
			node_item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);

			node_item->set_text(2, itos(total_calls));
			node_item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);

			float denom = m.frame_time > 0.0001f ? (m.frame_time * 1000.0f) : 1.0f;
			float npct = (total_ms / denom) * 100.0f;
			if (npct > 20.0f) {
				node_item->set_text(3, "HOTSPOT (>20%)");
				node_item->set_custom_color(3, Color(1.0f, 0.35f, 0.35f));
				node_item->set_icon(3, get_editor_theme_icon(SNAME("StatusError")));
			} else if (npct > 10.0f) {
				node_item->set_text(3, "HEAVY (>10%)");
				node_item->set_custom_color(3, Color(1.0f, 0.75f, 0.2f));
				node_item->set_icon(3, get_editor_theme_icon(SNAME("StatusWarning")));
			} else {
				node_item->set_text(3, "OK");
				node_item->set_custom_color(3, Color(0.4f, 0.85f, 0.4f));
				node_item->set_icon(3, get_editor_theme_icon(SNAME("StatusSuccess")));
			}

			if (plot_sigs.has(sig)) {
				node_item->set_checked(0, true);
				node_item->set_custom_color(0, _get_color_from_signature(sig));
			}
		}
	}

	updating_frame = false;
}

void EditorProfiler::_update_button_text() {
	if (activate->is_pressed()) {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Stop")));
		activate->set_text(TTRC("Stop"));
	} else {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Play")));
		activate->set_text(TTRC("Start"));
	}
}

void EditorProfiler::_activate_pressed() {
	_update_button_text();

	if (activate->is_pressed()) {
		_clear_pressed();
	}

	emit_signal(SNAME("enable_profiling"), activate->is_pressed());
}

void EditorProfiler::_clear_pressed() {
	clear_button->set_disabled(true);
	clear();
	_update_plot();
}

void EditorProfiler::_internal_profiles_pressed() {
	_combo_changed(0);
}

void EditorProfiler::_autostart_toggled(bool p_toggled_on) {
	EditorSettings::get_singleton()->set_project_metadata("debug_options", "autostart_profiler", p_toggled_on);
	EditorRunBar::get_singleton()->update_profiler_autostart_indicator();
}

void EditorProfiler::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (is_ready()) {
				_update_frame();
			}
			[[fallthrough]];
		}
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			activate->set_button_icon(get_editor_theme_icon(SNAME("Play")));
			clear_button->set_button_icon(get_editor_theme_icon(SNAME("Clear")));
			if (btn_copy && has_theme_icon(SNAME("ActionCopy"), SNAME("EditorIcons"))) {
				btn_copy->set_button_icon(get_theme_icon(SNAME("ActionCopy"), SNAME("EditorIcons")));
			}

			theme_cache.seek_line_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
			theme_cache.seek_line_color.a = 0.8;
			theme_cache.seek_line_hover_color = theme_cache.seek_line_color;
			theme_cache.seek_line_hover_color.a = 0.4;

			if (total_metrics > 0) {
				_update_plot();
			}
		} break;
	}
}

void EditorProfiler::_graph_tex_draw() {
	if (total_metrics == 0) {
		return;
	}

	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label")) * 0.8;

	// Frame budget reference lines:
	if (graph_height > 0.001f) {
		float h = graph->get_size().y;
		float w = graph->get_size().x;

		// 60 FPS (16.67ms = 0.01667s):
		float target_60 = 0.01667f;
		if (target_60 <= graph_height) {
			float y_60 = h * (1.0f - (target_60 / graph_height));
			graph->draw_line(Vector2(0, y_60), Vector2(w, y_60), Color(0.2, 0.8, 0.4, 0.4), 1.0);
			graph->draw_string(font, Vector2(w - 95 * EDSCALE, y_60 - 2), "60 FPS (16.7ms)", HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(0.2, 0.8, 0.4, 0.7));
		}

		// 30 FPS (33.33ms = 0.03333s):
		float target_30 = 0.03333f;
		if (target_30 <= graph_height) {
			float y_30 = h * (1.0f - (target_30 / graph_height));
			graph->draw_line(Vector2(0, y_30), Vector2(w, y_30), Color(0.9, 0.5, 0.15, 0.4), 1.0);
			graph->draw_string(font, Vector2(w - 95 * EDSCALE, y_30 - 2), "30 FPS (33.3ms)", HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(0.9, 0.5, 0.15, 0.7));
		}
	}

	if (seeking) {
		int frame = cursor_metric_edit->get_value() - _get_frame_metric(0).frame_number;
		frame = frame - _get_zoom_left_border() + 1;
		int cur_x = (frame * graph->get_size().width * Math::exp(graph_zoom)) / frame_metrics.size();
		cur_x = CLAMP(cur_x, 0, graph->get_size().width);
		graph->draw_line(Vector2(cur_x, 0), Vector2(cur_x, graph->get_size().y), theme_cache.seek_line_color);
	}
	if (hover_metric > -1) {
		int cur_x = (2 * hover_metric + 1) * graph->get_size().x / (2 * frame_metrics.size()) + 1;
		graph->draw_line(Vector2(cur_x, 0), Vector2(cur_x, graph->get_size().y), theme_cache.seek_line_hover_color);
	}
}

void EditorProfiler::_graph_tex_mouse_exit() {
	hover_metric = -1;
	graph->queue_redraw();
}

void EditorProfiler::_cursor_metric_changed(double) {
	if (updating_frame) {
		return;
	}

	graph->queue_redraw();
	_update_frame();
}

void EditorProfiler::_graph_tex_input(const Ref<InputEvent> &p_ev) {
	if (last_metric < 0) {
		return;
	}

	Ref<InputEventMouse> me = p_ev;
	Ref<InputEventMouseButton> mb = p_ev;
	Ref<InputEventMouseMotion> mm = p_ev;
	MouseButton button_idx = mb.is_valid() ? mb->get_button_index() : MouseButton();

	if (
			(mb.is_valid() && button_idx == MouseButton::LEFT && mb->is_pressed()) ||
			(mm.is_valid())) {
		int x = me->get_position().x - 1;
		hover_metric = x * frame_metrics.size() / graph->get_size().width;

		x = x * frame_metrics.size() / graph->get_size().width;
		x = x / Math::exp(graph_zoom) + _get_zoom_left_border();
		x = CLAMP(x, 0, frame_metrics.size() - 1);

		if (mb.is_valid() || (mm->get_button_mask().has_flag(MouseButtonMask::LEFT))) {
			updating_frame = true;
			cursor_metric_edit->set_value(_get_frame_metric(x).frame_number);
			updating_frame = false;
			seeking = true;
			emit_signal(SNAME("break_request"));
		}

		graph->queue_redraw();
		_update_frame();
	}

	if (mb.is_valid() && (button_idx == MouseButton::WHEEL_UP || button_idx == MouseButton::WHEEL_DOWN)) {
		float step = 0.1f * mb->get_factor();
		if (button_idx == MouseButton::WHEEL_DOWN) {
			step = -step;
		}

		int x = me->get_position().x - 1;
		x = x * frame_metrics.size() / graph->get_size().width;
		x = x / Math::exp(graph_zoom) + _get_zoom_left_border();
		x = CLAMP(x, 0, frame_metrics.size() - 1);
		zoom_center = x;

		graph_zoom = CLAMP(graph_zoom + step, 0.0f, 4.0f);
		_update_plot();
	}

	if (mm.is_valid() && mm->get_button_mask().has_flag(MouseButtonMask::MIDDLE)) {
		pan_accumulator += mm->get_relative().x;
		if (Math::abs(pan_accumulator) > 1.0f) {
			int delta = int(pan_accumulator);
			pan_accumulator -= delta;
			zoom_center = CLAMP(zoom_center - delta, 0, frame_metrics.size() - 1);
			_update_plot();
		}
	}
}

void EditorProfiler::disable_seeking() {
	seeking = false;
	graph->queue_redraw();
}

void EditorProfiler::_combo_changed(int) {
	_update_frame();
	_update_plot();
}

void EditorProfiler::_bind_methods() {
	ADD_SIGNAL(MethodInfo("enable_profiling", PropertyInfo(Variant::BOOL, "enable")));
	ADD_SIGNAL(MethodInfo("break_request"));
	ADD_SIGNAL(MethodInfo("inspect_node", PropertyInfo(Variant::INT, "object_id")));
}

void EditorProfiler::set_enabled(bool p_enable, bool p_clear) {
	activate->set_disabled(!p_enable);
	if (p_clear) {
		clear();
	}
}

void EditorProfiler::set_profiling(bool p_pressed) {
	activate->set_pressed(p_pressed);
	_update_button_text();
	emit_signal(SNAME("enable_profiling"), activate->is_pressed());
}

bool EditorProfiler::is_profiling() {
	return activate->is_pressed();
}

Vector<Vector<String>> EditorProfiler::get_data_as_csv() const {
	Vector<Vector<String>> res;

	if (total_metrics == 0) {
		return res;
	}

	HashSet<StringName> possible_signatures;
	for (int i = 0; i < total_metrics; i++) {
		const Metric &m = _get_frame_metric(i);
		for (const KeyValue<StringName, Metric::Category *> &E : m.category_ptrs) {
			possible_signatures.insert(E.key);
		}
		for (const KeyValue<StringName, Metric::Category::Item *> &E : m.item_ptrs) {
			possible_signatures.insert(E.key);
		}
	}

	Vector<String> signatures;
	signatures.resize(possible_signatures.size());
	HashMap<StringName, int> sig_map;
	int sig_index = 0;
	for (const StringName &E : possible_signatures) {
		signatures.write[sig_index] = E;
		sig_map[E] = sig_index;
		sig_index++;
	}
	res.push_back(signatures);

	Vector<String> values;
	int index = last_metric;

	for (int i = 0; i < frame_metrics.size(); i++) {
		++index;
		if (index >= frame_metrics.size()) {
			index = 0;
		}

		const Metric &m = frame_metrics[index];
		if (!m.valid) {
			continue;
		}

		values.clear();
		values.resize(possible_signatures.size());

		for (const KeyValue<StringName, Metric::Category *> &E : m.category_ptrs) {
			values.write[sig_map[E.key]] = String::num_real(E.value->total_time);
		}
		for (const KeyValue<StringName, Metric::Category::Item *> &E : m.item_ptrs) {
			values.write[sig_map[E.key]] = String::num_real(E.value->total);
		}

		res.push_back(values);
	}

	return res;
}

EditorProfiler::EditorProfiler() {
	HBoxContainer *hb = memnew(HBoxContainer);
	hb->add_theme_constant_override(SNAME("separation"), 8 * EDSCALE);
	add_child(hb);

	FlowContainer *container = memnew(FlowContainer);
	container->set_h_size_flags(SIZE_EXPAND_FILL);
	container->add_theme_constant_override(SNAME("h_separation"), 8 * EDSCALE);
	container->add_theme_constant_override(SNAME("v_separation"), 2 * EDSCALE);
	hb->add_child(container);

	activate = memnew(Button);
	activate->set_toggle_mode(true);
	activate->set_disabled(true);
	activate->set_text(TTRC("Start"));
	activate->connect(SceneStringName(pressed), callable_mp(this, &EditorProfiler::_activate_pressed));
	container->add_child(activate);

	clear_button = memnew(Button);
	clear_button->set_text(TTRC("Clear"));
	clear_button->connect(SceneStringName(pressed), callable_mp(this, &EditorProfiler::_clear_pressed));
	clear_button->set_disabled(true);
	container->add_child(clear_button);

	CheckBox *autostart_checkbox = memnew(CheckBox);
	autostart_checkbox->set_text(TTRC("Autostart"));
	autostart_checkbox->set_pressed(EditorSettings::get_singleton()->get_project_metadata("debug_options", "autostart_profiler", false));
	autostart_checkbox->connect(SceneStringName(toggled), callable_mp(this, &EditorProfiler::_autostart_toggled));
	container->add_child(autostart_checkbox);

	HBoxContainer *hb_measure = memnew(HBoxContainer);
	hb_measure->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	container->add_child(hb_measure);

	hb_measure->add_child(memnew(Label(TTRC("Measure:"))));

	display_mode = memnew(OptionButton);
	display_mode->set_accessibility_name(TTRC("Measure:"));
	display_mode->add_item(TTRC("Frame Time (ms)"));
	display_mode->add_item(TTRC("Average Time (ms)"));
	display_mode->add_item(TTRC("Frame %"));
	display_mode->add_item(TTRC("Physics Frame %"));
	display_mode->connect(SceneStringName(item_selected), callable_mp(this, &EditorProfiler::_combo_changed));
	hb_measure->add_child(display_mode);

	HBoxContainer *hb_time = memnew(HBoxContainer);
	hb_time->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	container->add_child(hb_time);

	hb_time->add_child(memnew(Label(TTRC("Time:"))));

	display_time = memnew(OptionButton);
	display_time->set_accessibility_name(TTRC("Time:"));
	display_time->add_item(TTRC("Inclusive"));
	display_time->add_item(TTRC("Self"));
	display_time->set_tooltip_text(TTRC("Inclusive: Includes time from other functions called by this function.\nUse this to spot bottlenecks.\n\nSelf: Only count the time spent in the function itself, not in other functions called by that function.\nUse this to find individual functions to optimize."));
	display_time->connect(SceneStringName(item_selected), callable_mp(this, &EditorProfiler::_combo_changed));
	hb_time->add_child(display_time);

	display_internal_profiles = memnew(CheckButton(TTRC("Display internal functions")));
	display_internal_profiles->set_visible(EDITOR_GET("debugger/profile_native_calls"));
	display_internal_profiles->set_pressed(false);
	display_internal_profiles->connect(SceneStringName(pressed), callable_mp(this, &EditorProfiler::_internal_profiles_pressed));
	container->add_child(display_internal_profiles);

	HBoxContainer *hb_frame = memnew(HBoxContainer);
	hb_frame->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	hb_frame->set_v_size_flags(SIZE_SHRINK_BEGIN);
	hb->add_child(hb_frame);

	hb_frame->add_child(memnew(Label(TTRC("Frame #:"))));

	cursor_metric_edit = memnew(SpinBox);
	cursor_metric_edit->set_accessibility_name(TTRC("Frame #:"));
	cursor_metric_edit->set_h_size_flags(SIZE_FILL);
	cursor_metric_edit->set_value(0);
	cursor_metric_edit->set_editable(false);
	hb_frame->add_child(cursor_metric_edit);
	cursor_metric_edit->connect(SceneStringName(value_changed), callable_mp(this, &EditorProfiler::_cursor_metric_changed));

	h_split = memnew(HSplitContainer);
	add_child(h_split);
	h_split->set_v_size_flags(SIZE_EXPAND_FILL);

	left_vbox = memnew(VBoxContainer);
	left_vbox->set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	left_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	h_split->add_child(left_vbox);

	filter_bar = memnew(HBoxContainer);
	left_vbox->add_child(filter_bar);

	search_box = memnew(LineEdit);
	search_box->set_placeholder(TTR("Filter functions or nodes..."));
	search_box->set_clear_button_enabled(true);
	search_box->set_h_size_flags(SIZE_EXPAND_FILL);
	search_box->connect(SceneStringName(text_changed), callable_mp(this, &EditorProfiler::_on_search_changed));
	filter_bar->add_child(search_box);

	profile_target = memnew(OptionButton);
	profile_target->add_item(TTR("Functions"), TARGET_FUNCTIONS);
	profile_target->add_item(TTR("Nodes"), TARGET_NODES);
	profile_target->add_item(TTR("All"), TARGET_ALL);
	profile_target->connect(SceneStringName(item_selected), callable_mp(this, &EditorProfiler::_on_target_changed));
	filter_bar->add_child(profile_target);

	btn_copy = memnew(Button);
	btn_copy->set_text(TTR("Copy"));
	btn_copy->set_tooltip_text(TTR("Copy inspected frame report to clipboard"));
	btn_copy->connect(SceneStringName(pressed), callable_mp(this, &EditorProfiler::_on_copy_pressed));
	filter_bar->add_child(btn_copy);

	variables = memnew(Tree);
	variables->set_v_size_flags(SIZE_EXPAND_FILL);
	left_vbox->add_child(variables);
	variables->set_hide_root(true);
	variables->set_columns(4);
	variables->set_column_titles_visible(true);
	variables->set_column_title(0, TTRC("Name / Node"));
	variables->set_column_expand(0, true);
	variables->set_column_clip_content(0, true);
	variables->set_column_custom_minimum_width(0, 80 * EDSCALE);

	variables->set_column_title(1, TTRC("Time"));
	variables->set_column_expand(1, false);
	variables->set_column_clip_content(1, true);
	variables->set_column_custom_minimum_width(1, 80 * EDSCALE);

	variables->set_column_title(2, TTRC("Calls"));
	variables->set_column_expand(2, false);
	variables->set_column_clip_content(2, true);
	variables->set_column_custom_minimum_width(2, 55 * EDSCALE);

	variables->set_column_title(3, TTRC("Status / Hotspot"));
	variables->set_column_expand(3, false);
	variables->set_column_clip_content(3, true);
	variables->set_column_custom_minimum_width(3, 110 * EDSCALE);

	variables->set_column_title_alignment(0, HORIZONTAL_ALIGNMENT_LEFT);
	variables->set_column_title_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
	variables->set_column_title_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
	variables->set_column_title_alignment(3, HORIZONTAL_ALIGNMENT_LEFT);
	variables->set_theme_type_variation("TreeSecondary");
	variables->connect("item_edited", callable_mp(this, &EditorProfiler::_item_edited));
	variables->connect("item_activated", callable_mp(this, &EditorProfiler::_item_activated));

	graph = memnew(TextureRect);
	graph->set_custom_minimum_size(Size2(250 * EDSCALE, 0));
	graph->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	graph->set_mouse_filter(MOUSE_FILTER_STOP);
	graph->connect(SceneStringName(draw), callable_mp(this, &EditorProfiler::_graph_tex_draw));
	graph->connect(SceneStringName(gui_input), callable_mp(this, &EditorProfiler::_graph_tex_input));
	graph->connect(SceneStringName(mouse_exited), callable_mp(this, &EditorProfiler::_graph_tex_mouse_exit));

	h_split->add_child(graph);
	graph->set_h_size_flags(SIZE_EXPAND_FILL);

	int metric_size = CLAMP(int(EDITOR_GET("debugger/profiler_frame_history_size")), 60, 10000);
	frame_metrics.resize(metric_size);

	frame_delay = memnew(Timer);
	frame_delay->set_wait_time(0.1);
	frame_delay->set_one_shot(true);
	add_child(frame_delay);
	frame_delay->connect("timeout", callable_mp(this, &EditorProfiler::_update_frame));

	plot_delay = memnew(Timer);
	plot_delay->set_wait_time(0.1);
	plot_delay->set_one_shot(true);
	add_child(plot_delay);
	plot_delay->connect("timeout", callable_mp(this, &EditorProfiler::_update_plot));

	plot_sigs.insert("physics_frame_time");
	plot_sigs.insert("category_frame_time");
}
