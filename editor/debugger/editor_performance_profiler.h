/**************************************************************************/
/*  editor_performance_profiler.h                                         */
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

#pragma once

#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "editor/debugger/editor_profiler.h"
#include "main/performance.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"
#include "servers/rendering/rendering_server_types.h"

class EditorPerformanceProfiler : public HSplitContainer {
	GDCLASS(EditorPerformanceProfiler, HSplitContainer);

public:
	enum ViewMode {
		VIEW_GRAPHS = 0,
		VIEW_BREAKDOWN = 1,
	};

	enum PresetID {
		PRESET_FRAMETIME = 0,
		PRESET_RENDERING,
		PRESET_MEMORY,
		PRESET_PHYSICS,
		PRESET_AUDIO,
		PRESET_SELECT_ALL,
		PRESET_CLEAR_ALL,
	};

private:
	static const int MAX_HISTORY = 600;

	class Monitor {
	public:
		String name;
		String base;
		List<float> history;
		float max = 0.0f;
		float history_sum = 0.0f;
		TreeItem *item = nullptr;
		Performance::MonitorType type = Performance::MONITOR_TYPE_QUANTITY;
		int frame_index = 0;

		Monitor() {}
		Monitor(const String &p_name, const String &p_base, int p_frame_index, Performance::MonitorType p_type, TreeItem *p_item);
		void reset();
	};

	HashMap<StringName, Monitor> monitors;
	HashMap<StringName, TreeItem *> base_map;

	// Left panel (Monitors Tree & Filter)
	VBoxContainer *left_vbox = nullptr;
	HBoxContainer *filter_hbox = nullptr;
	LineEdit *search_box = nullptr;
	MenuButton *presets_button = nullptr;
	Tree *monitor_tree = nullptr;
	String search_filter;

	// Right panel (Toolbar + Graphs/Breakdown)
	VBoxContainer *right_vbox = nullptr;
	HBoxContainer *header_bar = nullptr;
	PanelContainer *diagnosis_panel = nullptr;
	Label *diagnosis_label = nullptr;
	OptionButton *view_mode_opt = nullptr;
	Button *btn_pause = nullptr;
	CheckBox *btn_call_chains = nullptr;
	Button *btn_copy = nullptr;
	Button *btn_clear = nullptr;

	Control *content_stack = nullptr;
	Control *monitor_draw = nullptr;
	Label *info_message = nullptr;
	Tree *breakdown_tree = nullptr;
	PopupMenu *breakdown_context_menu = nullptr;

	enum ContextMenuOptions {
		CONTEXT_COPY_ROW = 0,
		CONTEXT_COPY_CATEGORY_HEADER_ONLY,
		CONTEXT_COPY_FUNC_NAME,
		CONTEXT_COPY_CALL_CHAIN,
		CONTEXT_COPY_REPORT_WITH_CHAINS,
		CONTEXT_COPY_REPORT_SUMMARY,
	};

	StringName marker_key;
	int marker_frame = 0;
	const int MARGIN = 4;
	const int POINT_SEPARATION = 5;
	const int MARKER_MARGIN = 2;

	bool is_paused = false;
	bool detailed_profiling_enabled = true;
	EditorProfiler::Metric last_server_metric;
	ViewMode view_mode = VIEW_GRAPHS;
	Vector<RenderingServerTypes::FrameProfileArea> last_gpu_areas;
	uint64_t last_breakdown_update_msec = 0;

	String _format_label(float p_value, Performance::MonitorType p_type) const;
	void _update_monitor_value(Monitor *p_monitor, float p_value);
	void _monitor_select();
	void _monitor_draw();
	void _build_monitor_tree();
	TreeItem *_get_monitor_base(const StringName &p_base_name);
	TreeItem *_create_monitor_item(const StringName &p_monitor_name, TreeItem *p_base);
	void _marker_input(const Ref<InputEvent> &p_event);

	void _on_search_changed(const String &p_text);
	void _apply_filter();
	void _on_preset_selected(int p_id);
	void _on_view_mode_selected(int p_mode);
	void _on_pause_toggled(bool p_pressed);
	void _on_call_chains_toggled(bool p_pressed);
	void _on_copy_pressed();
	void _on_clear_pressed();
	void _on_breakdown_tree_rmb(const Vector2 &p_pos, MouseButton p_button);
	void _breakdown_tree_gui_input(const Ref<InputEvent> &p_event);
	void _on_context_menu_id_pressed(int p_id);
	void _copy_selected_item();
	void _export_item_subtree(TreeItem *p_item, int p_depth, String &r_text) const;
	void _export_report_children(TreeItem *p_parent, int p_indent, bool p_include_call_chains, String &r_text) const;
	String _generate_breakdown_report(bool p_include_call_chains) const;
	void _collect_collapsed_items(TreeItem *p_item, HashSet<String> &r_collapsed);

	void _update_frame_breakdown(const Vector<float> &p_values);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void reset();
	void update_monitors(const Vector<StringName> &p_names, const PackedInt32Array &p_types);
	void add_profile_frame(const Vector<float> &p_values);
	void add_visual_profile_frame(const Vector<RenderingServerTypes::FrameProfileArea> &p_areas);
	void add_server_profile_metric(const EditorProfiler::Metric &p_metric);
	bool is_detailed_profiling_enabled() const { return detailed_profiling_enabled; }
	List<float> *get_monitor_data(const StringName &p_name);
	EditorPerformanceProfiler();
};
