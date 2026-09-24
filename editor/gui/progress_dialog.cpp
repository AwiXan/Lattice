/**************************************************************************/
/*  progress_dialog.cpp                                                   */
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

#include "progress_dialog.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "main/main.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/progress_bar.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"

void BackgroundProgress::_add_task(const String &p_task, const String &p_label, int p_steps) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_MSG(tasks.has(p_task), "Task '" + p_task + "' already exists.");
	BackgroundProgress::Task t;
	t.hb = memnew(HBoxContainer);
	Label *l = memnew(Label);
	l->set_text(p_label + " ");
	t.hb->add_child(l);
	t.progress = memnew(ProgressBar);
	t.progress->set_theme_type_variation("PopupProgressBar");
	t.progress->set_max(p_steps);
	t.progress->set_value(p_steps);
	Control *ec = memnew(Control);
	ec->set_h_size_flags(SIZE_EXPAND_FILL);
	ec->set_v_size_flags(SIZE_EXPAND_FILL);
	t.progress->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	ec->add_child(t.progress);
	ec->set_custom_minimum_size(Size2(80, 5) * EDSCALE);
	t.hb->add_child(ec);

	add_child(t.hb);

	tasks[p_task] = t;
}

void BackgroundProgress::_update() {
	_THREAD_SAFE_METHOD_

	for (const KeyValue<String, int> &E : updates) {
		if (tasks.has(E.key)) {
			_task_step(E.key, E.value);
		}
	}

	updates.clear();
}

void BackgroundProgress::_task_step(const String &p_task, int p_step) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!tasks.has(p_task));

	Task &t = tasks[p_task];
	if (p_step < 0) {
		t.progress->set_value(t.progress->get_value() + 1);
	} else {
		t.progress->set_value(p_step);
	}
}

void BackgroundProgress::_end_task(const String &p_task) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!tasks.has(p_task));
	Task &t = tasks[p_task];

	memdelete(t.hb);
	tasks.erase(p_task);
}

void BackgroundProgress::add_task(const String &p_task, const String &p_label, int p_steps) {
	callable_mp(this, &BackgroundProgress::_add_task).call_deferred(p_task, p_label, p_steps);
}

void BackgroundProgress::task_step(const String &p_task, int p_step) {
	//this code is weird, but it prevents deadlock.
	bool no_updates = true;
	{
		_THREAD_SAFE_METHOD_
		no_updates = updates.is_empty();
	}

	if (no_updates) {
		callable_mp(this, &BackgroundProgress::_update).call_deferred();
	}

	{
		_THREAD_SAFE_METHOD_
		updates[p_task] = p_step;
	}
}

void BackgroundProgress::end_task(const String &p_task) {
	callable_mp(this, &BackgroundProgress::_end_task).call_deferred(p_task);
}

////////////////////////////////////////////////

ProgressDialog *ProgressDialog::singleton = nullptr;

void ProgressDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			Ref<StyleBox> style = main->get_theme_stylebox(SceneStringName(panel), SNAME("PopupMenu"));
			main_border_size = style->get_minimum_size();
			main->set_offset(SIDE_LEFT, style->get_margin(SIDE_LEFT));
			main->set_offset(SIDE_RIGHT, -style->get_margin(SIDE_RIGHT));
			main->set_offset(SIDE_TOP, style->get_margin(SIDE_TOP));
			main->set_offset(SIDE_BOTTOM, -style->get_margin(SIDE_BOTTOM));

			center_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), "PopupPanel"));
			main->add_theme_constant_override("separation", 14 * EDSCALE);
			for (KeyValue<String, Task> &E : tasks) {
				_style_bar(E.value.progress);
			}
		} break;

		case NOTIFICATION_DRAW: {
			// The editor behind it dimmed: it waits.
			draw_rect(Rect2(Point2(), get_size()), Color(0, 0, 0, 0.35));
		} break;
	}
}

