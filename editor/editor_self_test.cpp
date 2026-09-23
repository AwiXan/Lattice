/**************************************************************************/
/*  editor_self_test.cpp                                                  */
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

#include "editor_self_test.h"

#include "core/input/input_event.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_pane.h"
#include "editor/gui/editor_pane_tree.h"
#include "editor/scene/editor_scene_panel.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_bar.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

bool EditorSelfTest::is_requested() {
	return OS::get_singleton()->has_environment("LATTICE_SELFTEST");
}

void EditorSelfTest::_error_handler(void *p_self, const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	if (p_type == ERR_HANDLER_WARNING) {
		return;
	}
	static_cast<EditorSelfTest *>(p_self)->errors.increment();
}

void EditorSelfTest::_add(const String &p_name, const Callable &p_run) {
	Step step;
	step.name = p_name;
	step.run = p_run;
	steps.push_back(step);
}

void EditorSelfTest::_check(bool p_ok, const String &p_what) {
	if (p_ok) {
		passed++;
		print_line("SELFTEST PASS: " + p_what);
	} else {
		failed++;
		print_line("SELFTEST FAIL: " + p_what);
	}
}

EditorPaneTree *EditorSelfTest::_tree() const {
	return EditorNode::get_editor_main_screen()->get_pane_tree();
}

EditorPane *EditorSelfTest::_pane_showing(const StringName &p_type, int *r_index) const {
	for (EditorPane *pane : _tree()->get_panes()) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_type_at(i) == p_type) {
				if (r_index) {
					*r_index = i;
				}
				return pane;
			}
		}
	}
	return nullptr;
}

StringName EditorSelfTest::_type_titled(const String &p_title) const {
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		if (type && type->title == p_title) {
			return id;
		}
	}
	return StringName();
}

void EditorSelfTest::_close_tab(EditorPane *p_pane, const StringName &p_type) {
	// The way a user does it: the tab's close button, which is what notes it.
	for (int i = 0; i < p_pane->get_panel_count(); i++) {
		if (p_pane->get_panel_type_at(i) == p_type) {
			p_pane->get_tab_bar()->emit_signal(SNAME("tab_close_pressed"), i);
			return;
		}
	}
}

void EditorSelfTest::_close_pane(EditorPane *p_pane) {
	// The pane's own close button.
	TypedArray<Node> buttons = p_pane->find_children("*", "Button", true, false);
	for (int i = 0; i < buttons.size(); i++) {
		Button *button = Object::cast_to<Button>(buttons[i]);
		if (button && button->get_tooltip_text() == "Close this pane.") {
			button->emit_signal(SceneStringName(pressed));
			return;
		}
	}
}

int EditorSelfTest::_visible_popups(Node *p_in) const {
	int count = 0;
	TypedArray<Node> windows = p_in->find_children("*", "Window", true, false);
	for (int i = 0; i < windows.size(); i++) {
		Window *window = Object::cast_to<Window>(windows[i]);
		if (window && window->is_visible() && Object::cast_to<PopupMenu>(window)) {
			count++;
		}
	}
	return count;
}

void EditorSelfTest::_hide_popups() {
	TypedArray<Node> windows = EditorNode::get_singleton()->find_children("*", "Window", true, false);
	for (int i = 0; i < windows.size(); i++) {
		Window *window = Object::cast_to<Window>(windows[i]);
		// Menus and dialogs a step opened; never a window of panes.
		if (window && window->is_visible() && (Object::cast_to<PopupMenu>(window) || Object::cast_to<AcceptDialog>(window))) {
			window->hide();
		}
	}
}

void EditorSelfTest::_edit(const String &p_path) {
	Ref<Resource> resource = ResourceLoader::load(p_path);
	if (resource.is_valid()) {
		EditorNode::get_singleton()->edit_resource(resource);
	}
}

// ---------------------------------------------------------------- the steps

