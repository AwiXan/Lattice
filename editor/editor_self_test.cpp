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

#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_crash_report.h"
#include "editor/editor_data.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_scene_recovery.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_pane.h"
#include "editor/gui/editor_pane_tree.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/editor_scene_panel.h"
#include "editor/script/editor_script_panel.h"
#include "editor/script/script_editor_base.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/gui/code_editor.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_bar.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

bool EditorScreenshot::is_requested() {
	return OS::get_singleton()->has_environment("LATTICE_SHOT");
}

void EditorScreenshot::_open_scene() {
	const String scene = OS::get_singleton()->get_environment("LATTICE_SHOT_SCENE");
	if (!scene.is_empty()) {
		EditorNode::get_singleton()->load_scene(scene);
	}
}

void EditorScreenshot::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			started_at = OS::get_singleton()->get_ticks_msec();
			set_process(true);
			callable_mp(this, &EditorScreenshot::_open_scene).call_deferred();
		} break;

		case NOTIFICATION_PROCESS: {
			const uint64_t elapsed = OS::get_singleton()->get_ticks_msec() - started_at;
			if (!selected && elapsed > 2000) {
				selected = true;
				Node *root = EditorNode::get_singleton()->get_edited_scene();
				TypedArray<Node> meshes = root ? root->find_children("*", "MeshInstance3D", true, false) : TypedArray<Node>();
				if (!meshes.is_empty()) {
					EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
					selection->clear();
					selection->add_node(Object::cast_to<Node>(meshes[0]));
				}
			}
			if (!taken && elapsed > 4500) {
				taken = true;
				Ref<Image> image = get_tree()->get_root()->get_texture()->get_image();
				const String path = OS::get_singleton()->get_environment("LATTICE_SHOT");
				// Part of it only, to look at something small up close.
				const PackedStringArray crop = OS::get_singleton()->get_environment("LATTICE_SHOT_CROP").split(",");
				if (image.is_valid() && crop.size() == 4) {
					const Rect2i region = Rect2i(crop[0].to_int(), crop[1].to_int(), crop[2].to_int(), crop[3].to_int()).intersection(Rect2i(Point2i(), image->get_size()));
					if (region.has_area()) {
						image = image->get_region(region);
					}
				}
				if (image.is_valid()) {
					image->save_png(path);
					print_line("SHOT SAVED: " + path);
				} else {
					print_line("SHOT FAILED: nothing drawn");
				}
				get_tree()->quit(image.is_valid() ? 0 : 1);
			}
		} break;
	}
}

EditorScreenshot::EditorScreenshot() {
	set_name("EditorScreenshot");
}

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

void EditorSelfTest::_crash_report() {
	// misc/scripts/lattice_selftest.py leaves a session behind that never
	// closed, with a crash in its log.
	_check(EditorCrashReport::did_previous_session_crash(), "a session that never closed is noticed");
	const String excerpt = EditorCrashReport::get_excerpt();
	_check(EditorCrashReport::was_backtrace_found() && excerpt.contains("Program crashed") && excerpt.contains("END OF C++ BACKTRACE"), "and the backtrace is read from its log");
	bool shown = false;
	TypedArray<Node> reports = EditorNode::get_singleton()->get_gui_base()->find_children("*", "EditorCrashReport", true, false);
	for (int i = 0; i < reports.size(); i++) {
		Window *report = Object::cast_to<Window>(reports[i]);
		if (report && report->is_visible()) {
			shown = true;
			report->hide();
		}
	}
	_check(shown, "and shown when the editor opens");
	_check(FileAccess::exists(ProjectSettings::get_singleton()->get_project_data_path().path_join("editor/session_running")), "this session is marked as running");
}

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

EditorPane *EditorSelfTest::_script_pane(int *r_index) const {
	for (EditorPane *pane : _tree()->get_panes()) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_type_at(i) == StringName("script") && pane->get_panel_subject_at(i) == Variant("res://probe.gd")) {
				if (r_index) {
					*r_index = i;
				}
				return pane;
			}
		}
	}
	return nullptr;
}

static bool _probe_open_in_script_editor() {
	for (const Ref<Script> &script : ScriptEditor::get_singleton()->get_open_scripts()) {
		if (script.is_valid() && script->get_path() == "res://probe.gd") {
			return true;
		}
	}
	return false;
}

void EditorSelfTest::_script_open() {
	_edit("res://probe.gd");
}

void EditorSelfTest::_script_opened() {
	int index = -1;
	EditorPane *pane = _script_pane(&index);
	_check(pane != nullptr, "opening a script gives it a panel of its own");
	if (!pane) {
		return;
	}
	script_pane = pane->get_instance_id();
	EditorScriptPanel *panel = Object::cast_to<EditorScriptPanel>(pane->get_panel_at(index));
	_check(panel && panel->get_editor() && ScriptEditor::get_singleton()->is_editor_lent(panel->get_editor()), "the script editor lends it the script's editor");
	_check(_probe_open_in_script_editor(), "and still counts it among the scripts open");
	_check(pane->get_tab_bar()->get_tab_title(index) == "probe.gd", "its tab says which script");

	// A change, as typing would make one; the editor looks at it a moment later.
	TextEditorBase *editor = panel ? Object::cast_to<TextEditorBase>(panel->get_editor()) : nullptr;
	if (editor) {
		editor->get_code_editor()->get_text_editor()->insert_text("\n# changed by the self-test", 0, 0);
	}
	seconds_to_wait = 3.0;
	waiting_since = OS::get_singleton()->get_ticks_msec();
}

