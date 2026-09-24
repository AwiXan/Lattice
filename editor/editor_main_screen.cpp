/**************************************************************************/
/*  editor_main_screen.cpp                                                */
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

#include "editor_main_screen.h"

#include "core/io/config_file.h"
#include "core/object/callable_mp.h"
#include "editor/editor_document_view.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/gui/editor_pane.h"
#include "editor/gui/editor_pane_window.h"
#include "editor/gui/editor_pane_tree.h"
#include "editor/editor_string_names.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "scene/gui/button.h"

void EditorMainScreen::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			_watch_drag();
		} break;

		case NOTIFICATION_READY: {
			set_accessibility_region(true);
			// What a fresh editor shows. A saved arrangement replaces it a moment
			// later; this is what there is when there is none.
			if (EDITOR_3D < plugin_allowed.size() && plugin_allowed[EDITOR_3D]) {
				select(EDITOR_3D);
				return;
			}
			for (int i = 0; i < plugin_allowed.size(); i++) {
				if (plugin_allowed[i]) {
					select(i);
					return;
				}
			}
		} break;
	}
}

void EditorMainScreen::save_layout_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const {
	const int selected_main_editor_idx = get_selected_index();
	if (selected_main_editor_idx != -1) {
		p_config_file->set_value(p_section, "selected_main_editor_idx", selected_main_editor_idx);
	} else {
		p_config_file->set_value(p_section, "selected_main_editor_idx", Variant());
	}

	// How the panes were arranged and what was in them. Written as panel type
	// ids and scene paths, so it means the same thing next time the editor runs.
	if (pane_tree) {
		p_config_file->set_value(p_section, "panes", pane_tree->save_layout());
	}

	Array windows;
	for (const EditorPaneWindow *window : pane_windows) {
		windows.push_back(window->save_layout());
	}
	p_config_file->set_value(p_section, "pane_windows", windows);
}

void EditorMainScreen::load_layout_from_config(Ref<ConfigFile> p_config_file, const String &p_section) {
	int selected_main_editor_idx = p_config_file->get_value(p_section, "selected_main_editor_idx", -1);
	if (selected_main_editor_idx >= 0 && selected_main_editor_idx < editor_table.size()) {
		callable_mp(this, &EditorMainScreen::select).call_deferred(selected_main_editor_idx);
	}

	const Dictionary panes = p_config_file->get_value(p_section, "panes", Dictionary());
	if (!panes.is_empty()) {
		// Deferred, because a pane that was on a particular scene is written
		// down by that scene's path, and the scenes are not open yet when the
		// layout is read.
		callable_mp(this, &EditorMainScreen::_restore_panes).call_deferred(panes);
	}

	const Array windows = p_config_file->get_value(p_section, "pane_windows", Array());
	if (!windows.is_empty()) {
		// After the main arrangement, for the same reason: a panel in a window
		// may be pointed at a scene that is not open yet.
		callable_mp(this, &EditorMainScreen::_restore_pane_windows).call_deferred(windows);
	}
}

void EditorMainScreen::save_workspace_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const {
	if (pane_tree) {
		p_config_file->set_value(p_section, "panes", pane_tree->save_layout());
	}
	Array windows;
	for (const EditorPaneWindow *window : pane_windows) {
		windows.push_back(window->save_layout());
	}
	p_config_file->set_value(p_section, "pane_windows", windows);
}

bool EditorMainScreen::load_workspace_from_config(Ref<ConfigFile> p_config_file, const String &p_section) {
	const Dictionary panes = p_config_file->get_value(p_section, "panes", Dictionary());
	if (panes.is_empty() || !pane_tree) {
		return false;
	}

	// The windows of the arrangement being left go with it.
	while (!pane_windows.is_empty()) {
		_close_pane_window(pane_windows[0], false);
	}

	pane_tree->load_layout(panes);
	_restore_pane_windows(p_config_file->get_value(p_section, "pane_windows", Array()));
	return true;
}

void EditorMainScreen::_restore_panes(const Dictionary &p_layout) {
	if (pane_tree) {
		pane_tree->load_layout(p_layout);
	}
}

bool EditorMainScreen::show_panel(const StringName &p_type) {
	// A dock asked for by name may have something standing in for it.
	const StringName type = EditorPanelRegistry::resolve(p_type);
	if (!EditorPanelRegistry::has_type(type)) {
		return false;
	}
	// The pane being worked in, if it has one; otherwise wherever one is
	// already showing, in this window or another - there is one script editor,
	// one of each dock, and a request for one that tried to make a second in
	// the pane last used was quietly refused - and only failing both, a new one
	// here.
	EditorPane *active = pane_tree ? pane_tree->get_active_pane() : nullptr;
	EditorPaneWindow *window = nullptr;
	EditorPane *pane = nullptr;
	if (active) {
		for (int i = 0; i < active->get_panel_count(); i++) {
			if (active->get_panel_type_at(i) == type) {
				pane = active;
				break;
			}
		}
	}
	if (!pane) {
		pane = _pane_showing(type, &window);
	}
	if (!pane) {
		pane = _pane_for_new(type, last_places.getptr(type));
		window = _window_of(pane);
	}
	if (!pane) {
		return false;
	}
	pane->show_panel_of_type(type);
	if (window) {
		window->grab_window_focus();
	}
	return true;
}