void ProgressDialog::_style_bar(ProgressBar *p_bar) const {
	// Thin and rounded, the part done in the theme's accent colour.
	const Control *theme = EditorNode::get_singleton() && EditorNode::get_singleton()->get_gui_base() ? EditorNode::get_singleton()->get_gui_base() : this;
	const Color accent = theme->get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	Color track = theme->get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	track.a = 0.12;
	const int radius = Math::round(3 * EDSCALE);
	Ref<StyleBoxFlat> background;
	background.instantiate();
	background->set_bg_color(track);
	background->set_corner_radius_all(radius);
	background->set_content_margin_all(0);
	Ref<StyleBoxFlat> fill = background->duplicate();
	fill->set_bg_color(accent);
	p_bar->add_theme_style_override("background", background);
	p_bar->add_theme_style_override("fill", fill);
	p_bar->set_show_percentage(false);
	p_bar->set_custom_minimum_size(Size2(0, 6 * EDSCALE));
}

void ProgressDialog::_show_progress(Task &p_task) {
	const double max = MAX(1.0, p_task.progress->get_max());
	const double value = CLAMP(p_task.progress->get_value(), 0.0, max);
	p_task.percent->set_text(itos(int(Math::round(value / max * 100.0))) + "%");
	p_task.count->set_text(vformat("%d / %d", int(value), int(max)));
}

void ProgressDialog::_update_ui() {
	// Run main loop for two frames.
	if (is_inside_tree()) {
		DisplayServer::get_singleton()->process_events();
		Main::iteration();
	}
}

void ProgressDialog::_popup() {
	// Activate processing of all inputs in EditorNode, and the EditorNode::input method
	// will discard every key input.
	EditorNode::get_singleton()->set_process_input(true);
	// Disable all other windows to prevent interaction with them.
	for (ObjectID wid : host_windows) {
		Window *w = ObjectDB::get_instance<Window>(wid);
		if (w) {
			w->set_process_mode(PROCESS_MODE_DISABLED);
		}
	}

	Size2 ms = main->get_combined_minimum_size();
	ms.width = MAX(500 * EDSCALE, ms.width);
	ms += main_border_size;

	center_panel->set_custom_minimum_size(ms);

	if (is_ready()) {
		_reparent_and_show();
	} else {
		callable_mp(this, &ProgressDialog::_reparent_and_show).call_deferred();
	}
}

void ProgressDialog::_reparent_and_show() {
	Window *current_window = SceneTree::get_singleton()->get_root()->get_last_exclusive_window();
	ERR_FAIL_NULL(current_window);
	reparent(current_window);

	// Ensures that events are properly released before the dialog blocks input.
	bool window_is_input_disabled = current_window->is_input_disabled();
	current_window->set_disable_input(!window_is_input_disabled);
	current_window->set_disable_input(window_is_input_disabled);

	show();
}