void EditorSelfTest::_script_edited() {
	int index = -1;
	EditorPane *pane = _script_pane(&index);
	const PackedStringArray unsaved = ScriptEditor::get_singleton()->get_unsaved_scripts();
	_check(pane && pane->get_tab_bar()->get_tab_title(index).ends_with("(*)") && !unsaved.is_empty(), "changed, its tab says so, and the script editor knows it is not saved");
	if (pane) {
		EditorScriptPanel *panel = Object::cast_to<EditorScriptPanel>(pane->get_panel_at(index));
		// Working in it makes it the script the script editor acts on.
		ScriptEditor::get_singleton()->activate_lent_editor(panel->get_editor());
		ScriptEditor::get_singleton()->save_current_script();
	}
}

void EditorSelfTest::_script_saved() {
	int index = -1;
	EditorPane *pane = _script_pane(&index);
	const String title = pane ? pane->get_tab_bar()->get_tab_title(index) : String();
	const PackedStringArray unsaved = ScriptEditor::get_singleton()->get_unsaved_scripts();
	_check(pane && !title.ends_with("(*)") && unsaved.is_empty(), "saved from its panel, it is saved" + (pane && !title.ends_with("(*)") && unsaved.is_empty() ? String() : vformat(" - tab '%s', unsaved %s", title, String(", ").join(unsaved))));
}

void EditorSelfTest::_script_close() {
	int index = -1;
	EditorPane *pane = _script_pane(&index);
	if (pane) {
		_close_tab(pane, "script");
	}
}

void EditorSelfTest::_script_closed() {
	_check(!_script_pane() && !_probe_open_in_script_editor(), "closing its panel closes the script");
	_edit("res://probe.gd");
}

void EditorSelfTest::_script_back_where_it_was() {
	EditorPane *pane = _script_pane();
	_check(pane && pane->get_instance_id() == script_pane, "opened again, it is back in the pane it was closed in");
	// Changed again, and closed without saving.
	int index = -1;
	pane = _script_pane(&index);
	EditorScriptPanel *panel = pane ? Object::cast_to<EditorScriptPanel>(pane->get_panel_at(index)) : nullptr;
	TextEditorBase *editor = panel ? Object::cast_to<TextEditorBase>(panel->get_editor()) : nullptr;
	if (editor) {
		editor->get_code_editor()->get_text_editor()->insert_text("\n# changed again", 0, 0);
	}
	seconds_to_wait = 3.0;
	waiting_since = OS::get_singleton()->get_ticks_msec();
}

void EditorSelfTest::_script_close_unsaved() {
	int index = -1;
	EditorPane *pane = _script_pane(&index);
	if (pane) {
		_close_tab(pane, "script");
	}
}

void EditorSelfTest::_script_close_unsaved_asked() {
	// Asked, as closing its tab in the script editor would be - and still
	// open, not lost, until answered.
	bool asked = false;
	TypedArray<Node> dialogs = ScriptEditor::get_singleton()->find_children("*", "ConfirmationDialog", true, false);
	for (int i = 0; i < dialogs.size(); i++) {
		Window *dialog = Object::cast_to<Window>(dialogs[i]);
		if (dialog && dialog->is_visible()) {
			asked = true;
			dialog->hide();
		}
	}
	_check(asked && _probe_open_in_script_editor() && !ScriptEditor::get_singleton()->get_unsaved_scripts().is_empty(), "closing it with changes not saved asks first, and loses nothing");
}

void EditorSelfTest::_second_script_open() {
	_edit("res://probe.gd");
}

void EditorSelfTest::_second_script_stacked() {
	// probe.gd has a panel again; a second script goes beside it.
	EditorPane *first = _script_pane();
	_edit("res://probe_b.gd");
	callable_mp_static(&EditorSelfTest::_check_second_script).call_deferred(get_instance_id(), first ? first->get_instance_id() : ObjectID());
}

void EditorSelfTest::_check_second_script(ObjectID p_self, ObjectID p_first_pane) {
	EditorSelfTest *self = ObjectDB::get_instance<EditorSelfTest>(p_self);
	if (!self) {
		return;
	}
	// Twice deferred: the script editor shows the new panel deferred too.
	callable_mp(self, &EditorSelfTest::_second_script_check).call_deferred(p_first_pane);
}

void EditorSelfTest::_second_script_check(ObjectID p_first_pane) {
	EditorPane *second = nullptr;
	for (EditorPane *pane : _tree()->get_panes()) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_type_at(i) == StringName("script") && pane->get_panel_subject_at(i) == Variant("res://probe_b.gd")) {
				second = pane;
			}
		}
	}
	_check(second && second->get_instance_id() == p_first_pane, "a second script gets its own panel, stacked with the first");
}

