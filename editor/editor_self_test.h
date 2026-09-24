/**************************************************************************/
/*  editor_self_test.h                                                    */
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

#include "core/error/error_macros.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_refcount.h"
#include "scene/main/node.h"

class Control;
class EditorPane;
class EditorPaneTree;

// A picture of the editor, for checking how something looks without sitting
// in front of it: LATTICE_SHOT=<png path> opens LATTICE_SHOT_SCENE if given,
// selects its first 3D node, waits for things to settle, saves the window to
// the file and quits. misc/scripts/lattice_screenshot.py sets it all up. Needs
// a real window: headless draws nothing.
class EditorScreenshot : public Node {
	GDCLASS(EditorScreenshot, Node);

	uint64_t started_at = 0;
	bool selected = false;
	bool later_done = false;
	bool taken = false;

	// LATTICE_STRESS_POPUPS=<seconds>: every dropdown of the editor opened and
	// closed in turn, frame after frame, before the picture - looking for a
	// freeze.
	// Frames until a picture of the window of its own showing is taken as
	// well - one opening, say - as <shot>_early.png; 0 for none.
	int early_shot_frames = 0;
	// Frames left to say, each, whether this menu button's popup is showing.
	ObjectID watched_menu;
	int watch_frames = 0;
	bool watch_real = false;
	uint64_t stress_until = 0;
	int stress_step = 0;
	Vector<ObjectID> stress_targets;
	ObjectID stress_open;
	void _stress_popups();

	void _open_scene();

protected:
	void _notification(int p_what);

public:
	static bool is_requested();

	EditorScreenshot();
};

// The editor checking itself: split, close, move, open and reopen things the
// way a user would, and say what did not come out as it should.
//
// Started by the LATTICE_SELFTEST environment variable, on a project made for
// it - misc/scripts/lattice_selftest.py makes one and reads the answer. It
// counts every error printed while it runs, and quits with a non-zero exit
// code if anything failed; leaks are only known after that, which is why the
// script reads the output too.
class EditorSelfTest : public Node {
	GDCLASS(EditorSelfTest, Node);

	struct Step {
		String name;
		Callable run;
	};
	LocalVector<Step> steps;
	int next_step = 0;
	int frames_to_wait = 0;
	// A step can ask for time to pass before the next, for what the editor
	// does on a timer - a script is checked a moment after it is typed in.
	double seconds_to_wait = 0.0;
	uint64_t waiting_since = 0;
	bool started = false;

	Variant scripts_in_panels_before;
	int passed = 0;
	int failed = 0;
	SafeNumeric<uint32_t> errors;
	ErrorHandlerList error_handler;
	static void _error_handler(void *p_self, const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type);

	void _add(const String &p_name, const Callable &p_run);
	void _check(bool p_ok, const String &p_what);

	EditorPaneTree *_tree() const;
	EditorPane *_pane_showing(const StringName &p_type, int *r_index = nullptr) const;
	StringName _type_titled(const String &p_title) const;
	void _close_tab(EditorPane *p_pane, const StringName &p_type);
	void _close_pane(EditorPane *p_pane);
	int _visible_popups(Node *p_in) const;
	void _hide_popups();
	void _edit(const String &p_path);

	// The steps, in order.
	void _crash_report();
	void _begin();
	void _scene_panel();
	void _inspector_panel();
	void _inspector_lets_go();
	void _inspector_shows_again();
	ObjectID inspector_node;
	bool inspector_added = false;
	void _script_open();
	void _script_opened();
	void _script_edited();
	void _script_saved();
	void _script_close();
	void _script_closed();
	void _script_back_where_it_was();
	void _script_close_unsaved();
	void _script_close_unsaved_asked();
	void _second_script_open();
	void _second_script_stacked();
	static void _check_second_script(ObjectID p_self, ObjectID p_first_pane);
	void _second_script_check(ObjectID p_first_pane);
	EditorPane *_pane_with_script(const String &p_path, int *r_index = nullptr) const;
	ObjectID moved_script_pane;
	void _script_moved();
	void _script_follows_the_move();
	void _script_pane_gone();
	void _script_with_the_others();
	void _view_chrome();
	void _view_shading();
	void _view_overlays();
	void _view_bar();
	int addon_view_item_pressed = -1;
	void _addon_view_item_pressed(int p_id);
	void _view_sidebar_open();
	void _view_sidebar_check();
	void _view_hints();
	// An addon putting its icons back into the editor's theme one at a time,
	// whenever it changes - Jenova does - and a theme setting switched.
	void _theme_rebuilt_with_addon_icons();
	void _put_addon_icons_back();
	void _sidebar_slide_open();
	void _sidebar_slide_close();
	void _sidebar_slid_out();
	void _pie_shading();
	void _pie_tap();
	void _pie_view();
	static void _press_key(Control *p_focus, Key p_key, bool p_pressed);
	void _addon_mirror_prepare();
	void _addon_mirror_check();
	void _view_2d_open();
	void _view_2d_check();
	void _script_drag_out_open();
	void _script_drag_out_drop();
	void _script_drag_out_close();
	void _script_drag_out_back();
	void _script_bring_out();
	void _script_bring_here();
	void _script_brought_here();
	static bool _script_is_open(const String &p_path);
	ObjectID view_2d;
	void _addon_button_pressed() { addon_presses++; }
	int addon_presses = 0;
	ObjectID addon_pane;
	ObjectID addon_button;
	ObjectID addon_plugin;
	ObjectID sidebar_view;
	ObjectID sidebar_node;
	real_t sidebar_node_x = 0.0;
	void _script_left_open();
	void _script_stand_in();
	EditorPane *_script_pane(int *r_index = nullptr) const;
	ObjectID script_pane;
	void _shader_open();
	void _shader_close_tab();
	void _shader_back_in_its_pane();
	void _shader_close_pane();
	void _shader_pane_closed();
	void _shader_back_in_its_place();
	void _node_does_not_pull_docks();
	void _reopen_closed();
	void _maximize();
	void _maximize_by_shortcut();
	void _restore_by_shortcut();
	void _drop_zones_prepare();
	void _drop_zones_hold_steady();
	void _tab_lands_where_marked();
	void _dock_menus_follow_focus();
	void _compass_prepare();
	void _compass_targets();
	void _whole_side();
	void _whole_side_check();
	void _drop_from_another_window();
	void _workspaces_save();
	void _workspaces_switch_back();
	void _workspaces_switch_again();
	void _workspaces_change_one();
	void _workspaces_remembered();
	void _hidden_inspector_prepare();
	void _hidden_inspector_idle();
	void _recovery_offered();
	void _recovery_restored();
	void _recovery_copies();
	void _scene_colors();
	void _panel_from_palette();
	void _panel_from_palette_shown();
	void _remote_filter();
	void _keys_prepare();
	void _keys_move();
	void _keys_tabs();
	void _worlds_prepare();
	void _worlds_check();
	void _worlds_after_camera_moved();
	void _worlds_one_view_closed();
	void _finish();

	// Kept between steps.
	ObjectID shader_pane;
	int panes_before_shader = 0;
	real_t shader_ratio = 0;
	ObjectID animation_player;

protected:
	void _notification(int p_what);

public:
	static bool is_requested();

	EditorSelfTest();
	~EditorSelfTest();
};
