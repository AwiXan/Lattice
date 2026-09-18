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

#include "scene/gui/panel_container.h"

class Button;
class ConfigFile;
class EditorDocumentView;
class EditorPlugin;
class OptionButton;
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
	// Main screen views live in panes side by side. Only the first one is
	// populated until split view is turned on, and the split container then
	// reveals the second, so a single-pane layout is unaffected.
	HSplitContainer *pane_split = nullptr;
	VBoxContainer *main_screen_vbox = nullptr;
	VBoxContainer *secondary_screen_vbox = nullptr;

	EditorPlugin *selected_plugin = nullptr;

	// The view filling the second pane, owned here. Null whenever the layout is
	// a single pane, which is what keeps that case exactly as it was.
	EditorDocumentView *secondary_view = nullptr;
	// The beginnings of the pane header: it says which document the pane is
	// showing, which a pane that can show something other than the current
	// scene has to, and lets that be changed.
	HBoxContainer *secondary_header = nullptr;
	OptionButton *secondary_document = nullptr;

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

	int _pick_document_for_second_pane() const;
	void _build_secondary_header();
	void _update_secondary_document_list();
	void _secondary_document_selected(int p_index);

	HBoxContainer *button_hb = nullptr;
	Vector<Button *> buttons;
	Vector<EditorPlugin *> editor_table;
	HashMap<String, EditorPlugin *> main_editor_plugins;

	int _get_current_main_editor() const;

protected:
	void _notification(int p_what);

public:
	void set_button_container(HBoxContainer *p_button_hb);

	void save_layout_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const;
	void load_layout_from_config(Ref<ConfigFile> p_config_file, const String &p_section);

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
	VBoxContainer *get_secondary_control() const;

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

	void set_split_view_enabled(bool p_enabled);
	bool is_split_view_enabled() const { return secondary_view != nullptr; }
	bool can_split_view() const;

	void add_main_plugin(EditorPlugin *p_editor);
	void remove_main_plugin(EditorPlugin *p_editor);

	EditorMainScreen();
};