void EditorSelfTest::_view_chrome() {
	Node3DEditor *view = Node3DEditor::get_singleton();
	Control *column = view->get_tool_column();
	_check(column && column->find_children("*", "Button", true, false).size() >= 11, "the 3D tools are down the side of the view");
	HFlowContainer *header = view->get_toolbar();
	const bool menus_first = header && header->get_child_count() > 0 && !header->get_child(0)->find_children("*", "MenuButton", true, false).is_empty();
	const bool end_last = header && view->get_header_end() && view->get_header_end()->get_index() == header->get_child_count() - 1;
	_check(menus_first && end_last, "the 3D header has its menus first and the view's display at the far end");

	// Whatever an addon hands the 3D view goes where it always went, and
	// comes back out again.
	EditorPlugin *plugin = memnew(EditorPlugin);
	bool placed = true;
	const EditorPlugin::CustomControlContainer containers[] = {
		EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU,
		EditorPlugin::CONTAINER_SPATIAL_EDITOR_SIDE_LEFT,
		EditorPlugin::CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT,
		EditorPlugin::CONTAINER_SPATIAL_EDITOR_BOTTOM,
	};
	for (EditorPlugin::CustomControlContainer container : containers) {
		Button *button = memnew(Button);
		plugin->add_control_to_container(container, button);
		placed = placed && view->is_ancestor_of(button);
		if (container == EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU) {
			placed = placed && button->get_parent() == view->get_context_toolbar();
		}
		plugin->remove_control_from_container(container, button);
		placed = placed && !button->get_parent();
		memdelete(button);
	}
	memdelete(plugin);
	_check(placed, "an addon's controls go into the 3D view's containers and come back out, as before");
}

void EditorSelfTest::_view_shading() {
	Node3DEditor *view = Node3DEditor::get_singleton();
	Button *wireframe = view->get_shading_button(0);
	Button *normal = view->get_shading_button(3);
	if (!wireframe || !normal) {
		_check(false, "the 3D header has the four shading buttons");
		return;
	}
	// The header's button reaches every viewport of the view.
	wireframe->emit_signal(SceneStringName(pressed));
	bool all_wireframe = true;
	for (uint32_t i = 0; i < Node3DEditor::VIEWPORTS_COUNT; i++) {
		all_wireframe = all_wireframe && view->get_editor_viewport(i)->get_viewport_node()->get_debug_draw() == Viewport::DEBUG_DRAW_WIREFRAME;
	}
	_check(all_wireframe && wireframe->is_pressed() && !normal->is_pressed(), "a shading button in the 3D header sets every viewport of the view");
	// And the view's own menu moves the header's buttons with it.
	view->get_editor_viewport(0)->get_view_menu()->get_popup()->emit_signal(SceneStringName(id_pressed), Node3DEditorViewport::get_display_normal_id());
	_check(normal->is_pressed() && !wireframe->is_pressed(), "changing how a viewport draws from its own menu updates the header");
}

void EditorSelfTest::_view_overlays() {
	Node3DEditor *view = Node3DEditor::get_singleton();
	MenuButton *menu = view->get_overlays_menu();
	if (!menu) {
		_check(false, "the 3D header has an Overlays menu");
		return;
	}
	PopupMenu *popup = menu->get_popup();
	popup->emit_signal(SNAME("about_to_popup"));
	const bool listed = popup->get_item_count() > Node3DEditor::OVERLAY_MAX;
	const bool had = view->is_overlay_shown_everywhere(Node3DEditor::OVERLAY_INFORMATION);
	popup->emit_signal(SceneStringName(id_pressed), (int)Node3DEditor::OVERLAY_INFORMATION);
	const bool shown = view->is_overlay_shown_everywhere(Node3DEditor::OVERLAY_INFORMATION);
	popup->emit_signal(SceneStringName(id_pressed), (int)Node3DEditor::OVERLAY_INFORMATION);
	const bool hidden_again = !view->is_overlay_shown_everywhere(Node3DEditor::OVERLAY_INFORMATION);
	_check(listed && !had && shown && hidden_again, "an overlay switched in the Overlays menu is switched in every viewport of the view");
}

EditorPane *EditorSelfTest::_pane_with_script(const String &p_path, int *r_index) const {
	for (EditorPane *pane : _tree()->get_panes()) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_type_at(i) == StringName("script") && pane->get_panel_subject_at(i) == Variant(p_path)) {
				if (r_index) {
					*r_index = i;
				}
				return pane;
			}
		}
	}
	return nullptr;
}

void EditorSelfTest::_script_moved() {
	// probe_b.gd dragged off to a pane of its own, and another script opened.
	int index = -1;
	EditorPane *from = _pane_with_script("res://probe_b.gd", &index);
	EditorPane *to = from ? _tree()->split_pane(from, false, false, false) : nullptr;
	if (to) {
		from->transfer_panel_to(to, index);
		moved_script_pane = to->get_instance_id();
	}
	_edit("res://probe_c.gd");
}

void EditorSelfTest::_script_follows_the_move() {
	EditorPane *pane = _pane_with_script("res://probe_c.gd");
	_check(pane && pane->get_instance_id() == moved_script_pane, "a script opens in the pane a script was last moved to");
	EditorPane *moved = ObjectDB::get_instance<EditorPane>(moved_script_pane);
	if (moved) {
		_tree()->close_pane(moved);
	}
}

