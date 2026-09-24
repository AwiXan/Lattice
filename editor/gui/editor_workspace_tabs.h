/**************************************************************************/
/*  editor_workspace_tabs.h                                               */
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
class ButtonGroup;
class ConfirmationDialog;
class PopupMenu;

// The workspaces - saved arrangements of panes, panels and windows - as tabs
// in the title bar, after the menus, the way Blender has them: one click to
// another, the one left keeping what was changed in it; "+" to keep the
// arrangement there is as a new one; a right click to keep it in one that
// exists, or to delete one. It only asks: EditorNode does it.
class EditorWorkspaceTabs : public HBoxContainer {
	GDCLASS(EditorWorkspaceTabs, HBoxContainer);

	HBoxContainer *tabs = nullptr;
	Button *add_button = nullptr;
	Ref<ButtonGroup> group;
	PopupMenu *context_menu = nullptr;
	ConfirmationDialog *delete_confirmation = nullptr;
	String context_name;
	Vector<String> names;
	String current;
	bool theming = false;

	void _tab_pressed(const String &p_name);
	void _add_pressed();
	void _tab_input(const Ref<InputEvent> &p_event, const String &p_name);
	void _context_pressed(int p_id);
	void _delete_confirmed();
	void _style_tab(Button *p_tab);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_workspaces(const Vector<String> &p_names, const String &p_current);
	Button *get_tab(const String &p_name) const;
	Button *get_add_button() const { return add_button; }

	EditorWorkspaceTabs();
};
