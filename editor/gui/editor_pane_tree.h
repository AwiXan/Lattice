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

#include "scene/gui/container.h"

class EditorPane;
class EditorPaneDropHint;

// Panes arranged by splitting, with no fixed number of them.
//
// Every pane is a direct child of this container. What is nested is not the
// panes but a description of where they go: a tree of rectangles, each holding
// either one pane or two of them sharing the space. Splitting adds a child and
// changes the description; it never moves a pane that is already here.
//
// That is the whole point of doing it this way. Nesting the panes themselves -
// a pane inside a SplitContainer inside a SplitContainer - reads better, and is
// what this used to do, but then every split had to take a pane out of the
// scene tree and put it back one level down. The pane holding the editor's main
// screen is some three thousand nodes, every one of them leaving and re-entering
// the tree, re-reading its theme and reshaping its text: two hundred
// milliseconds of stutter whenever anyone split anything, with the viewports
// visibly starting over. A split is a rectangle being divided now.
class EditorPaneTree : public Container {
	GDCLASS(EditorPaneTree, Container);

	// A leaf is one pane. A branch is two of them sharing a rectangle, split
	// across or down, the first taking `ratio` of what there is to share.
	struct Slot {
		EditorPane *pane = nullptr;
		bool vertical = false;
		real_t ratio = 0.5;
		Slot *first = nullptr;
		Slot *second = nullptr;
		Slot *parent = nullptr;
		// Where it ended up when the panes were last laid out, which is what
		// the dividers are drawn and grabbed by.
		Rect2 rect;

		bool is_leaf() const { return pane != nullptr; }
	};

	Slot *root = nullptr;
	// Lies over all of them while a panel is being dragged. See
	// EditorPaneDropHint: a pane cannot show where a drop would land, because
	// what it shows is drawn on top of it and takes the mouse besides.
	EditorPaneDropHint *drop_hint = nullptr;

	// The divider being dragged, where along it the grab started, and what the
	// ratio was then - so a drag is one sum rather than a running total.
	Slot *dragging = nullptr;
	real_t drag_start = 0;
	real_t drag_start_ratio = 0.5;
	Slot *hovered = nullptr;

	struct ThemeCache {
		int separation = 0;
		int grab_thickness = 0;
		Color divider_color;
		Color divider_hover_color;
	} theme_cache;

	void _wire_pane(EditorPane *p_pane);
	void _pane_split_requested(bool p_vertical, EditorPane *p_pane);

	Slot *_leaf_for(const EditorPane *p_pane) const;
	Slot *_find_leaf(Slot *p_slot, const EditorPane *p_pane) const;
	void _free_slot(Slot *p_slot);
	void _collect(Slot *p_slot, Vector<EditorPane *> &r_panes) const;
	Size2 _min_size_of(const Slot *p_slot) const;
	void _lay_out(Slot *p_slot, const Rect2 &p_rect);
	Rect2 _divider_rect(const Slot *p_slot) const;
	Slot *_find_divider(Slot *p_slot, const Point2 &p_point) const;
	void _draw_dividers(const Slot *p_slot);
	void _update_theme();

	Dictionary _save_slot(const Slot *p_slot) const;
	Slot *_load_slot(const Dictionary &p_data, Slot *p_parent);
	// Between what a binding means while running and what it means on disk.
	static Variant _subject_to_saved(const StringName &p_type, const Variant &p_subject);
	static Variant _subject_from_saved(const StringName &p_type, const Variant &p_saved);
	// Which pane a layout being loaded says holds the editor's main screen, so
	// that it can be given back once the old arrangement has let go of it.
	EditorPane *pending_main_screen_host = nullptr;
	void _update_closable();

protected:
	void _notification(int p_what);
	static void _bind_methods();

	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	virtual Size2 get_minimum_size() const override;
	virtual CursorShape get_cursor_shape(const Point2 &p_pos = Point2i()) const override;

	// The pane every other one is split off from. It holds the editor's main
	// screen and is never closed.
	EditorPane *get_first_pane() const;
	// The one actually holding the main screen, wherever it has ended up.
	EditorPane *get_main_screen_pane() const;
	Vector<EditorPane *> get_panes() const;

	// Splits a pane in two, side by side or one above the other, and returns the
	// pane that appeared. p_before puts the new pane first, which is what a drop
	// on the left or top edge means. Filled, it starts on the same panel as the
	// pane it was split from, so splitting is a way to compare rather than a way
	// to lose your place; empty, it is about to be given something.
	EditorPane *split_pane(EditorPane *p_pane, bool p_vertical, bool p_before = false, bool p_fill = true);

	// Splits p_target and moves a panel of p_source into the pane that appears.
	// This is what dropping a tab on the edge of a pane does.
	EditorPane *split_with_panel(EditorPane *p_target, bool p_vertical, bool p_before, EditorPane *p_source, int p_panel_index);

	// The same, for a drop that brought a description rather than a panel - a
	// scene tab, a file - so the new pane is given a panel built for it.
	EditorPane *split_with_new_panel(EditorPane *p_target, bool p_vertical, bool p_before, const StringName &p_type, const Variant &p_subject);

	// Closes any pane left holding nothing. A pane that has just given its last
	// panel away has no reason to take up room.
	void drop_empty_panes();
	// Closes a pane; its sibling takes the space back. Refuses to close the last.
	void close_pane(EditorPane *p_pane);

	// The arrangement as data: panel types, what each is pointed at, and the
	// shape of the splits. No class names, so it survives anything but a type
	// being unregistered.
	//
	// What a panel is pointed at is written down in a form that outlives the
	// session. A document's history id is only meaningful while the editor is
	// running, so what is saved is the scene's path, and what is read back is
	// the id that scene has this time round. Saving for the session itself -
	// switching between arrangements without closing the editor - is the same
	// call: the path resolves either way.
	Dictionary save_layout() const;
	void load_layout(const Dictionary &p_layout);

	// Takes the Control the editor's main screen is, to show in the first pane.
	void adopt_main_screen(Control *p_main_screen, const String &p_title);

	EditorPaneTree();
	~EditorPaneTree();
};
