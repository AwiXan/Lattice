/**************************************************************************/
/*  editor_title_bar.cpp                                                  */
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

#include "editor_title_bar.h"

#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"

// Icons for the items of the main menus. An item is found by its shortcut, or
// by its text when it has none (submenus, and the menus themselves as entries
// of the compact main menu). Items left out keep what they have.
struct MainMenuIcon {
	const char *shortcut;
	const char *text;
	const char *icon;
};

static const MainMenuIcon main_menu_icons[] = {
	// The menus, as entries of the compact main menu.
	{ nullptr, "Scene", "PackedScene" },
	{ nullptr, "Project", "Folder" },
	{ nullptr, "Debug", "Debug" },
	{ nullptr, "Editor", "Tools" },
	{ nullptr, "Help", "Help" },

	// Scene.
	{ "editor/new_scene", nullptr, "New" },
	{ "editor/new_inherited_scene", nullptr, "CreateNewSceneFrom" },
	{ "editor/open_scene", nullptr, "Load" },
	{ nullptr, "Open Recent", "History" },
	{ "editor/save_scene", nullptr, "Save" },
	{ "ui_undo", nullptr, "UndoRedo" },
	{ "ui_redo", nullptr, "Redo" },
	{ "editor/quick_open", nullptr, "LoadQuick" },
	{ "editor/quick_open_scene", nullptr, "PackedScene" },
	{ "editor/quick_open_script", nullptr, "Script" },
	{ "editor/reload_saved_scene", nullptr, "Reload" },
	{ "editor/close_scene", nullptr, "Close" },

	// Project.
	{ "editor/project_settings", nullptr, "Settings" },
	{ "editor/find_in_files", nullptr, "Search" },
	{ "editor/replace_in_files", nullptr, "ReplaceText" },
	{ nullptr, "Version Control", "VcsBranches" },
	{ nullptr, "Tools", "Tools" },
	{ nullptr, "Open User Data Folder", "Folder" },
	{ "editor/reload_current_project", nullptr, "Reload" },
	{ "editor/quit_to_project_list", nullptr, "ProjectList" },

	// Debug.
	{ nullptr, "Customize Run Instances...", "InstanceOptions" },

	// Editor.
	{ "editor/editor_settings", nullptr, "Settings" },
	{ "editor/command_palette", nullptr, "Shortcut" },
	{ nullptr, "Editor Docks", "MakeFloating" },
	{ nullptr, "Editor Layout", "Panels3" },
	{ "editor/take_screenshot", nullptr, "Image" },
	{ "editor/fullscreen_mode", nullptr, "DistractionFree" },
	{ nullptr, "Open Editor Data/Settings Folder", "Folder" },
	{ nullptr, "Open Editor Data Folder", "Folder" },
	{ nullptr, "Open Editor Settings Folder", "Folder" },

	// Help. Search Help, Copy System Info, About and Support have theirs already.
	{ "editor/online_docs", nullptr, "ExternalLink" },
	{ "editor/forum", nullptr, "ExternalLink" },
	{ "editor/community", nullptr, "ExternalLink" },
	{ "editor/report_a_bug", nullptr, "ExternalLink" },
	{ "editor/suggest_a_feature", nullptr, "ExternalLink" },
	{ "editor/send_docs_feedback", nullptr, "ExternalLink" },
};

void EditorTitleBar::gui_input(const Ref<InputEvent> &p_event) {
	if (!can_move) {
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && moving) {
		if (mm->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
			Window *w = Object::cast_to<Window>(get_viewport());
			if (w) {
				Point2 mouse = DisplayServer::get_singleton()->mouse_get_position();
				w->set_position(mouse - click_pos);
			}
		} else {
			moving = false;
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && has_point(mb->get_position())) {
		Window *w = Object::cast_to<Window>(get_viewport());
		if (w) {
			if (mb->get_button_index() == MouseButton::LEFT) {
				if (mb->is_pressed()) {
					if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_WINDOW_DRAG)) {
						DisplayServer::get_singleton()->window_start_drag(w->get_window_id());
					} else {
						click_pos = DisplayServer::get_singleton()->mouse_get_position() - w->get_position();
						moving = true;
					}
				} else {
					moving = false;
				}
			}
			if (mb->get_button_index() == MouseButton::LEFT && mb->is_double_click() && mb->is_pressed()) {
				if (DisplayServer::get_singleton()->window_maximize_on_title_dbl_click()) {
					if (w->get_mode() == Window::MODE_WINDOWED) {
						w->set_mode(Window::MODE_MAXIMIZED);
					} else if (w->get_mode() == Window::MODE_MAXIMIZED) {
						w->set_mode(Window::MODE_WINDOWED);
					}
				} else if (DisplayServer::get_singleton()->window_minimize_on_title_dbl_click()) {
					w->set_mode(Window::MODE_MINIMIZED);
				}
				moving = false;
			}
		}
	}
}