void EditorSelfTest::_begin() {
	EditorPaneTree *tree = _tree();
	while (tree->get_panes().size() > 1) {
		tree->close_pane(tree->get_panes()[tree->get_panes().size() - 1]);
	}
	EditorPane *pane = tree->get_first_pane();
	pane->set_panel_type("view_3d");
	tree->set_active_pane(pane);
	_check(tree->get_panes().size() == 1 && pane->get_panel_type() == StringName("view_3d"), "starts from one pane showing the 3D view");
	_check(EditorNode::get_singleton()->get_edited_scene() != nullptr, "a scene with a root is open");
}

void EditorSelfTest::_scene_panel() {
	EditorPane *pane = _tree()->get_first_pane();
	const int index = pane->show_panel_of_type("dock_Scene");
	_check(index >= 0 && pane->get_panel_type_at(index) == StringName("scene_tree"), "the Scene dock's name opens the Scene panel");
	EditorScenePanel *panel = index >= 0 ? Object::cast_to<EditorScenePanel>(pane->get_panel_at(index)) : nullptr;
	if (!panel) {
		return;
	}
	const int buttons = panel->find_children("*", "Button", true, false).size();
	const int filters = panel->find_children("*", "LineEdit", true, false).size();
	_check(buttons >= 6 && filters >= 1, vformat("the Scene panel has the dock's buttons and a filter (%d buttons)", buttons));

	panel->get_local_tree()->get_scene_tree()->emit_signal(SceneStringName(focus_entered));
	_check(SceneTreeDock::get_singleton()->get_tree_editor() == panel->get_local_tree(), "working in the Scene panel makes it the tree the dock acts on");
	panel->get_local_tree()->emit_signal(SNAME("rmb_pressed"), Vector2(10, 10));
	_check(_visible_popups(SceneTreeDock::get_singleton()) > 0, "the Scene panel's context menu opens");
	_hide_popups();
	pane->close_panel(index);
}

void EditorSelfTest::_inspector_panel() {
	EditorPane *pane = _tree()->get_first_pane();
	const int index = pane->show_panel_of_type("dock_Inspector");
	_check(index >= 0 && pane->get_panel_type_at(index) == StringName("inspector"), "the Inspector dock's name opens the Inspector panel");
	Control *panel = index >= 0 ? pane->get_panel_at(index) : nullptr;
	if (!panel) {
		return;
	}
	_check(panel->find_children("*", "EditorObjectSelector", true, false).size() == 1, "the Inspector panel has the path to the object");
	_check(panel->find_children("*", "Button", true, false).size() >= 9, "the Inspector panel has the dock's buttons");
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type("inspector");
	_check(type && type->icon == StringName("AnimationTrackList"), "the Inspector panel has the Inspector's icon");
	pane->close_panel(index);
}

void EditorSelfTest::_script_open() {
	_edit("res://probe.gd");
	_check(_pane_showing("main_Script") != nullptr, "opening a script shows the script editor");
}

void EditorSelfTest::_script_reopen() {
	EditorPane *pane = _pane_showing("main_Script");
	if (pane) {
		_close_tab(pane, "main_Script");
	}
	_check(_pane_showing("main_Script") == nullptr, "the script editor's tab closes");
	_edit("res://probe.gd");
	_check(_pane_showing("main_Script") == pane, "opening a script again shows it again, where it was");
}

void EditorSelfTest::_shader_open() {
	panes_before_shader = _tree()->get_panes().size();
	_edit("res://probe.gdshader");
	const StringName type = _type_titled("Shader Editor");
	EditorPane *pane = _pane_showing(type);
	_check(pane != nullptr, "opening a shader shows the shader editor, with nowhere set up for it");
	if (!pane) {
		return;
	}
	shader_pane = pane->get_instance_id();
	const EditorPaneTree::Place place = _tree()->get_place_of(pane);
	_check(_tree()->get_panes().size() == panes_before_shader + 1 && place.vertical && !place.before, "it gets a pane of its own below the one being worked in");
	shader_ratio = place.ratio;
}