void EditorSelfTest::_script_pane_gone() {
	_edit("res://probe_c.gd");
}

void EditorSelfTest::_script_with_the_others() {
	// That pane is gone: with the scripts still open, not in a new pane.
	EditorPane *pane = _pane_with_script("res://probe_c.gd");
	EditorPane *others = _pane_with_script("res://probe.gd");
	_check(pane && pane == others && !ObjectDB::get_instance<EditorPane>(moved_script_pane), "with that pane gone, it opens with the other scripts");
}

void EditorSelfTest::_script_left_open() {
	// Left in its own panel for the editor to quit with.
	_edit("res://probe_b.gd");
}

void EditorSelfTest::_script_stand_in() {
	// The script editor itself, showing the tab of a script a panel has.
	EditorPane *pane = _tree()->get_first_pane();
	pane->show_panel_of_type(EditorMainScreen::get_main_panel_type_id(EditorNode::get_editor_data().get_editor_by_name("Script")));
	bool stand_in = false;
	TypedArray<Node> stand_ins = ScriptEditor::get_singleton()->find_children("*", "ScriptEditorStandIn", true, false);
	for (int i = 0; i < stand_ins.size(); i++) {
		TypedArray<Node> buttons = Object::cast_to<Node>(stand_ins[i])->find_children("*", "Button", true, false);
		stand_in = stand_in || !buttons.is_empty();
	}
	_check(stand_in, "the script editor keeps a stand-in, with a way to the panel, for a script shown elsewhere");
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

void EditorSelfTest::_compass_prepare() {
	_tree()->set_maximized_pane(_tree()->get_first_pane());
}

void EditorSelfTest::_compass_targets() {
	EditorPane *pane = _tree()->get_first_pane();
	const Rect2 into = pane->get_compass_target_rect(EditorPane::DROP_INTO);
	const Rect2 left = pane->get_compass_target_rect(EditorPane::DROP_LEFT);
	_tree()->set_maximized_pane(nullptr);
	_check(into.has_area() && left.has_area(), "a pane with room for it has a compass");
	_check(pane->get_compass_zone_at(into.get_center()) == EditorPane::DROP_INTO && pane->get_compass_zone_at(left.get_center()) == EditorPane::DROP_LEFT, "its targets say where a panel goes");
	_check(pane->get_compass_zone_at(pane->get_body_rect().position + Vector2(4, 4)) == EditorPane::DROP_NONE, "and away from them the edges decide, as before");
}

void EditorSelfTest::_whole_side() {
	EditorPaneTree *tree = _tree();
	const Rect2 left = tree->get_edge_target_rect(EditorPane::DROP_LEFT);
	_check(tree->get_panes().size() > 1 && left.has_area() && tree->get_edge_target_at(left.get_center()) == EditorPane::DROP_LEFT, "with more than one pane, each edge of the arrangement has a target");
	panes_before_shader = tree->get_panes().size();
	EditorPane *fresh = tree->split_root(false, true, 0.3);
	shader_pane = fresh ? fresh->get_instance_id() : ObjectID();
}

void EditorSelfTest::_whole_side_check() {
	EditorPaneTree *tree = _tree();
	EditorPane *fresh = ObjectDB::get_instance<EditorPane>(shader_pane);
	const EditorPaneTree::Place place = fresh ? tree->get_place_of(fresh) : EditorPaneTree::Place();
	const Rect2 whole = Rect2(Point2(), tree->get_size());
	_check(fresh && tree->get_panes().size() == panes_before_shader + 1 && place.before && !place.vertical && Math::abs(place.ratio - 0.3) < 0.01, "a whole side gets a new pane beside everything there is");
	_check(fresh && Math::abs(fresh->get_rect().size.y - whole.size.y) < 1 && fresh->get_rect().position.x < 1, "running the full height of the left side");
	if (fresh) {
		tree->close_pane(fresh);
	}
}

void EditorSelfTest::_drop_from_another_window() {
	// What following a drag into another window comes down to: the pointer,
	// in screen coordinates, handed to that window's hint.
	EditorPaneTree *tree = _tree();
	Vector<EditorPane *> panes = tree->get_panes();
	if (panes.size() < 2) {
		_check(false, "two panes to drag between");
		return;
	}
	EditorPane *from = panes[0];
	EditorPane *to = panes[panes.size() - 1];
	const StringName moving = from->get_panel_type_at(from->get_current_panel());
	const int from_count = from->get_panel_count();
	Dictionary data;
	data["type"] = "editor_pane_panel";
	data["pane"] = (int64_t)from->get_instance_id();
	data["index"] = from->get_current_panel();
	// Over the middle of the target pane: into it.
	const Point2 at = to->get_screen_position() + to->get_body_rect().get_center();
	const bool taken = tree->get_drop_hint()->drop_external(at, data);
	bool arrived = false;
	for (int i = 0; i < to->get_panel_count(); i++) {
		arrived = arrived || to->get_panel_type_at(i) == moving;
	}
	_check(taken && arrived && from->get_panel_count() == from_count - 1, "a panel dropped from another window lands in the pane under the pointer");
	_check(!tree->get_drop_hint()->is_visible(), "and the hint that showed it is put away");
}

// Saved in the editor's own layouts, which are the user's too: named so as to
// be unmistakable, and taken away again at the end - or at the start, after a
// run that did not get that far.
static const char *WORKSPACE_A = "Lattice Self-Test A";
static const char *WORKSPACE_B = "Lattice Self-Test B";

void EditorSelfTest::_workspaces_save() {
	EditorNode *editor = EditorNode::get_singleton();
	editor->delete_workspace(WORKSPACE_A);
	editor->delete_workspace(WORKSPACE_B);

	EditorPaneTree *tree = _tree();
	while (tree->get_panes().size() > 1) {
		tree->close_pane(tree->get_panes()[tree->get_panes().size() - 1]);
	}
	tree->get_first_pane()->set_panel_type("view_3d");
	editor->save_workspace(WORKSPACE_A);

	EditorPane *side = tree->split_pane(tree->get_first_pane(), false, true, false);
	side->add_panel("dock_FileSystem");
	editor->save_workspace(WORKSPACE_B);
	_check(editor->get_workspace_names().has(WORKSPACE_A) && editor->get_workspace_names().has(WORKSPACE_B) && editor->get_current_workspace() == WORKSPACE_B, "arrangements can be saved as workspaces");
}

void EditorSelfTest::_workspaces_switch_back() {
	EditorNode::get_singleton()->switch_workspace(WORKSPACE_A);
	_check(_tree()->get_panes().size() == 1 && !_pane_showing("dock_FileSystem") && EditorNode::get_singleton()->get_current_workspace() == WORKSPACE_A, "switching to one puts its arrangement back");
}

void EditorSelfTest::_workspaces_switch_again() {
	EditorNode::get_singleton()->switch_workspace(WORKSPACE_B);
	_check(_tree()->get_panes().size() == 2 && _pane_showing("dock_FileSystem") != nullptr, "and switching back brings back what the other had, the FileSystem included");
}

void EditorSelfTest::_workspaces_change_one() {
	EditorPane *side = _pane_showing("dock_FileSystem");
	if (side) {
		_tree()->close_pane(side);
	}
	EditorNode::get_singleton()->switch_workspace(WORKSPACE_A);
}

void EditorSelfTest::_workspaces_remembered() {
	EditorNode *editor = EditorNode::get_singleton();
	editor->switch_workspace(WORKSPACE_B);
	_check(_tree()->get_panes().size() == 1 && !_pane_showing("dock_FileSystem"), "a workspace keeps what was changed in it when it is left");
	editor->delete_workspace(WORKSPACE_A);
	editor->delete_workspace(WORKSPACE_B);
	_check(!editor->get_workspace_names().has(WORKSPACE_A) && !editor->get_workspace_names().has(WORKSPACE_B) && editor->get_current_workspace().is_empty(), "and workspaces can be deleted");
}

void EditorSelfTest::_hidden_inspector_prepare() {
	EditorPane *pane = _tree()->get_first_pane();
	pane->show_panel_of_type("inspector");
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	if (root) {
		EditorNode::get_singleton()->push_item(root);
	}
}

void EditorSelfTest::_hidden_inspector_idle() {
	// The dock's inspector is still the one the editor edits things in, but
	// nobody sees it: it should not be building property editors for anyone.
	const int hidden = InspectorDock::get_inspector_singleton()->find_children("*", "EditorProperty", true, false).size();
	int shown = 0;
	EditorPane *pane = _pane_showing("inspector");
	if (pane) {
		shown = pane->get_panel()->find_children("*", "EditorProperty", true, false).size();
		pane->close_panel(pane->get_current_panel());
	}
	_check(shown > 0 && hidden == 0, vformat("the hidden Inspector dock builds nothing, the panel does (%d, %d)", hidden, shown));
}

void EditorSelfTest::_recovery_offered() {
	// misc/scripts/lattice_selftest.py leaves a copy of scene_b behind, with a
	// node the file on disk does not have.
	_check(EditorSceneRecovery::get_previous_count() == 1, "a scene the crashed session had not saved is offered back");
	bool offered = false;
	TypedArray<Node> reports = EditorNode::get_singleton()->get_gui_base()->find_children("*", "EditorCrashReport", true, false);
	for (int i = 0; i < reports.size(); i++) {
		TypedArray<Node> buttons = Object::cast_to<Node>(reports[i])->find_children("*", "Button", true, false);
		for (int j = 0; j < buttons.size(); j++) {
			Button *button = Object::cast_to<Button>(buttons[j]);
			offered = offered || (button->is_visible() && button->get_text().begins_with("Restore"));
		}
	}
	_check(offered, "by the crash report");
	_check(EditorSceneRecovery::get_singleton() && EditorSceneRecovery::get_singleton()->restore_previous() == 1, "and brought back");
}

void EditorSelfTest::_recovery_restored() {
	EditorData &editor_data = EditorNode::get_editor_data();
	bool found = false;
	String seen;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node *root = editor_data.get_edited_scene_root(i);
		seen += vformat(" [%s root=%s recovered=%s unsaved=%s]", editor_data.get_scene_path(i), root ? String(root->get_name()) : String("-"), root && root->has_node(NodePath("Recovered")) ? "yes" : "no", EditorNode::get_singleton()->is_scene_unsaved(i) ? "yes" : "no");
		if (editor_data.get_scene_path(i) == "res://scene_b.tscn" && root) {
			found = root->has_node(NodePath("Recovered")) && EditorNode::get_singleton()->is_scene_unsaved(i);
		}
	}
	_check(found, "in place of the file it came from, with its changes not yet saved there" + (found ? String() : " -" + seen));
}