void EditorTitleBar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_EXIT_TREE: {
			SceneTree::get_singleton()->get_root()->disconnect(SceneStringName(nonclient_window_input), callable_mp(this, &EditorTitleBar::gui_input));
			get_window()->set_nonclient_area(Rect2i());
		} break;
		case NOTIFICATION_ENTER_TREE: {
			SceneTree::get_singleton()->get_root()->connect(SceneStringName(nonclient_window_input), callable_mp(this, &EditorTitleBar::gui_input));
			[[fallthrough]];
		}
		case NOTIFICATION_RESIZED: {
			get_window()->set_nonclient_area(get_global_transform().xform(Rect2i(get_position(), get_size())));
		} break;
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_CHILD_ORDER_CHANGED: {
			// The menus are rebuilt and put back here when their kind changes.
			_queue_update_menu_icons();
		} break;
	}
}

void EditorTitleBar::_queue_update_menu_icons() {
	if (menu_icons_queued) {
		return;
	}
	menu_icons_queued = true;
	callable_mp(this, &EditorTitleBar::update_menu_icons).call_deferred();
}

void EditorTitleBar::_update_menu_icons(PopupMenu *p_menu, bool p_global_menu, bool p_dark_mode, int p_depth) {
	EditorSettings *settings = EditorSettings::get_singleton();
	for (int i = 0; i < p_menu->get_item_count(); i++) {
		if (p_depth > 0 && p_menu->get_item_submenu_node(i)) {
			_update_menu_icons(p_menu->get_item_submenu_node(i), p_global_menu, p_dark_mode, p_depth - 1);
		}

		const Ref<Shortcut> shortcut = p_menu->get_item_shortcut(i);
		const String text = p_menu->get_item_text(i);
		for (const MainMenuIcon &entry : main_menu_icons) {
			const bool found = entry.shortcut ? (shortcut.is_valid() && shortcut == settings->get_shortcut(entry.shortcut)) : (shortcut.is_null() && text == entry.text);
			if (found) {
				p_menu->set_item_icon(i, EditorNode::get_singleton()->get_editor_theme_native_menu_icon(entry.icon, p_global_menu, p_dark_mode));
				break;
			}
		}
	}
}

void EditorTitleBar::update_menu_icons() {
	menu_icons_queued = false;
	if (!is_inside_tree() || !EditorNode::get_singleton() || !EditorSettings::get_singleton()) {
		return;
	}

	const bool dark_mode = DisplayServer::get_singleton()->is_dark_mode_supported() && DisplayServer::get_singleton()->is_dark_mode();
	for (int i = 0; i < get_child_count(); i++) {
		if (MenuBar *menu_bar = Object::cast_to<MenuBar>(get_child(i))) {
			for (int j = 0; j < menu_bar->get_menu_count(); j++) {
				_update_menu_icons(menu_bar->get_menu_popup(j), menu_bar->is_native_menu(), dark_mode, 0);
			}
		} else if (MenuButton *menu_button = Object::cast_to<MenuButton>(get_child(i))) {
			// The compact main menu: the menus are its submenus.
			_update_menu_icons(menu_button->get_popup(), false, dark_mode, 1);
		}
	}
}

void EditorTitleBar::set_can_move_window(bool p_enabled) {
	can_move = p_enabled;
	set_process_input(can_move);
}

bool EditorTitleBar::get_can_move_window() const {
	return can_move;
}
