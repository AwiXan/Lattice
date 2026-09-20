/**************************************************************************/
/*  editor_dock_manager.h                                                 */
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

#include "editor/docks/editor_dock.h"
#include "scene/gui/popup.h"
#include "scene/gui/split_container.h"

class Button;
class ConfigFile;
class Control;
class EditorDock;
class PopupMenu;
class TabBar;
class TabContainer;
class VBoxContainer;
class WindowWrapper;
class StyleBoxFlat;

class DockSplitContainer : public SplitContainer {
	GDCLASS(DockSplitContainer, SplitContainer);

private:
	bool is_updating = false;

protected:
	void _notification(int p_what);
	void _update_visibility();

	virtual void add_child_notify(Node *p_child) override;
	virtual void remove_child_notify(Node *p_child) override;

public:
	Control *get_child_as_control(int p_index) const;

	DockSplitContainer();
};

class DockShortcutHandler : public Node {
	GDCLASS(DockShortcutHandler, Node);

protected:
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	DockShortcutHandler() { set_process_shortcut_input(true); }
};

class DockContextPopup;
class EditorDockDragHint;
class DockTabContainer;

class EditorDockManager : public Object {
	GDCLASS(EditorDockManager, Object);

private:
	friend class DockContextPopup;
	friend class EditorDockDragHint;
	friend class DockShortcutHandler;

	static inline EditorDockManager *singleton = nullptr;

	// To access splits easily by index.
	Vector<DockSplitContainer *> vsplits;
	DockSplitContainer *main_vsplit = nullptr;
	DockSplitContainer *main_hsplit = nullptr;
	DockSplitContainer *bottom_hsplit = nullptr;

	DockTabContainer *dock_slots[EditorDock::DOCK_SLOT_MAX];
	Vector<WindowWrapper *> dock_windows;
	LocalVector<EditorDock *> all_docks;
	HashSet<EditorDock *> dirty_docks;

	EditorDock *dock_tab_dragged = nullptr;
	bool docks_visible = true;

	// Docks a pane is showing instead of a dock slot. The editor has one of
	// each, so a dock that is out on loan cannot be lent again.
	HashSet<EditorDock *> lent_docks;

	DockContextPopup *dock_context_popup = nullptr;
	PopupMenu *docks_menu = nullptr;
	LocalVector<EditorDock *> docks_menu_docks;
	Control *closed_dock_parent = nullptr;

	EditorDock *_get_dock_tab_dragged();
	void _dock_drag_stopped();
	void _dock_split_dragged(int p_offset);
	void _update_layout();

	void _docks_menu_option(int p_id);

	void _window_close_request(WindowWrapper *p_wrapper);
	EditorDock *_close_window(WindowWrapper *p_wrapper);
	void _open_dock_in_window(EditorDock *p_dock, bool p_show_window = true, bool p_reset_size = false);
	void _restore_dock_to_saved_window(EditorDock *p_dock, const Dictionary &p_window_dump);

	void _make_dock_visible(EditorDock *p_dock, bool p_grab_focus);
	void _move_dock(EditorDock *p_dock, Control *p_target, int p_tab_index = -1, bool p_set_current = true);

	void _queue_update_tab_style(EditorDock *p_dock);
	void _update_dirty_dock_tabs();

	// A dock as a panel: taken out of the arrangement of slots for as long as
	// something else shows it, and put back when that thing has done with it.
	// Registered for every dock, which is what lets a pane, a saved layout or a
	// window ask for one without knowing this class exists.
	Control *_lend_dock_panel(EditorDock *p_dock);
	void _return_dock_panel(Control *p_panel, EditorDock *p_dock);

public:
	static EditorDockManager *get_singleton() { return singleton; }

	void update_docks_menu();
	void update_tab_styles();
	void set_tab_icon_max_width(int p_max_width);

	void add_vsplit(DockSplitContainer *p_split);
	void set_main_vsplit(DockSplitContainer *p_split);
	void set_main_hsplit(DockSplitContainer *p_split);
	void set_bottom_hsplit(DockSplitContainer *p_split);
	void register_dock_slot(DockTabContainer *p_tab_container);
	int get_vsplit_count() const;
	PopupMenu *get_docks_menu();

	void save_docks_to_config(Ref<ConfigFile> p_layout, const String &p_section) const;
	void load_docks_from_config(Ref<ConfigFile> p_layout, const String &p_section, bool p_first_load = false);

	void set_dock_enabled(EditorDock *p_dock, bool p_enabled);
	void close_dock(EditorDock *p_dock);
	void open_dock(EditorDock *p_dock, bool p_set_current = true);
	void focus_dock(EditorDock *p_dock);
	void make_dock_floating(EditorDock *p_dock);

	void set_docks_visible(bool p_show);
	bool are_docks_visible() const;

	void add_dock(EditorDock *p_dock);
	void remove_dock(EditorDock *p_dock);

	// Whether this dock is somewhere that is not one of the slots, which is why
	// the slots no longer show it.
	bool is_dock_lent(EditorDock *p_dock) const { return lent_docks.has(p_dock); }
	// What this dock is called in the panel registry, and therefore in a saved
	// layout and in what a dock tab carries while it is being dragged.
	static StringName get_dock_panel_type_id(const EditorDock *p_dock);

	EditorDockManager();
};

class DockContextPopup : public PopupPanel {
	GDCLASS(DockContextPopup, PopupPanel);

private:
	Button *make_float_button = nullptr;
	Button *close_button = nullptr;

	EditorDock *context_dock = nullptr;
	EditorDockManager *dock_manager = nullptr;

	void _close_dock();
	void _float_dock();

	void _update_buttons();

protected:
	void _notification(int p_what);

public:
	void set_dock(EditorDock *p_dock);
	void docks_updated();

	DockContextPopup();
};