EditorPaneWindow *EditorMainScreen::_window_of(const EditorPane *p_pane) const {
	if (!p_pane) {
		return nullptr;
	}
	for (EditorPaneWindow *window : pane_windows) {
		if (window->get_pane_tree()->is_ancestor_of(p_pane)) {
			return window;
		}
	}
	return nullptr;
}

EditorPane *EditorMainScreen::_pane_for_new(const StringName &p_type, const PanelPlace *p_place) {
	if (p_place) {
		// Where it was: the same pane, or the one made again in its place.
		EditorPane *pane = ObjectDB::get_instance<EditorPane>(p_place->pane);
		if (!pane) {
			const ObjectID *remade = remade_panes.getptr(p_place->pane);
			pane = remade ? ObjectDB::get_instance<EditorPane>(*remade) : nullptr;
		}
		if (pane && pane->is_inside_tree()) {
			return pane;
		}
		// That pane is gone; one is made beside what it was beside.
		EditorPaneTree *tree = ObjectDB::get_instance<EditorPaneTree>(p_place->tree);
		EditorPane *made = tree ? tree->make_pane_at(p_place->at) : nullptr;
		if (made) {
			remade_panes[p_place->pane] = made->get_instance_id();
			return made;
		}
	}

	// Never anywhere yet: beside the pane being worked in, on the side this
	// kind of panel has always been on - a shader editor below, the FileSystem
	// to the left - or failing that, in the pane itself.
	EditorPane *active = pane_tree ? pane_tree->get_active_pane() : nullptr;
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_type);
	if (active && type && type->side != EditorPanelRegistry::SIDE_NONE && active->get_panel_count() > 0) {
		EditorPaneTree::Place at;
		at.neighbor = active->get_instance_id();
		switch (type->side) {
			case EditorPanelRegistry::SIDE_LEFT: {
				at.before = true;
				at.ratio = 0.25;
			} break;
			case EditorPanelRegistry::SIDE_RIGHT: {
				at.ratio = 0.75;
			} break;
			case EditorPanelRegistry::SIDE_BOTTOM: {
				at.vertical = true;
				at.ratio = 0.65;
			} break;
			default: {
			} break;
		}
		EditorPane *made = pane_tree->make_pane_at(at);
		if (made) {
			return made;
		}
	}
	return active;
}

StringName EditorMainScreen::get_main_panel_type_id(const EditorPlugin *p_editor) {
	return p_editor ? _main_panel_type_id(p_editor) : StringName();
}

EditorPane *EditorMainScreen::open_panel(const StringName &p_type, const Variant &p_subject) {
	const StringName type = EditorPanelRegistry::resolve(p_type);
	if (!EditorPanelRegistry::has_type(type)) {
		return nullptr;
	}
	const PanelPlace *place = last_places.getptr(type);
	const EditorPane *remembered = place ? ObjectDB::get_instance<EditorPane>(place->pane) : nullptr;
	EditorPane *pane = nullptr;
	if (!remembered || !remembered->is_inside_tree()) {
		// The pane the last one was worked with is gone. With the others of the
		// kind still open, rather than in a pane of its own where that one was:
		// a script goes with the scripts.
		pane = _pane_showing(type);
	}
	if (!pane) {
		pane = _pane_for_new(type, place);
	}
	if (!pane) {
		return nullptr;
	}
	const int at = pane->add_panel(type, p_subject);
	if (at < 0) {
		return nullptr;
	}
	if (pane_tree && pane_tree->is_ancestor_of(pane)) {
		pane_tree->set_active_pane(pane);
	}
	EditorPaneWindow *window = _window_of(pane);
	if (window) {
		window->grab_window_focus();
	}
	// Where the next one of the kind goes too.
	note_panel_touched(pane, at);
	return pane;
}

static EditorPane *_pane_holding(EditorPaneTree *p_tree, Control *p_panel, int *r_index) {
	if (!p_tree) {
		return nullptr;
	}
	for (EditorPane *pane : p_tree->get_panes()) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_at(i) == p_panel) {
				*r_index = i;
				return pane;
			}
		}
	}
	return nullptr;
}

bool EditorMainScreen::reveal_panel(Control *p_panel) {
	int index = -1;
	EditorPane *pane = _pane_holding(pane_tree, p_panel, &index);
	for (int i = 0; !pane && i < pane_windows.size(); i++) {
		pane = _pane_holding(pane_windows[i]->get_pane_tree(), p_panel, &index);
	}
	if (!pane) {
		return false;
	}
	pane->set_current_panel(index);
	EditorPaneWindow *window = _window_of(pane);
	if (window) {
		window->grab_window_focus();
	}
	return true;
}