void ProgressDialog::add_task(const String &p_task, const String &p_label, int p_steps, bool p_can_cancel) {
	if (MessageQueue::get_singleton()->is_flushing()) {
		ERR_PRINT("Do not use progress dialog (task) while flushing the message queue or using call_deferred()!");
		return;
	}

	ERR_FAIL_COND_MSG(tasks.has(p_task), "Task '" + p_task + "' already exists.");
	// What is being done and how far along it is, over a thin bar in the
	// theme's colour; under it, the step it is on.
	ProgressDialog::Task t;
	t.vb = memnew(VBoxContainer);
	t.vb->add_theme_constant_override("separation", 6 * EDSCALE);
	HBoxContainer *head = memnew(HBoxContainer);
	t.vb->add_child(head);
	Label *title = memnew(Label(p_label));
	title->set_theme_type_variation("HeaderSmall");
	title->set_h_size_flags(SIZE_EXPAND_FILL);
	title->set_clip_text(true);
	head->add_child(title);
	t.percent = memnew(Label);
	t.percent->set_theme_type_variation("HeaderSmall");
	head->add_child(t.percent);
	t.progress = memnew(ProgressBar);
	t.progress->set_theme_type_variation("PopupProgressBar");
	_style_bar(t.progress);
	t.progress->set_max(p_steps);
	t.progress->set_value(0);
	t.vb->add_child(t.progress);
	HBoxContainer *foot = memnew(HBoxContainer);
	t.vb->add_child(foot);
	t.state = memnew(Label);
	t.state->set_clip_text(true);
	t.state->set_h_size_flags(SIZE_EXPAND_FILL);
	t.state->set_modulate(Color(1, 1, 1, 0.7));
	foot->add_child(t.state);
	t.count = memnew(Label);
	t.count->set_modulate(Color(1, 1, 1, 0.55));
	foot->add_child(t.count);
	_show_progress(t);
	main->add_child(t.vb);

	tasks[p_task] = t;
	if (p_can_cancel) {
		cancel_hb->show();
	} else {
		cancel_hb->hide();
	}
	cancel_hb->move_to_front();
	canceled = false;
	_popup();
	if (p_can_cancel) {
		cancel->grab_focus();
	}
	_update_ui();
}

bool ProgressDialog::task_step(const String &p_task, const String &p_state, int p_step, bool p_force_redraw) {
	ERR_FAIL_COND_V(!tasks.has(p_task), canceled);

	Task &t = tasks[p_task];
	if (!p_force_redraw) {
		uint64_t tus = OS::get_singleton()->get_ticks_usec();
		if (tus - t.last_progress_tick < 200000) { //200ms
			return canceled;
		}
	}
	if (p_step < 0) {
		t.progress->set_value(t.progress->get_value() + 1);
	} else {
		t.progress->set_value(p_step);
	}

	t.state->set_text(p_state);
	_show_progress(t);
	t.last_progress_tick = OS::get_singleton()->get_ticks_usec();
	_update_ui();

	return canceled;
}

void ProgressDialog::end_task(const String &p_task) {
	ERR_FAIL_COND(!tasks.has(p_task));
	Task &t = tasks[p_task];

	memdelete(t.vb);
	tasks.erase(p_task);

	if (tasks.is_empty()) {
		hide();
		EditorNode::get_singleton()->set_process_input(false);
		for (ObjectID wid : host_windows) {
			Window *w = ObjectDB::get_instance<Window>(wid);
			if (w) {
				w->set_process_mode(PROCESS_MODE_INHERIT);
			}
		}
	} else {
		_popup();
	}
}

void ProgressDialog::add_host_window(ObjectID p_window) {
	host_windows.push_back(p_window);
}

void ProgressDialog::remove_host_window(ObjectID p_window) {
	host_windows.erase(p_window);
}

void ProgressDialog::_cancel_pressed() {
	canceled = true;
}

ProgressDialog::ProgressDialog() {
	// We want to cover the entire screen to prevent the user from interacting with the Editor.
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	// Be sure it's the top most component.
	set_z_index(RSE::CANVAS_ITEM_Z_MAX);
	singleton = this;
	hide();

	center_panel = memnew(PanelContainer);
	add_child(center_panel);
	center_panel->set_h_size_flags(SIZE_SHRINK_BEGIN);
	center_panel->set_v_size_flags(SIZE_SHRINK_BEGIN);

	main = memnew(VBoxContainer);
	center_panel->add_child(main);

	cancel_hb = memnew(HBoxContainer);
	main->add_child(cancel_hb);
	cancel_hb->hide();
	cancel = memnew(Button);
	cancel_hb->add_spacer();
	cancel_hb->add_child(cancel);
	cancel->set_text(TTR("Cancel"));
	cancel_hb->add_spacer();
	cancel->connect(SceneStringName(pressed), callable_mp(this, &ProgressDialog::_cancel_pressed));
}

ProgressDialog::~ProgressDialog() {
	singleton = nullptr;
}