void EditorSelfTest::_recovery_copies() {
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	if (!root) {
		_check(false, "a scene to change");
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action("Self-test change");
	undo_redo->add_do_property(root, "editor_description", "changed by the self-test");
	undo_redo->add_undo_property(root, "editor_description", root->get_editor_description());
	undo_redo->commit_action();
	EditorSceneRecovery::get_singleton()->save_now();

	const String dir = ProjectSettings::get_singleton()->get_project_data_path().path_join("editor/recovery/current");
	const String path = EditorNode::get_editor_data().get_scene_path(EditorNode::get_editor_data().get_edited_scene());
	Ref<ConfigFile> index;
	index.instantiate();
	bool listed = false;
	if (index->load(dir.path_join("index.cfg")) == OK) {
		for (const String &file : index->get_sections()) {
			listed = listed || (String(index->get_value(file, "path", "")) == path && FileAccess::exists(dir.path_join(file)));
		}
	}
	_check(listed, "a scene with unsaved changes is copied aside");
}

void EditorSelfTest::_scene_colors() {
	// By now scene_a and the recovered scene_b are both open.
	EditorData &editor_data = EditorNode::get_editor_data();
	if (editor_data.get_edited_scene_count() < 2) {
		_check(false, "two scenes open to tell apart");
		return;
	}
	const Color first = editor_data.get_scene_color(0);
	const Color second = editor_data.get_scene_color(1);
	_check(editor_data.are_scene_colors_shown() && first != second, "two open scenes have colors of their own");
	_check(editor_data.get_scene_color(0) == first, "and keep them");
}

void EditorSelfTest::_panel_from_palette() {
	EditorPane *pane = _pane_showing("dock_FileSystem");
	if (pane) {
		_close_tab(pane, "dock_FileSystem");
	}
	EditorCommandPalette::get_singleton()->execute_command(EditorPanelRegistry::get_palette_command_key("dock_FileSystem"));
}

void EditorSelfTest::_panel_from_palette_shown() {
	_check(_pane_showing("dock_FileSystem") != nullptr, "a panel can be shown from the Command Palette, by name");
}

void EditorSelfTest::_remote_filter() {
	EditorPane *pane = _tree()->get_first_pane();
	const int index = pane->show_panel_of_type("scene_tree");
	EditorScenePanel *panel = index >= 0 ? Object::cast_to<EditorScenePanel>(pane->get_panel_at(index)) : nullptr;
	if (!panel) {
		_check(false, "a Scene panel to look at the running game in");
		return;
	}
	panel->set_remote(true);
	int buttons_shown = 0;
	bool filter_shown = false;
	TypedArray<Node> controls = panel->get_child(0)->get_children();
	for (int i = 0; i < controls.size(); i++) {
		Control *control = Object::cast_to<Control>(controls[i]);
		filter_shown = filter_shown || (Object::cast_to<LineEdit>(control) && control->is_visible_in_tree());
		buttons_shown += Object::cast_to<Button>(control) && control->is_visible() ? 1 : 0;
	}
	_check(filter_shown && buttons_shown == 0, "looking at the running game, the filter stays and the editing buttons go");
	panel->set_remote(false);
	pane->close_panel(index);
}

static Ref<InputEventKey> _pane_key(Key p_key) {
	Ref<InputEventKey> key;
	key.instantiate();
	key->set_keycode(p_key);
	key->set_ctrl_pressed(true);
	key->set_shift_pressed(true);
	key->set_alt_pressed(true);
	key->set_pressed(true);
	return key;
}

void EditorSelfTest::_keys_prepare() {
	EditorPaneTree *tree = _tree();
	while (tree->get_panes().size() > 1) {
		tree->close_pane(tree->get_panes()[tree->get_panes().size() - 1]);
	}
	EditorPane *left = tree->get_first_pane();
	left->set_panel_type("view_3d");
	EditorPane *right = tree->split_pane(left, false, false, false);
	right->add_panel("scene_tree");
	right->add_panel("inspector");
	shader_pane = right->get_instance_id();
	tree->set_active_pane(left);
	left->focus_current_panel();
}

void EditorSelfTest::_keys_move() {
	EditorNode::get_singleton()->get_window()->push_input(_pane_key(Key::RIGHT));
	EditorPane *right = ObjectDB::get_instance<EditorPane>(shader_pane);
	const Control *focus = EditorNode::get_singleton()->get_viewport()->gui_get_focus_owner();
	_check(right && _tree()->get_active_pane() == right && focus && right->is_ancestor_of(focus), "Ctrl+Shift+Alt+Right moves the keyboard to the pane on the right");
}

void EditorSelfTest::_keys_tabs() {
	EditorPane *right = ObjectDB::get_instance<EditorPane>(shader_pane);
	if (!right) {
		return;
	}
	const int before = right->get_current_panel();
	EditorNode::get_singleton()->get_window()->push_input(_pane_key(Key::PAGEDOWN));
	_check(right->get_current_panel() == (before + 1) % right->get_panel_count(), "Ctrl+Shift+Alt+PageDown goes to its next tab");
	const int count = right->get_panel_count();
	EditorNode::get_singleton()->get_window()->push_input(_pane_key(Key::W));
	_check(right->get_panel_count() == count - 1, "and Ctrl+Shift+Alt+W closes the tab it is on");
}

static int _document_of(const String &p_path) {
	EditorData &editor_data = EditorNode::get_editor_data();
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == p_path) {
			return editor_data.get_scene_history_id(i);
		}
	}
	return -1;
}