void EditorMainScreen::remove_panel(Control *p_panel) {
	int index = -1;
	EditorPane *pane = _pane_holding(pane_tree, p_panel, &index);
	for (int i = 0; !pane && i < pane_windows.size(); i++) {
		pane = _pane_holding(pane_windows[i]->get_pane_tree(), p_panel, &index);
	}
	if (pane) {
		pane->close_panel(index);
	}
}

EditorMainScreen::PanelPlace EditorMainScreen::_place_of(EditorPane *p_pane) {
	PanelPlace place;
	place.pane = p_pane->get_instance_id();
	for (Node *n = p_pane->get_parent(); n; n = n->get_parent()) {
		EditorPaneTree *tree = Object::cast_to<EditorPaneTree>(n);
		if (tree) {
			place.tree = tree->get_instance_id();
			place.at = tree->get_place_of(p_pane);
			break;
		}
	}
	return place;
}

void EditorMainScreen::note_panel_touched(EditorPane *p_pane, int p_index) {
	ERR_FAIL_NULL(p_pane);
	const StringName type = p_pane->get_panel_type_at(p_index);
	if (type != StringName()) {
		last_places[type] = _place_of(p_pane);
	}
}

void EditorMainScreen::note_panel_closing(EditorPane *p_pane, int p_index) {
	ERR_FAIL_NULL(p_pane);
	const StringName type = p_pane->get_panel_type_at(p_index);
	if (type == StringName()) {
		return;
	}
	EditorPanelRegistry::notify_closed_by_user(type, p_pane->get_panel_at(p_index));

	const PanelPlace place = _place_of(p_pane);
	last_places[type] = place;

	ClosedPanel closed;
	closed.type = type;
	closed.subject = p_pane->get_panel_subject_at(p_index);
	closed.state = EditorPanelRegistry::save_panel_state(type, p_pane->get_panel_at(p_index));
	closed.title = p_pane->get_panel_title_at(p_index);
	closed.place = place;
	closed_panels.push_back(closed);
	// Enough to undo a few closes in a row; not a history of the session.
	while (closed_panels.size() > 12) {
		closed_panels.remove_at(0);
	}
}

bool EditorMainScreen::reopen_closed_panel(int p_index) {
	if (closed_panels.is_empty()) {
		return false;
	}
	const int index = p_index < 0 ? closed_panels.size() - 1 : p_index;
	ERR_FAIL_INDEX_V(index, closed_panels.size(), false);
	const ClosedPanel closed = closed_panels[index];
	closed_panels.remove_at(index);

	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(closed.type);
	if (!type) {
		// Unregistered since - an addon turned off.
		return false;
	}
	if (type->lent) {
		// There is one of it: if it is showing somewhere already, that is
		// where it is.
		EditorPaneWindow *window = nullptr;
		EditorPane *showing = _pane_showing(closed.type, &window);
		if (showing) {
			showing->show_panel_of_type(closed.type);
			if (window) {
				window->grab_window_focus();
			}
			return true;
		}
	}

	EditorPane *pane = _pane_for_new(closed.type, &closed.place);
	if (!pane) {
		return false;
	}
	const int at = pane->add_panel(closed.type, closed.subject);
	if (at < 0) {
		return false;
	}
	EditorPanelRegistry::load_panel_state(closed.type, pane->get_panel_at(at), closed.state);
	EditorPaneWindow *window = _window_of(pane);
	if (window) {
		window->grab_window_focus();
	}
	return true;
}

String EditorMainScreen::get_closed_panel_title(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, closed_panels.size(), String());
	return closed_panels[p_index].title;
}

StringName EditorMainScreen::get_closed_panel_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, closed_panels.size(), StringName());
	return closed_panels[p_index].type;
}

EditorPane *EditorMainScreen::_pane_showing(const StringName &p_type, EditorPaneWindow **r_window) const {
	auto search = [&p_type](EditorPaneTree *p_tree) -> EditorPane * {
		for (EditorPane *pane : p_tree->get_panes()) {
			for (int i = 0; i < pane->get_panel_count(); i++) {
				if (pane->get_panel_type_at(i) == p_type) {
					return pane;
				}
			}
		}
		return nullptr;
	};
	if (pane_tree) {
		EditorPane *found = search(pane_tree);
		if (found) {
			return found;
		}
	}
	for (EditorPaneWindow *window : pane_windows) {
		EditorPane *found = search(window->get_pane_tree());
		if (found) {
			if (r_window) {
				*r_window = window;
			}
			return found;
		}
	}
	return nullptr;
}

void EditorMainScreen::_watch_tree(EditorPaneTree *p_tree) {
	p_tree->connect(SNAME("panel_float_requested"), callable_mp(this, &EditorMainScreen::_panel_float_requested).bind(p_tree));
}

