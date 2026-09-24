/**************************************************************************/
/*  editor_script_panel.h                                                 */
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

#include "scene/gui/box_container.h"

class Button;
class FindReplaceBar;
class Label;

// What the script editor keeps in a tab whose script a panel is showing.
//
// The script is still one of the scripts open there - listed, saved with the
// rest, asked about on quitting - so something has to hold its place. This
// is it: the editor's own code goes through it to the editor wherever that
// is, and anyone looking at the tab here is told where, with a way there.
class ScriptEditorStandIn : public VBoxContainer {
	GDCLASS(ScriptEditorStandIn, VBoxContainer);

	ObjectID editor;
	ObjectID panel;
	Label *message = nullptr;

	void _show_pressed();
	void _bring_pressed();
	static void _bring_back(ObjectID p_panel, const String &p_path);

public:
	Control *get_editor() const;
	Control *get_panel() const;
	// What "Bring It Here" does: the script back from its panel, and shown.
	void bring_here();

	ScriptEditorStandIn(Control *p_editor = nullptr, Control *p_panel = nullptr);
};

// A script in a panel of its own, which can be put in any pane or window and
// stacked with others as tabs.
//
// The script editor is still what edits it, the way the Scene dock serves any
// number of trees: this borrows the editor of one script - its Edit, Search
// and Go To menus with it, and a find bar of its own - and working in it makes
// it the script the rest of the script editor acts on. Closing it closes the
// script, asking first if there are changes not saved; a panel going away for
// any other reason - a workspace being switched - only gives the script back.
class EditorScriptPanel : public VBoxContainer {
	GDCLASS(EditorScriptPanel, VBoxContainer);

	HBoxContainer *menu_bar = nullptr;
	FindReplaceBar *find_bar = nullptr;
	Label *note = nullptr;
	ObjectID editor;
	String path;
	bool closing_by_user = false;

	void _give_back(bool p_close);
	void _editor_name_changed();
	void _focus_changed(Control *p_control);
	void _set_note(const String &p_text);

protected:
	void _notification(int p_what);
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	void set_script_path(const String &p_path);
	String get_script_path() const { return path; }
	Control *get_editor() const;
	String get_title() const;
	// Says the tab again: the script editor's list of names changed.
	void refresh_title();

	// What the panel type registers.
	static Control *create_panel();
	static void bind_panel(Control *p_panel, const Variant &p_subject);
	static int rank_resource(const String &p_path, const StringName &p_class);
	static String title_of(Control *p_panel);
	static void closed_by_user(Control *p_panel);

	EditorScriptPanel();
};
