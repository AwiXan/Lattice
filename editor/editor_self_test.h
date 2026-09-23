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

class EditorPane;
class EditorPaneTree;

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
	bool started = false;

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
	void _script_open();
	void _script_reopen();
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