EditorPaneWindow *EditorMainScreen::open_panel_in_window(EditorPane *p_from, int p_panel, const Rect2i &p_rect) {
	ERR_FAIL_NULL_V(p_from, nullptr);
	if (!EditorNode::get_singleton()->is_multi_window_enabled()) {
		return nullptr;
	}

	EditorPaneWindow *window = memnew(EditorPaneWindow);
	// Not in the main screen: that is a container, and it would lay this out to
	// fill it - an empty stand-in over the whole editor, swallowing every
	// click. The docks park theirs in the same place for the same reason.
	EditorNode::get_singleton()->get_gui_base()->add_child(window);
	_watch_tree(window->get_pane_tree());
	window->connect("window_close_requested", callable_mp(this, &EditorMainScreen::_pane_window_closed).bind(window));
	pane_windows.push_back(window);

	EditorPane *host = window->get_pane_tree()->get_first_pane();
	if (!host || !p_from->transfer_panel_to(host, p_panel)) {
		// Nothing moved, so there is nothing for the window to show.
		_close_pane_window(window, false);
		return nullptr;
	}
	if (pane_tree) {
		pane_tree->drop_empty_panes();
	}

	window->update_title();
	// A third of the editor, near where it came from, which is what the docks
	// do when they are made floating.
	if (p_rect.has_area()) {
		window->restore_window(p_rect, DisplayServer::get_singleton()->get_screen_from_rect(Rect2(p_rect)));
	} else {
		const Size2i size = EditorNode::get_singleton()->get_window()->get_size() / 2;
		const Point2i at = p_from->get_screen_position();
		window->restore_window(Rect2i(at, size), EditorNode::get_singleton()->get_gui_base()->get_window()->get_current_screen());
	}
	window->grab_window_focus();
	return window;
}

void EditorMainScreen::begin_panel_drag(Viewport *p_viewport, const Variant &p_data) {
	ERR_FAIL_NULL(p_viewport);
	drag_viewport = p_viewport->get_instance_id();
	// Kept: by the time the drag is known to be over, the viewport has let go
	// of what was being dragged.
	drag_data = p_data;
	drag_hint = ObjectID();
	set_process_internal(true);
}

Window *EditorMainScreen::_window_at(const Point2i &p_screen_position) const {
	const DisplayServerEnums::WindowID id = DisplayServer::get_singleton()->get_window_at_screen_position(p_screen_position);
	if (id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return nullptr;
	}
	return Object::cast_to<Window>(ObjectDB::get_instance(DisplayServer::get_singleton()->window_get_attached_instance_id(id)));
}

EditorPaneTree *EditorMainScreen::_tree_in_window(const Window *p_window) const {
	if (!p_window) {
		return nullptr;
	}
	if (pane_tree && pane_tree->get_window() == p_window) {
		return pane_tree;
	}
	for (EditorPaneWindow *window : pane_windows) {
		if (window->get_pane_tree()->get_window() == p_window) {
			return window->get_pane_tree();
		}
	}
	return nullptr;
}

void EditorMainScreen::_watch_drag() {
	Viewport *viewport = ObjectDB::get_instance<Viewport>(drag_viewport);
	const Point2i mouse = DisplayServer::get_singleton()->mouse_get_position();
	// Another of the editor's windows under the pointer: the one the drag
	// started in looks after itself.
	Window *under = _window_at(mouse);
	EditorPaneTree *tree = under != viewport ? _tree_in_window(under) : nullptr;
	EditorPaneDropHint *hint = tree ? tree->get_drop_hint() : nullptr;
	EditorPaneDropHint *previous = ObjectDB::get_instance<EditorPaneDropHint>(drag_hint);
	if (previous && previous != hint) {
		previous->end_external();
	}
	drag_hint = hint ? hint->get_instance_id() : ObjectID();

	if (viewport && viewport->gui_is_dragging()) {
		if (hint) {
			hint->track_external(mouse, drag_data);
		}
		return;
	}

	// The drag is over.
	set_process_internal(false);
	const Variant data = drag_data;
	drag_data = Variant();
	drag_viewport = ObjectID();
	drag_hint = ObjectID();
	if (!viewport || viewport->gui_is_drag_successful()) {
		if (hint) {
			hint->end_external();
		}
		return;
	}
	if (hint) {
		hint->drop_external(mouse, data);
		return;
	}
	if (!under) {
		// Let go outside every window of the editor, the way a browser tab is
		// torn off.
		_tear_off(data, mouse);
	}
}

void EditorMainScreen::_tear_off(const Variant &p_data, const Point2i &p_screen_position) {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return;
	}
	const Dictionary data = p_data;
	if (String(data.get("type", "")) != "editor_pane_panel") {
		// A scene tab or a file: a description of something, not a panel that
		// already exists to be moved.
		return;
	}
	EditorPane *pane = ObjectDB::get_instance<EditorPane>(ObjectID(uint64_t(int64_t(data.get("pane", 0)))));
	const int index = data.get("index", -1);
	if (!pane || index < 0 || index >= pane->get_panel_count()) {
		return;
	}
	// The size it had, near where it was let go, with the pointer on its
	// title bar as if it had been carried there.
	Control *panel = pane->get_panel_at(index);
	const Size2i size = (panel ? Size2i(panel->get_size()) : Size2i()).max(Size2i(Size2(480, 320) * EDSCALE));
	const Point2i at = p_screen_position - Point2i(Size2(60, 12) * EDSCALE);
	open_panel_in_window(pane, index, Rect2i(at, size));
}