void EditorSelfTest::_shader_close_tab() {
	EditorPane *pane = ObjectDB::get_instance<EditorPane>(shader_pane);
	if (pane) {
		_close_tab(pane, _type_titled("Shader Editor"));
	}
	_check(_pane_showing(_type_titled("Shader Editor")) == nullptr, "the shader editor's tab closes");
	_edit("res://probe.gdshader");
}

void EditorSelfTest::_shader_back_in_its_pane() {
	EditorPane *pane = _pane_showing(_type_titled("Shader Editor"));
	_check(pane && pane->get_instance_id() == shader_pane, "opened again, it is back in the pane it was closed in");
}

void EditorSelfTest::_shader_close_pane() {
	// A pane closes a frame later, so it is looked for in the next step.
	EditorPane *pane = ObjectDB::get_instance<EditorPane>(shader_pane);
	if (pane) {
		_close_pane(pane);
	}
}

void EditorSelfTest::_shader_pane_closed() {
	_check(_tree()->get_panes().size() == panes_before_shader, "the shader editor's pane closes");
	_edit("res://probe.gdshader");
}

void EditorSelfTest::_shader_back_in_its_place() {
	EditorPane *pane = _pane_showing(_type_titled("Shader Editor"));
	_check(pane != nullptr, "opened again after its pane was closed, it is shown");
	if (!pane) {
		return;
	}
	const EditorPaneTree::Place place = _tree()->get_place_of(pane);
	_check(place.vertical && !place.before && Math::abs(place.ratio - shader_ratio) < 0.01, "in a pane made again where the closed one was");
	shader_pane = pane->get_instance_id();
}

void EditorSelfTest::_node_does_not_pull_docks() {
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	if (!root) {
		return;
	}
	AnimationPlayer *player = memnew(AnimationPlayer);
	root->add_child(player);
	player->set_owner(root);
	animation_player = player->get_instance_id();
	const int panes = _tree()->get_panes().size();
	EditorNode::get_singleton()->push_item(player);
	const StringName animation = _type_titled("Animation");
	_check(animation != StringName() && _pane_showing(animation) == nullptr && _tree()->get_panes().size() == panes, "selecting a node does not pull a dock out for it");
	// Left in the scene, which is never saved: freeing it under the Animation
	// dock's feet is a different test.
	EditorNode::get_singleton()->push_item(root);
}

void EditorSelfTest::_reopen_closed() {
	EditorPane *pane = _tree()->get_first_pane();
	pane->show_panel_of_type("scene_tree");
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	const int closed = main_screen->get_closed_panel_count();
	_close_tab(pane, "scene_tree");
	_check(main_screen->get_closed_panel_count() == closed + 1 && main_screen->get_closed_panel_type(closed) == StringName("scene_tree"), "a closed panel is listed as recently closed");
	_check(main_screen->reopen_closed_panel() && _pane_showing("scene_tree") == pane, "reopening it brings it back to its pane");
}

void EditorSelfTest::_maximize() {
	EditorPaneTree *tree = _tree();
	EditorPane *pane = tree->get_first_pane();
	tree->toggle_maximized(pane);
	bool others_hidden = true;
	for (EditorPane *other : tree->get_panes()) {
		if (other != pane && other->is_visible()) {
			others_hidden = false;
		}
	}
	_check(tree->get_maximized_pane() == pane && others_hidden, "a pane can be shown over all the others");
	tree->toggle_maximized(pane);
	bool all_shown = true;
	for (EditorPane *other : tree->get_panes()) {
		all_shown = all_shown && other->is_visible();
	}
	_check(tree->get_maximized_pane() == nullptr && all_shown, "and put back");
}