static Node3DEditor *_view_of(int p_document, int p_nth = 0) {
	for (Node3DEditor *editor : Node3DEditor::get_instances()) {
		if (editor->get_bound_document() == p_document && p_nth-- == 0) {
			return editor;
		}
	}
	return nullptr;
}

void EditorSelfTest::_worlds_prepare() {
	if (_document_of("res://scene_b.tscn") < 0) {
		EditorNode::get_singleton()->load_scene("res://scene_b.tscn");
	}
	const int a = _document_of("res://scene_a.tscn");
	const int b = _document_of("res://scene_b.tscn");
	EditorPaneTree *tree = _tree();
	while (tree->get_panes().size() > 1) {
		tree->close_pane(tree->get_panes()[tree->get_panes().size() - 1]);
	}
	// Two views of one scene, and one of another: three views, two worlds.
	EditorPane *first = tree->get_first_pane();
	first->set_panel_type("view_3d", a);
	EditorPane *second = tree->split_pane(first, false, false, false);
	second->add_panel("view_3d", b);
	EditorPane *third = tree->split_pane(second, true, false, false);
	third->add_panel("view_3d", a);
}

void EditorSelfTest::_worlds_check() {
	const int a = _document_of("res://scene_a.tscn");
	const int b = _document_of("res://scene_b.tscn");
	Node3DEditor *view_a = _view_of(a);
	Node3DEditor *view_b = _view_of(b);
	if (!view_a || !view_b || !_view_of(a, 1)) {
		_check(false, "three 3D views, two of them on one scene");
		return;
	}
	const RID world_a = view_a->get_editing_world()->get_scenario();
	const RID world_b = view_b->get_editing_world()->get_scenario();
	_check(world_a != world_b && Node3DEditor::world_has_grid_and_origin(world_a) && Node3DEditor::world_has_grid_and_origin(world_b), "each scene's world has its own grid and origin lines");
	_check(Node3DEditor::count_preview_suns_in(world_a) == 1 && Node3DEditor::count_preview_suns_in(world_b) == 1, "and one preview sun, however many views show it");

	// The camera of the other scene's view travelling far enough to rebuild
	// its grid used to take the grid away from this one.
	Camera3D *camera = view_b->get_editor_viewport(0)->get_camera_3d();
	camera->set_position(camera->get_position() + Vector3(500, 0, 0));
	view_b->update_grid();
}