void EditorMainScreen::_panel_float_requested(EditorPane *p_pane, int p_panel, EditorPaneTree *p_tree) {
	if (!p_tree->is_windowed()) {
		open_panel_in_window(p_pane, p_panel);
		return;
	}

	// The other direction: back into the main arrangement, beside whatever is
	// being worked on there.
	EditorPane *home = pane_tree ? pane_tree->get_active_pane() : nullptr;
	if (!home) {
		return;
	}
	p_pane->transfer_panel_to(home, p_panel);
	p_tree->drop_empty_panes();

	for (EditorPaneWindow *window : pane_windows) {
		if (window->get_pane_tree() != p_tree) {
			continue;
		}
		bool empty = true;
		for (EditorPane *pane : p_tree->get_panes()) {
			empty = empty && pane->get_panel_count() == 0;
		}
		if (empty) {
			// Nothing left to look at.
			_close_pane_window(window, false);
		} else {
			window->update_title();
		}
		break;
	}
}

void EditorMainScreen::_pane_window_closed(EditorPaneWindow *p_window) {
	// Closing a window closes what is in it, the way closing a window does
	// everywhere else. Whatever cannot be closed - a dock, the script editor -
	// goes back where it lives. interface/panes/when_a_window_closes turns this
	// into bringing everything back instead, for those who would rather.
	const bool bring_back = int(EDITOR_GET("interface/panes/when_a_window_closes")) == 1;
	if (!bring_back) {
		for (EditorPane *pane : p_window->get_pane_tree()->get_panes()) {
			for (int i = pane->get_panel_count() - 1; i >= 0; i--) {
				note_panel_closing(pane, i);
			}
		}
	}
	_close_pane_window(p_window, bring_back);
}

void EditorMainScreen::_close_pane_window(EditorPaneWindow *p_window, bool p_keep_panels) {
	ERR_FAIL_NULL(p_window);

	if (p_keep_panels && pane_tree) {
		// Closing a window is not throwing away what is in it: whatever it
		// still holds comes back to the arrangement it came from.
		EditorPane *home = pane_tree->get_active_pane();
		if (home) {
			for (EditorPane *pane : p_window->get_pane_tree()->get_panes()) {
				while (pane->get_panel_count() > 0) {
					if (!pane->transfer_panel_to(home, 0)) {
						break;
					}
				}
			}
		}
	}

	// What it still holds goes now, while the window is in the tree: what it
	// was lent goes home at once - there is one FileSystem and one script
	// editor, and whoever closed the window may want either next - and a
	// script in a panel goes back to a script editor that is not on its way
	// out with the window.
	for (EditorPane *pane : p_window->get_pane_tree()->get_panes()) {
		while (pane->get_panel_count() > 0) {
			pane->close_panel(0);
		}
	}

	pane_windows.erase(p_window);
	p_window->queue_free();
}

void EditorMainScreen::_restore_pane_windows(const Array &p_windows) {
	if (!EditorNode::get_singleton()->is_multi_window_enabled()) {
		// The arrangement said windows and this editor has none, so what they
		// held stays in the main one rather than being lost.
		return;
	}
	for (int i = 0; i < p_windows.size(); i++) {
		const Dictionary data = p_windows[i];
		if (data.is_empty()) {
			continue;
		}
		EditorPaneWindow *window = memnew(EditorPaneWindow);
		EditorNode::get_singleton()->get_gui_base()->add_child(window);
		_watch_tree(window->get_pane_tree());
		window->connect("window_close_requested", callable_mp(this, &EditorMainScreen::_pane_window_closed).bind(window));
		pane_windows.push_back(window);
		window->load_layout(data);
	}
}

void EditorMainScreen::set_button_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, plugin_allowed.size());
	plugin_allowed.write[p_index] = p_enabled;
	if (!p_enabled && selected_plugin == editor_table[p_index]) {
		select(EDITOR_2D);
	}
}

bool EditorMainScreen::is_button_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, plugin_allowed.size(), false);
	return plugin_allowed[p_index];
}

int EditorMainScreen::_get_current_main_editor() const {
	for (int i = 0; i < editor_table.size(); i++) {
		if (editor_table[i] == selected_plugin) {
			return i;
		}
	}

	return 0;
}

void EditorMainScreen::select_next() {
	int editor = _get_current_main_editor();

	do {
		if (editor == editor_table.size() - 1) {
			editor = 0;
		} else {
			editor++;
		}
	} while (!plugin_allowed[editor]);

	select(editor);
}

void EditorMainScreen::select_prev() {
	int editor = _get_current_main_editor();

	do {
		if (editor == 0) {
			editor = editor_table.size() - 1;
		} else {
			editor--;
		}
	} while (!plugin_allowed[editor]);

	select(editor);
}