static Ref<InputEventKey> _ctrl_space() {
	Ref<InputEventKey> key;
	key.instantiate();
	key->set_keycode(Key::SPACE);
	key->set_ctrl_pressed(true);
	key->set_pressed(true);
	return key;
}

void EditorSelfTest::_maximize_by_shortcut() {
	EditorNode::get_singleton()->get_window()->push_input(_ctrl_space());
	_check(_tree()->get_maximized_pane() != nullptr, "Ctrl+Space shows a pane over the others");
}

void EditorSelfTest::_restore_by_shortcut() {
	EditorNode::get_singleton()->get_window()->push_input(_ctrl_space());
	_check(_tree()->get_maximized_pane() == nullptr, "and Ctrl+Space again puts it back");
}

void EditorSelfTest::_drop_zones_prepare() {
	// Room to aim in, whatever the other panes took: laid out next frame.
	_tree()->set_maximized_pane(_tree()->get_first_pane());
}

void EditorSelfTest::_drop_zones_hold_steady() {
	EditorPane *pane = _tree()->get_first_pane();
	const Rect2 body = pane->get_body_rect();
	_tree()->set_maximized_pane(nullptr);
	if (body.size.x < 200 || body.size.y < 200) {
		_check(false, vformat("a pane is big enough to aim at (%s)", body.size));
		return;
	}
	// Just inside the left band, then a little past where it ends.
	const real_t band = MIN(body.size.x * 0.3, 180.0 * EDSCALE);
	const Point2 inside = body.position + Point2(band - 4 * EDSCALE, body.size.y * 0.5);
	const Point2 past = body.position + Point2(band + 8 * EDSCALE, body.size.y * 0.5);
	_check(pane->get_drop_zone_at(inside) == EditorPane::DROP_LEFT, "near the left edge, a drop splits to the left");
	_check(pane->get_drop_zone_at(past, EditorPane::DROP_LEFT) == EditorPane::DROP_LEFT, "and it stays that way a little past the edge of the band");
	_check(pane->get_drop_zone_at(past) == EditorPane::DROP_INTO, "where coming from anywhere else it would join the pane");
}

void EditorSelfTest::_tab_lands_where_marked() {
	EditorPane *pane = _tree()->get_first_pane();
	const StringName fillers[] = { "inspector", "scene_tree", "view_3d" };
	for (const StringName &type : fillers) {
		if (pane->get_panel_count() >= 3) {
			break;
		}
		bool has = false;
		for (int i = 0; i < pane->get_panel_count(); i++) {
			has = has || pane->get_panel_type_at(i) == type;
		}
		if (!has) {
			pane->add_panel(type);
		}
	}
	if (pane->get_panel_count() < 3) {
		_check(false, "a pane with three tabs to reorder");
		return;
	}
	const StringName first = pane->get_panel_type_at(0);
	const StringName second = pane->get_panel_type_at(1);

	// Let go past the middle of the second tab: the mark says after it.
	TabBar *bar = pane->get_tab_bar();
	const Rect2 tab = bar->get_tab_rect(1);
	const Point2 in_bar = tab.get_center() + Vector2(tab.size.x * 0.25, 0);
	const Point2 in_pane = pane->get_global_transform().affine_inverse().xform(bar->get_global_transform().xform(in_bar));
	Dictionary data;
	data["type"] = "editor_pane_panel";
	data["pane"] = (int64_t)pane->get_instance_id();
	data["index"] = 0;
	pane->accept_drop(in_pane, data);
	_check(pane->get_panel_type_at(0) == second && pane->get_panel_type_at(1) == first, "a tab let go past the middle of another lands after it, where the mark was");
}

