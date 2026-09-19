/**************************************************************************/
/*  editor_pane.h                                                         */
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

#include "editor/editor_panel_registry.h"
#include "scene/gui/box_container.h"

class Button;
class Control;
class OptionButton;

// One place a panel sits: a leaf of the pane tree.
//
// A pane knows its panel only as a registered type and what that panel is
// pointed at. It never names a class, which is what lets a saved layout say
// "a 3D view on the scene at this path, here" and be restored without anything
// knowing which classes exist.
//
// A pane may instead adopt a Control built elsewhere. The editor's main screen
// is one: plugins parent their views into it and reach it through
// EditorInterface, so it stays the one Control it has always been and simply
// lives in a pane like everything else.
class EditorPane : public VBoxContainer {
	GDCLASS(EditorPane, VBoxContainer);

	HBoxContainer *header = nullptr;
	OptionButton *type_button = nullptr;
	OptionButton *subject_button = nullptr;
	Button *split_right_button = nullptr;
	Button *split_down_button = nullptr;
	Button *close_button = nullptr;

	StringName panel_type;
	Variant panel_subject;
	Control *panel = nullptr;
	// An adopted panel is the editor's main screen, shown here. It is freed with
	// this pane like any child, but it cannot be swapped for another panel and
	// its pane cannot be closed: the main screen has to be somewhere.
	bool panel_adopted = false;
	String adopted_title;

	void _build_header();
	void _update_type_list();
	void _update_subject_list();
	void _type_selected(int p_index);
	void _subject_selected(int p_index);
	void _split_pressed(bool p_vertical);
	void _close_pressed();

protected:
	static void _bind_methods();

public:
	// Shows a panel of a registered type, pointed at p_subject - a document's
	// history id, a resource path, or nothing for a panel that shows the same
	// thing whoever holds it.
	void set_panel_type(const StringName &p_type, const Variant &p_subject = Variant());
	StringName get_panel_type() const { return panel_type; }
	Variant get_panel_subject() const { return panel_subject; }
	Control *get_panel() const { return panel; }

	// Shows a Control built elsewhere - the editor's main screen. The pane holds
	// it like any other child and it is freed with the editor, but it cannot be
	// swapped for another panel and its pane cannot be closed: the main screen
	// has to be somewhere.
	void adopt_panel(Control *p_panel, const String &p_title);
	bool is_adopting() const { return panel_adopted; }
	String get_adopted_title() const { return adopted_title; }
	// Hands the adopted Control back, so that replacing the whole arrangement
	// does not destroy the editor's main screen along with it.
	Control *release_adopted_panel();

	// Whether this pane offers to be closed. The last one does not.
	void set_closable(bool p_closable);
	// With one pane there is nothing to choose between, so its header would be a
	// row of buttons above an editor that has always had none.
	void set_header_visible(bool p_visible);

	EditorPane();
};