void EditorMainScreen::select_by_name(const String &p_name) {
	ERR_FAIL_COND(p_name.is_empty());

	for (int i = 0; i < editor_table.size(); i++) {
		if (editor_table[i]->get_plugin_name() == p_name) {
			select(i);
			return;
		}
	}

	ERR_FAIL_MSG("The editor name '" + p_name + "' was not found.");
}

void EditorMainScreen::select(int p_index) {
	if (EditorNode::get_singleton()->is_changing_scene()) {
		return;
	}

	ERR_FAIL_INDEX(p_index, editor_table.size());

	if (p_index >= plugin_allowed.size() || !plugin_allowed[p_index]) {
		// Turned off by a feature profile.
		return;
	}

	EditorPlugin *new_editor = editor_table[p_index];
	ERR_FAIL_NULL(new_editor);

	// Shown every time it is asked for, not only when it was not the one
	// selected already. Selected used to mean "on screen"; now it only means
	// "last asked for", and the panel may have been closed since - which is why
	// opening a second script after closing the first showed nothing at all.
	const bool changed = selected_plugin != new_editor;
	selected_plugin = new_editor;
	const StringName type = selected_plugin->get_main_screen_panel_type() != StringName()
			? selected_plugin->get_main_screen_panel_type()
			: _main_panel_type_id(selected_plugin);

	if (Object::cast_to<ScriptEditorPlugin>(selected_plugin) && ScriptEditor::opens_scripts_in_panels()) {
		// Scripts have panels of their own. Which one is known once whatever
		// asked for the script editor has opened the script in it.
		callable_mp(ScriptEditor::get_singleton(), &ScriptEditor::show_current_in_panel).call_deferred();
	} else if (!show_panel(type)) {
		// Nothing can show it - an addon that keeps its view to itself - so it
		// falls back to the way it always worked.
		selected_plugin->make_visible(true);
	}
	selected_plugin->selected_notify();
	if (!changed) {
		return;
	}
	// A scene dropped on a pane becomes this kind of view, unless the pane it
	// lands on is already showing one and has its own answer. A main screen
	// that is not a document view - the script editor, the asset library -
	// leaves the last answer standing rather than clearing it.
	const StringName selected_panel_type = selected_plugin->get_main_screen_panel_type();
	if (selected_panel_type != StringName()) {
		EditorPanelRegistry::set_default_type_for(EditorPanelRegistry::BINDING_DOCUMENT, selected_panel_type);
	}
	set_accessibility_name(selected_plugin->get_plugin_name());

	EditorData &editor_data = EditorNode::get_editor_data();
	int plugin_count = editor_data.get_editor_plugin_count();
	for (int i = 0; i < plugin_count; i++) {
		editor_data.get_editor_plugin(i)->notify_main_screen_changed(selected_plugin->get_plugin_name());
	}

	EditorNode::get_singleton()->update_distraction_free_mode();
}

int EditorMainScreen::get_selected_index() const {
	for (int i = 0; i < editor_table.size(); i++) {
		if (selected_plugin == editor_table[i]) {
			return i;
		}
	}
	return -1;
}

int EditorMainScreen::get_plugin_index(EditorPlugin *p_editor) const {
	int screen = -1;
	for (int i = 0; i < editor_table.size(); i++) {
		if (p_editor == editor_table[i]) {
			screen = i;
			break;
		}
	}
	return screen;
}

EditorPlugin *EditorMainScreen::get_selected_plugin() const {
	return selected_plugin;
}

EditorPlugin *EditorMainScreen::get_plugin_by_name(const String &p_plugin_name) const {
	ERR_FAIL_COND_V(!main_editor_plugins.has(p_plugin_name), nullptr);
	return main_editor_plugins[p_plugin_name];
}

bool EditorMainScreen::can_auto_switch_screens() const {
	if (selected_plugin == nullptr) {
		return true;
	}
	// Only from a main screen that comes before the script editor: opening a
	// scene should not pull the view away from someone writing code.
	for (int i = 0; i < editor_table.size(); i++) {
		if (i == EDITOR_SCRIPT) {
			return false;
		}
		if (editor_table[i] == selected_plugin) {
			return true;
		}
	}
	return false;
}

VBoxContainer *EditorMainScreen::get_control() const {
	return main_screen_vbox;
}

void EditorMainScreen::_panes_changed() {
	// Which scene a view shows is its pane's business, written down in the pane
	// and nowhere else. This used to hold the first view to a scene whenever
	// there were several panes and let go of it when there was one again - from
	// the outside, without the pane knowing - so a view could be following the
	// current scene while its header said it was pinned to another, and choosing
	// "follow" in that header then did nothing, because it already was.
}

bool EditorMainScreen::is_split_view_enabled() const {
	return pane_tree && pane_tree->get_panes().size() > 1;
}