void EditorSelfTest::_dock_menus_follow_focus() {
	TypedArray<Node> menus = SceneTreeDock::get_singleton()->find_children("*", "PopupMenu", true, false);
	String not_following;
	for (int i = 0; i < menus.size(); i++) {
		Window *menu = Object::cast_to<Window>(menus[i]);
		if (!menu->is_transient_to_focused()) {
			not_following += " " + String(menu->get_parent()->get_name()) + "/" + String(menu->get_name());
		}
	}
	_check(menus.size() > 0 && not_following.is_empty(), "the Scene dock's menus belong to the window they are opened from" + (not_following.is_empty() ? String() : " - not:" + not_following));
}

void EditorSelfTest::_finish() {
	remove_error_handler(&error_handler);
	const uint32_t error_count = errors.get();
	print_line(vformat("SELFTEST DONE: %d passed, %d failed, %d errors", passed, failed, error_count));
	get_tree()->quit(failed > 0 || error_count > 0 ? 1 : 0);
}

void EditorSelfTest::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			if (!started) {
				// Once the project has been looked through and the layout put
				// back, and then some: plenty is deferred on the way up.
				EditorFileSystem *file_system = EditorFileSystem::get_singleton();
				if (file_system && (file_system->is_scanning() || file_system->doing_first_scan())) {
					return;
				}
				if (!EditorNode::get_singleton()->get_edited_scene()) {
					EditorNode::get_singleton()->load_scene("res://scene_a.tscn");
				}
				started = true;
				frames_to_wait = 30;
				return;
			}
			if (frames_to_wait > 0) {
				frames_to_wait--;
				return;
			}
			if (next_step >= (int)steps.size()) {
				return;
			}
			const Step &step = steps[next_step++];
			print_line("SELFTEST STEP: " + step.name);
			step.run.call();
			frames_to_wait = 3;
		} break;
	}
}

EditorSelfTest::EditorSelfTest() {
	set_name("EditorSelfTest");
	error_handler.errfunc = _error_handler;
	error_handler.userdata = this;
	add_error_handler(&error_handler);

	_add("begin", callable_mp(this, &EditorSelfTest::_begin));
	_add("scene panel", callable_mp(this, &EditorSelfTest::_scene_panel));
	_add("inspector panel", callable_mp(this, &EditorSelfTest::_inspector_panel));
	_add("script open", callable_mp(this, &EditorSelfTest::_script_open));
	_add("script reopen", callable_mp(this, &EditorSelfTest::_script_reopen));
	_add("shader open", callable_mp(this, &EditorSelfTest::_shader_open));
	_add("shader close tab", callable_mp(this, &EditorSelfTest::_shader_close_tab));
	_add("shader back in its pane", callable_mp(this, &EditorSelfTest::_shader_back_in_its_pane));
	_add("shader close pane", callable_mp(this, &EditorSelfTest::_shader_close_pane));
	_add("shader pane closed", callable_mp(this, &EditorSelfTest::_shader_pane_closed));
	_add("shader back in its place", callable_mp(this, &EditorSelfTest::_shader_back_in_its_place));
	_add("node does not pull docks", callable_mp(this, &EditorSelfTest::_node_does_not_pull_docks));
	_add("reopen closed", callable_mp(this, &EditorSelfTest::_reopen_closed));
	_add("maximize", callable_mp(this, &EditorSelfTest::_maximize));
	_add("maximize by shortcut", callable_mp(this, &EditorSelfTest::_maximize_by_shortcut));
	_add("restore by shortcut", callable_mp(this, &EditorSelfTest::_restore_by_shortcut));
	_add("drop zones prepare", callable_mp(this, &EditorSelfTest::_drop_zones_prepare));
	_add("drop zones hold steady", callable_mp(this, &EditorSelfTest::_drop_zones_hold_steady));
	_add("tab lands where marked", callable_mp(this, &EditorSelfTest::_tab_lands_where_marked));
	_add("dock menus follow focus", callable_mp(this, &EditorSelfTest::_dock_menus_follow_focus));
	_add("finish", callable_mp(this, &EditorSelfTest::_finish));
}

EditorSelfTest::~EditorSelfTest() {
	remove_error_handler(&error_handler);
}