void EditorSelfTest::_worlds_after_camera_moved() {
	const int a = _document_of("res://scene_a.tscn");
	const int b = _document_of("res://scene_b.tscn");
	Node3DEditor *view_a = _view_of(a);
	Node3DEditor *view_b = _view_of(b);
	if (!view_a || !view_b) {
		return;
	}
	_check(Node3DEditor::world_has_grid_and_origin(view_a->get_editing_world()->get_scenario()) && Node3DEditor::world_has_grid_and_origin(view_b->get_editing_world()->get_scenario()), "one scene's camera travelling leaves the other scene its grid");
	// Closing one of the two views of a scene leaves the other lighting it.
	EditorPane *pane = nullptr;
	for (EditorPane *candidate : _tree()->get_panes()) {
		if (!pane && candidate->get_panel_type() == StringName("view_3d") && candidate->get_panel_subject() == Variant(a)) {
			pane = candidate;
		}
	}
	if (pane) {
		_tree()->close_pane(pane);
	}
}

void EditorSelfTest::_worlds_one_view_closed() {
	const int a = _document_of("res://scene_a.tscn");
	Node3DEditor *view_a = _view_of(a);
	if (!view_a) {
		_check(false, "a view of the scene left");
		return;
	}
	const RID world_a = view_a->get_editing_world()->get_scenario();
	_check(Node3DEditor::world_has_grid_and_origin(world_a) && Node3DEditor::count_preview_suns_in(world_a) == 1, "closing one of two views of a scene leaves the other with its grid and preview sun");
}