void EditorMainScreen::split_main_pane(bool p_vertical) {
	if (!pane_tree) {
		return;
	}

	EditorPane *pane = pane_tree->split_pane(pane_tree->get_active_pane(), p_vertical);
	if (!pane) {
		return;
	}
	// "Another of what I am looking at": the new pane starts on the selected
	// plugin's panel, pointed at the scene being worked on. The tree itself
	// knows nothing about main screen plugins; this is the one place that does.
	const StringName type = selected_plugin ? selected_plugin->get_main_screen_panel_type() : StringName();
	if (type != StringName()) {
		EditorData &editor_data = EditorNode::get_editor_data();
		const Variant subject = editor_data.get_edited_scene_count() > 0 ? Variant(editor_data.get_scene_history_id(editor_data.get_edited_scene())) : Variant(-1);
		pane->set_panel_type(type, subject);
	}
}

void EditorMainScreen::set_split_view_enabled(bool p_enabled) {
	if (!pane_tree || p_enabled == is_split_view_enabled()) {
		return;
	}

	if (p_enabled) {
		split_main_pane(false);
		return;
	}

	// Back to one: every pane but the one holding the main screen goes.
	Vector<EditorPane *> panes = pane_tree->get_panes();
	for (int i = panes.size() - 1; i >= 1; i--) {
		pane_tree->close_pane(panes[i]);
	}
}

void EditorMainScreen::view_activated(EditorDocumentView *p_view) {
	active_view = p_view;
	if (!p_view) {
		return;
	}

	// A scene dropped on a pane that has nothing to go by becomes the kind of
	// view last worked in, which is this one.
	const StringName panel_type = p_view->get_panel_type();
	if (panel_type != StringName()) {
		EditorPanelRegistry::set_default_type_for(EditorPanelRegistry::BINDING_DOCUMENT, panel_type);
	}

	if (changing_context) {
		return;
	}

	const int document_id = p_view->get_bound_document();
	if (document_id < 0) {
		// This view follows the current document already; there is nothing to
		// bring the rest of the editor to.
		return;
	}

	EditorData &editor_data = EditorNode::get_editor_data();
	const int idx = editor_data.get_scene_index_by_history_id(document_id);
	// A closed document leaves the view following the current one, and there is
	// nothing to switch to.
	if (idx < 0 || idx == editor_data.get_edited_scene()) {
		return;
	}

	changing_context = true;
	EditorNode::get_singleton()->set_current_scene_index(idx);
	changing_context = false;
}

void EditorMainScreen::current_document_changed() {
	// Nothing to do, on purpose. A view following the current document finds
	// out by itself; a view pointed at a scene stays on it. This used to point
	// the pane last worked in at whatever the tab bar picked, which overrode a
	// choice made in that pane's own header - and did it behind the pane's back,
	// so the header still named the old scene while the view showed the new.
}

StringName EditorMainScreen::_main_panel_type_id(const EditorPlugin *p_editor) {
	// The same name a saved layout uses, so an arrangement that says "the
	// script editor, here" still means it next time.
	return StringName("main_" + const_cast<EditorPlugin *>(p_editor)->get_plugin_name());
}

int EditorMainScreen::_rank_main_panel(const String &p_path, const StringName &p_class, EditorPlugin *p_editor) {
	return p_editor ? p_editor->rank_resource(p_path, p_class) : 0;
}

bool EditorMainScreen::_open_in_main_panel(Control *p_panel, const String &p_path, EditorPlugin *p_editor) {
	return p_editor ? p_editor->open_resource(p_path) : false;
}

Control *EditorMainScreen::_control_of(EditorPlugin *p_editor) {
	Control *known = p_editor->get_main_screen_control();
	if (known) {
		return known;
	}

	// An addon says what to show by making it visible, and never says which
	// Control that is. Asking it to show itself and seeing which of the parked
	// controls appears is how the editor finds out, and once is enough.
	const ObjectID *remembered = plugin_controls.getptr(p_editor->get_instance_id());
	if (remembered) {
		return ObjectDB::get_instance<Control>(*remembered);
	}

	HashSet<Control *> before;
	for (int i = 0; i < main_screen_vbox->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(main_screen_vbox->get_child(i));
		if (child && child->is_visible()) {
			before.insert(child);
		}
	}
	p_editor->make_visible(true);
	Control *found = nullptr;
	for (int i = 0; i < main_screen_vbox->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(main_screen_vbox->get_child(i));
		if (child && child->is_visible() && !before.has(child)) {
			found = child;
			break;
		}
	}
	if (found) {
		plugin_controls.insert(p_editor->get_instance_id(), found->get_instance_id());
	} else {
		p_editor->make_visible(false);
	}
	return found;
}

Control *EditorMainScreen::_lend_main_panel(EditorPlugin *p_editor) {
	ERR_FAIL_NULL_V(p_editor, nullptr);
	if (lent_plugins.has(p_editor)) {
		// There is one script editor, so a second pane cannot also show it.
		return nullptr;
	}
	Control *control = _control_of(p_editor);
	if (!control) {
		return nullptr;
	}

	lent_plugins.insert(p_editor);
	// It shows itself its own way, and then leaves the main screen for the pane
	// that asked. Nothing hides it while it is out; see select().
	p_editor->make_visible(true);
	if (control->get_parent()) {
		control->get_parent()->remove_child(control);
	}
	control->show();
	return control;
}

