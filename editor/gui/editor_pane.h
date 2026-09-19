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
class EditorPaneTree;
class OptionButton;
class TabBar;

// One place panels sit: a leaf of the pane tree, holding any number of them as
// tabs with one shown at a time.
//
// A pane knows a panel only as a registered type and what that panel is pointed
// at. It never names a class, which is what lets a saved layout say "a 3D view
// on the scene at this path, here" and be restored without anything knowing
// which classes exist - and what will let a tab be torn into a window of its
// own, since a window need only be told the same two things.
//
// A pane may instead adopt a Control built elsewhere. The editor's main screen
// is one: plugins parent their views into it and reach it through
// EditorInterface, so it stays the one Control it has always been and simply
// lives in a pane like everything else.
class EditorPane : public VBoxContainer {
	GDCLASS(EditorPane, VBoxContainer);

public:
	// Where a tab being dragged would land if it were let go at a point. Which
	// one it is decides between moving the panel into this pane and splitting
	// this pane to make room for it.
	enum DropZone {
		DROP_NONE,
		DROP_INTO,
		DROP_LEFT,
		DROP_RIGHT,
		DROP_TOP,
		DROP_BOTTOM,
	};

	DropZone get_drop_zone_at(const Point2 &p_point) const;

private:

	struct PanelEntry {
		StringName type;
		Variant subject;
		Control *control = nullptr;
		// The editor's main screen, shown here rather than built from a type.
		bool adopted = false;
		String title;
	};

	HBoxContainer *header = nullptr;
	TabBar *tab_bar = nullptr;
	OptionButton *add_button = nullptr;
	OptionButton *subject_button = nullptr;
	Button *split_right_button = nullptr;
	Button *split_down_button = nullptr;
	Button *close_button = nullptr;

	Vector<PanelEntry> panels;
	int current = -1;
	// Rebuilding the tab bar makes it report selections of its own - adding the
	// first tab selects it - which would overwrite the panel actually chosen.
	bool rebuilding_tabs = false;

	// Written while answering whether a drop is possible, which is the only
	// moment the position is known, and read while drawing the hint.
	mutable DropZone drop_zone = DROP_NONE;

	void _build_header();
	void _update_tabs();
	void _update_add_list();
	void _update_subject_list();
	void _tab_selected(int p_index);
	void _tab_close_pressed(int p_index);
	void _add_selected(int p_index);
	void _subject_selected(int p_index);
	void _split_pressed(bool p_vertical);
	void _close_pressed();
	String _title_of(const PanelEntry &p_entry) const;
	void _show_only_current();

	EditorPaneTree *_get_pane_tree() const;

	// What a drag is offering a pane.
	//
	// Either a panel that already exists somewhere - moved rather than made
	// again, so it keeps its camera, its scroll and what it had selected - or a
	// description of one to build. Anything in the editor can offer the second
	// kind by putting two keys in its drag data:
	//
	//   "editor_panel"         the registered type to build, which may be left
	//                          out to mean "whatever suits what it is pointed at"
	//   "editor_panel_subject" what that panel should show: a document's history
	//                          id, or a resource's path
	//
	// Every other key is none of a pane's business, which is what lets a scene
	// tab's drag be a tab bar's own drag as well, so dropping it back on the bar
	// still reorders the tabs.
	struct PanelDrop {
		EditorPane *source = nullptr;
		int source_index = -1;
		StringName type;
		Variant subject;
		bool is_valid() const { return source || type != StringName(); }
	};
	PanelDrop _read_drop(const Variant &p_data) const;
	StringName _type_for_subject(const Variant &p_subject) const;
	bool _accept_drop(const PanelDrop &p_drop, DropZone p_zone, int p_tab_index);

	Variant _tab_get_drag_data_fw(const Point2 &p_point, Control *p_from);
	bool _tab_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void _tab_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);

protected:
	void _notification(int p_what);
	static void _bind_methods();

	// A tab dropped on the body of a pane: into it, or beside it if let go near
	// an edge. The tab bar has its own, which inserts at a position instead.
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;

public:
	// Adds a panel of a registered type, pointed at p_subject - a document's
	// history id, a resource path, or nothing for a panel that shows the same
	// thing whoever holds it - and shows it. Returns its index, or -1.
	int add_panel(const StringName &p_type, const Variant &p_subject = Variant());
	// Drops one. Refuses to drop the editor's main screen, which has to be
	// somewhere.
	void close_panel(int p_index);

	int get_panel_count() const { return panels.size(); }
	int get_current_panel() const { return current; }
	void set_current_panel(int p_index);

	StringName get_panel_type_at(int p_index) const;
	Variant get_panel_subject_at(int p_index) const;
	Control *get_panel_at(int p_index) const;
	bool is_panel_adopted_at(int p_index) const;
	String get_panel_title_at(int p_index) const;

	// The panel being shown, for callers that only care about that.
	StringName get_panel_type() const { return get_panel_type_at(current); }
	Variant get_panel_subject() const { return get_panel_subject_at(current); }
	Control *get_panel() const { return get_panel_at(current); }

	// Makes this pane show exactly one panel of the given type.
	void set_panel_type(const StringName &p_type, const Variant &p_subject = Variant());
	// Points the panel being shown at something else.
	void set_panel_subject(const Variant &p_subject);

	// Shows a Control built elsewhere - the editor's main screen. It is freed
	// with this pane like any child, but it cannot be closed.
	void adopt_panel(Control *p_panel, const String &p_title);
	bool is_adopting() const;
	String get_adopted_title() const;
	// Hands the adopted Control back, so that replacing the whole arrangement
	// does not destroy the editor's main screen along with it.
	Control *release_adopted_panel();

	// Hands a panel over to another pane, Control and all. Nothing is rebuilt,
	// so what moves keeps its camera, its scroll and what it had selected - the
	// difference between moving a thing and making another one like it.
	bool transfer_panel_to(EditorPane *p_target, int p_index, int p_target_index = -1);

	// Whether this pane offers to be closed. The last one does not.
	void set_closable(bool p_closable);
	// With one pane holding one panel there is nothing to choose between, so a
	// header would be a row of buttons above an editor that has never had one.
	void set_header_visible(bool p_visible);

	EditorPane();
};
