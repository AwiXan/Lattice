/**************************************************************************/
/*  editor_pane_tree.h                                                    */
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

#include "scene/gui/margin_container.h"

class EditorPane;
class SplitContainer;

// Panes arranged by splitting, with no fixed number of them.
//
// The arrangement *is* the Control hierarchy: a leaf is an EditorPane, a branch
// is a SplitContainer with two children. Nothing is modelled twice, so saving a
// layout is a walk of that hierarchy and restoring one is the same walk in
// reverse. A torn-off window holds one of these as well, which is what makes a
// window no different from a pane.
class EditorPaneTree : public MarginContainer {
	GDCLASS(EditorPaneTree, MarginContainer);

	// The single Control under this one: a pane, or a split of panes.
	Control *root = nullptr;

	void _collapse_split(SplitContainer *p_split, Control *p_survivor);
	Dictionary _save_node(Control *p_node) const;
	Control *_load_node(const Dictionary &p_data);
	// Which pane a layout being loaded says holds the editor's main screen, so
	// that it can be given back once the old arrangement has let go of it.
	EditorPane *pending_main_screen_host = nullptr;
	void _collect_panes(Control *p_node, Vector<EditorPane *> &r_panes) const;
	void _update_closable();

protected:
	static void _bind_methods();

public:
	// The pane every other one is split off from. It holds the editor's main
	// screen and is never closed.
	EditorPane *get_first_pane() const;
	Vector<EditorPane *> get_panes() const;

	// Splits a pane in two, side by side or one above the other, and returns the
	// pane that appeared. The new pane starts on the same panel as the one it
	// was split from, which is what makes splitting a way to compare rather than
	// a way to lose your place.
	EditorPane *split_pane(EditorPane *p_pane, bool p_vertical);
	// Closes a pane; its sibling takes the space back. Refuses to close the last.
	void close_pane(EditorPane *p_pane);

	// The arrangement as data: panel types, what each is pointed at, and the
	// shape of the splits. No class names, so it survives anything but a type
	// being unregistered.
	Dictionary save_layout() const;
	void load_layout(const Dictionary &p_layout);

	// Takes the Control the editor's main screen is, to show in the first pane.
	void adopt_main_screen(Control *p_main_screen, const String &p_title);

	EditorPaneTree();
};