bool EditorMainScreen::_return_main_panel(Control *p_panel, EditorPlugin *p_editor) {
	ERR_FAIL_NULL_V(p_editor, false);
	lent_plugins.erase(p_editor);

	Control *control = _control_of(p_editor);
	if (!control) {
		return false;
	}
	if (control->get_parent()) {
		control->get_parent()->remove_child(control);
	}
	main_screen_vbox->add_child(control);
	control->hide();
	return true;
}

void EditorMainScreen::add_main_plugin(EditorPlugin *p_editor) {
	plugin_allowed.push_back(true);
	editor_table.push_back(p_editor);
	main_editor_plugins.insert(p_editor->get_plugin_name(), p_editor);

	// A main screen is a panel like any other. The ones that can be built twice
	// - the 2D and 3D views - name themselves; the rest are one of a kind, so
	// they are lent to whichever pane asks, and given back when it lets go.
	if (p_editor->get_main_screen_panel_type() == StringName()) {
		EditorPanelRegistry::PanelType type;
		type.id = _main_panel_type_id(p_editor);
		type.title = p_editor->get_plugin_name();
		type.icon = StringName(p_editor->get_plugin_name());
		type.icon_texture = p_editor->get_plugin_icon();
		type.binding = EditorPanelRegistry::BINDING_CONTEXT;
		type.lent = true;
		type.create = callable_mp(this, &EditorMainScreen::_lend_main_panel).bind(p_editor);
		type.release = callable_mp(this, &EditorMainScreen::_return_main_panel).bind(p_editor);
		type.rank = callable_mp_static(&EditorMainScreen::_rank_main_panel).bind(p_editor);
		type.open = callable_mp_static(&EditorMainScreen::_open_in_main_panel).bind(p_editor);
		EditorPanelRegistry::register_type(type);
	}
}

void EditorMainScreen::remove_main_plugin(EditorPlugin *p_editor) {
	const int index = editor_table.find(p_editor);
	if (index >= 0 && index < plugin_allowed.size()) {
		plugin_allowed.remove_at(index);
	}

	if (selected_plugin == p_editor) {
		selected_plugin = nullptr;
	}

	EditorPanelRegistry::unregister_type(_main_panel_type_id(p_editor));
	lent_plugins.erase(p_editor);
	editor_table.erase(p_editor);
	main_editor_plugins.erase(p_editor->get_plugin_name());
}

EditorMainScreen::EditorMainScreen() {
	ED_SHORTCUT("editor/toggle_maximize_pane", TTRC("Maximize or Restore Pane"), KeyModifierMask::CMD_OR_CTRL | Key::SPACE);
	// Working with panes is Ctrl+Shift+Alt, which the editor leaves nearly
	// alone: Ctrl+Shift+T reopens a script, and Ctrl+Alt with T, arrows or
	// digits already trims whitespace, duplicates lines and splits 3D views.
	const KeyModifierMask panes = KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | KeyModifierMask::ALT;
	ED_SHORTCUT("editor/reopen_closed_panel", TTRC("Reopen Closed Panel"), panes | Key::T);
	ED_SHORTCUT("editor/focus_pane_left", TTRC("Focus the Pane to the Left"), panes | Key::LEFT);
	ED_SHORTCUT("editor/focus_pane_right", TTRC("Focus the Pane to the Right"), panes | Key::RIGHT);
	ED_SHORTCUT("editor/focus_pane_up", TTRC("Focus the Pane Above"), panes | Key::UP);
	ED_SHORTCUT("editor/focus_pane_down", TTRC("Focus the Pane Below"), panes | Key::DOWN);
	ED_SHORTCUT("editor/next_pane_tab", TTRC("Next Tab in Pane"), panes | Key::PAGEDOWN);
	ED_SHORTCUT("editor/previous_pane_tab", TTRC("Previous Tab in Pane"), panes | Key::PAGEUP);
	ED_SHORTCUT("editor/close_pane_tab", TTRC("Close Tab in Pane"), panes | Key::W);

	pane_tree = memnew(EditorPaneTree);
	add_child(pane_tree);
	pane_tree->connect(SNAME("layout_changed"), callable_mp(this, &EditorMainScreen::_panes_changed));
	_watch_tree(pane_tree);

	// Where a main screen stands when no pane is showing it. Plugins parent
	// their views into this and addons reach it through EditorInterface, so it
	// is the same Control it has always been - it is simply not on screen any
	// more. What is on screen is panes, and a main screen is a panel in one.
	main_screen_vbox = memnew(VBoxContainer);
	main_screen_vbox->set_name("MainScreen");
	main_screen_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->add_theme_constant_override("separation", 0);
	main_screen_vbox->hide();
	add_child(main_screen_vbox);
}