void EditorSelfTest::_finish() {
	remove_error_handler(&error_handler);
	const uint32_t error_count = errors.get();
	print_line(vformat("SELFTEST DONE: %d passed, %d failed, %d errors", passed, failed, error_count));
	OS::get_singleton()->printerr("SELFTEST QUITTING\n");
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
			if (seconds_to_wait > 0.0 && (OS::get_singleton()->get_ticks_msec() - waiting_since) < seconds_to_wait * 1000.0) {
				return;
			}
			seconds_to_wait = 0.0;
			if (next_step >= (int)steps.size()) {
				return;
			}
			const Step &step = steps[next_step++];
			print_line("SELFTEST STEP: " + step.name);
			// On the error stream too, where errors are: which step an error
			// came in shows there, in order.
			OS::get_singleton()->printerr("SELFTEST STEP: %s\n", step.name.utf8().get_data());
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

	_add("crash report", callable_mp(this, &EditorSelfTest::_crash_report));
	_add("begin", callable_mp(this, &EditorSelfTest::_begin));
	_add("scene panel", callable_mp(this, &EditorSelfTest::_scene_panel));
	_add("inspector panel", callable_mp(this, &EditorSelfTest::_inspector_panel));
	_add("script open", callable_mp(this, &EditorSelfTest::_script_open));
	_add("script opened", callable_mp(this, &EditorSelfTest::_script_opened));
	_add("script edited", callable_mp(this, &EditorSelfTest::_script_edited));
	_add("script saved", callable_mp(this, &EditorSelfTest::_script_saved));
	_add("script close", callable_mp(this, &EditorSelfTest::_script_close));
	_add("script closed", callable_mp(this, &EditorSelfTest::_script_closed));
	_add("script back where it was", callable_mp(this, &EditorSelfTest::_script_back_where_it_was));
	_add("script close unsaved", callable_mp(this, &EditorSelfTest::_script_close_unsaved));
	_add("script close unsaved asked", callable_mp(this, &EditorSelfTest::_script_close_unsaved_asked));
	_add("second script open", callable_mp(this, &EditorSelfTest::_second_script_open));
	_add("second script stacked", callable_mp(this, &EditorSelfTest::_second_script_stacked));
	_add("script moved", callable_mp(this, &EditorSelfTest::_script_moved));
	_add("script follows the move", callable_mp(this, &EditorSelfTest::_script_follows_the_move));
	_add("script pane gone", callable_mp(this, &EditorSelfTest::_script_pane_gone));
	_add("script with the others", callable_mp(this, &EditorSelfTest::_script_with_the_others));
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
	_add("compass prepare", callable_mp(this, &EditorSelfTest::_compass_prepare));
	_add("compass targets", callable_mp(this, &EditorSelfTest::_compass_targets));
	_add("whole side", callable_mp(this, &EditorSelfTest::_whole_side));
	_add("whole side check", callable_mp(this, &EditorSelfTest::_whole_side_check));
	_add("drop from another window", callable_mp(this, &EditorSelfTest::_drop_from_another_window));
	_add("workspaces save", callable_mp(this, &EditorSelfTest::_workspaces_save));
	_add("workspaces switch back", callable_mp(this, &EditorSelfTest::_workspaces_switch_back));
	_add("workspaces switch again", callable_mp(this, &EditorSelfTest::_workspaces_switch_again));
	_add("workspaces change one", callable_mp(this, &EditorSelfTest::_workspaces_change_one));
	_add("workspaces remembered", callable_mp(this, &EditorSelfTest::_workspaces_remembered));
	_add("hidden inspector prepare", callable_mp(this, &EditorSelfTest::_hidden_inspector_prepare));
	_add("hidden inspector idle", callable_mp(this, &EditorSelfTest::_hidden_inspector_idle));
	_add("recovery offered", callable_mp(this, &EditorSelfTest::_recovery_offered));
	_add("recovery restored", callable_mp(this, &EditorSelfTest::_recovery_restored));
	_add("recovery copies", callable_mp(this, &EditorSelfTest::_recovery_copies));
	_add("scene colors", callable_mp(this, &EditorSelfTest::_scene_colors));
	_add("panel from palette", callable_mp(this, &EditorSelfTest::_panel_from_palette));
	_add("panel from palette shown", callable_mp(this, &EditorSelfTest::_panel_from_palette_shown));
	_add("remote filter", callable_mp(this, &EditorSelfTest::_remote_filter));
	_add("keys prepare", callable_mp(this, &EditorSelfTest::_keys_prepare));
	_add("keys move", callable_mp(this, &EditorSelfTest::_keys_move));
	_add("keys tabs", callable_mp(this, &EditorSelfTest::_keys_tabs));
	_add("worlds prepare", callable_mp(this, &EditorSelfTest::_worlds_prepare));
	_add("worlds check", callable_mp(this, &EditorSelfTest::_worlds_check));
	_add("worlds after camera moved", callable_mp(this, &EditorSelfTest::_worlds_after_camera_moved));
	_add("worlds one view closed", callable_mp(this, &EditorSelfTest::_worlds_one_view_closed));
	_add("view chrome", callable_mp(this, &EditorSelfTest::_view_chrome));
	_add("view shading", callable_mp(this, &EditorSelfTest::_view_shading));
	_add("view overlays", callable_mp(this, &EditorSelfTest::_view_overlays));
	_add("script left open", callable_mp(this, &EditorSelfTest::_script_left_open));
	_add("script stand-in", callable_mp(this, &EditorSelfTest::_script_stand_in));
	_add("finish", callable_mp(this, &EditorSelfTest::_finish));
}

EditorSelfTest::~EditorSelfTest() {
	remove_error_handler(&error_handler);
}
