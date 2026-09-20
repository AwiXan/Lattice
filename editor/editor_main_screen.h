/**************************************************************************/
/*  editor_main_screen.h                                                  */
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

#include "core/templates/hash_set.h"
#include "scene/gui/panel_container.h"

class Button;
class ConfigFile;
class EditorDocumentView;
class EditorPane;
class EditorPaneTree;
class EditorPaneWindow;
class EditorPlugin;
class HBoxContainer;
class HSplitContainer;
class VBoxContainer;

class EditorMainScreen : public PanelContainer {
	GDCLASS(EditorMainScreen, PanelContainer);

public:
	enum EditorTable {
		EDITOR_2D = 0,
		EDITOR_3D,
		EDITOR_SCRIPT,
		EDITOR_GAME,
		EDITOR_ASSETLIB,
	};

private:
	// Panes, arranged by splitting, with no fixed number of them. The first one
	// holds the main screen itself, which plugins parent their views into and
	// addons reach through EditorInterface, so that Control is unchanged and
	// simply lives in a pane like anything else.
	EditorPaneTree *pane_tree = nullptr;
	VBoxContainer *main_screen_vbox = nullptr;

	EditorPlugin *selected_plugin = nullptr;


	// The pane the user last worked in. It is what the Scene tree, the Inspector
	// and the selection follow, by way of the document it is bound to becoming
	// the current one - so clicking into a pane showing another scene brings the
	// rest of the editor with it, and the tab bar says which scene that is.
	EditorDocumentView *active_view = nullptr;
	// Set while this class is the one changing the current document, so that
	// being told about the change does not start over.
	bool changing_context = false;

	// The first pane's view while it is being held to one document, remembered
	// rather than looked up: which plugin is selected can change under it.
	EditorDocumentView *pinned_primary_view = nullptr;

	// Main screens that a pane is showing instead of the main screen itself.
	// There is one of each, so one that is out cannot be lent again - and
	// nothing hides it while it is out.
	HashSet<EditorPlugin *> lent_plugins;

	static StringName _main_panel_type_id(const EditorPlugin *p_editor);
	// Which Control a plugin puts in the main screen, remembered once found.
	HashMap<ObjectID, ObjectID> plugin_controls;
	Control *_control_of(EditorPlugin *p_editor);
	Control *_lend_main_panel(EditorPlugin *p_editor);
	static int _rank_main_panel(const String &p_path, const StringName &p_class, EditorPlugin *p_editor);
	static bool _open_in_main_panel(Control *p_panel, const String &p_path, EditorPlugin *p_editor);
	bool _return_main_panel(Control *p_panel, EditorPlugin *p_editor);

	// Arrangements of panes in windows of their own. The main one is not among
	// them; it is the one that is always there.
	Vector<EditorPaneWindow *> pane_windows;

	void _panes_changed();
	void _restore_panes(const Dictionary &p_layout);
	void _restore_pane_windows(const Array &p_windows);
	// A pane asking for one of its panels to be somewhere else: out of the main
	// window if it is in it, back into it if it is not.
	void _panel_float_requested(EditorPane *p_pane, int p_panel, EditorPaneTree *p_tree);
	void _pane_window_closed(EditorPaneWindow *p_window);
	void _watch_tree(EditorPaneTree *p_tree);
	// Closes a window, bringing whatever it still holds back with it.
	void _close_pane_window(EditorPaneWindow *p_window, bool p_keep_panels);

	// Whether a main screen may be asked for at all. The feature profiles turn
	// them off; there is no button left to hide, so it is written down here.
	Vector<bool> plugin_allowed;
	Vector<EditorPlugin *> editor_table;
	HashMap<String, EditorPlugin *> main_editor_plugins;

	int _get_current_main_editor() const;

protected:
	void _notification(int p_what);

public:
	void save_layout_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const;
	void load_layout_from_config(Ref<ConfigFile> p_config_file, const String &p_section);

	// Whether this main screen may be asked for. Kept for the feature profiles,
	// which turn 3D, scripting, the game view or the asset store off.
	void set_button_enabled(int p_index, bool p_enabled);
	bool is_button_enabled(int p_index) const;

	void select_next();
	void select_prev();
	void select_by_name(const String &p_name);
	void select(int p_index);
	int get_selected_index() const;
	int get_plugin_index(EditorPlugin *p_editor) const;
	EditorPlugin *get_selected_plugin() const;
	EditorPlugin *get_plugin_by_name(const String &p_plugin_name) const;
	bool can_auto_switch_screens() const;

	// The container main screen plugins parent their view into. This is the
	// first pane; addons keep reaching it through EditorInterface unchanged.
	VBoxContainer *get_control() const;
	EditorPaneTree *get_pane_tree() const { return pane_tree; }

	// Sends a panel to a window of its own, and hands back the window it went
	// to. Null if the editor cannot open windows at all.
	EditorPaneWindow *open_panel_in_window(EditorPane *p_from, int p_panel);
	int get_pane_window_count() const { return pane_windows.size(); }

	// Splitting shows a second view of the plugin currently selected, pointed
	// at another open scene, so two documents are edited side by side. Plugins
	// that cannot be built twice refuse, and the layout stays single-pane.
	// Called by a view when the user works in it. The document it is bound to
	// becomes the current one, which is what makes the docks and the selection
	// follow the pane rather than the tab bar alone.
	void view_activated(EditorDocumentView *p_view);
	// Called when the current document changes by any other route, the tab bar
	// above all: the pane being worked in follows it.
	void current_document_changed();

	// Splitting puts a second pane beside the first. Which panel it shows and
	// what that panel is pointed at are the pane's own business from then on.
	void set_split_view_enabled(bool p_enabled);
	// Another pane beside or below the one holding the main screen, showing the
	// same kind of view on the same scene. A pane with a header of its own can
	// be split from there; this is for the first one, which has none.
	void split_main_pane(bool p_vertical);
	bool is_split_view_enabled() const;

	void add_main_plugin(EditorPlugin *p_editor);
	void remove_main_plugin(EditorPlugin *p_editor);

	EditorMainScreen();
};
