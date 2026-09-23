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
#include "scene/resources/style_box_flat.h"

class Button;
class Control;
class EditorPaneTree;
class HBoxContainer;
class HFlowContainer;
class MenuButton;
class PanelContainer;
class OptionButton;
class PopupMenu;
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
// Nothing here is special. The 3D view a fresh editor opens on is a panel of a
// registered type like any other, and can be closed, moved or replaced like any
// other - there is no main screen underneath it that has to stay.
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

	// Where a drop at this point would land. The header is not part of it: a
	// tab let go over the tabs joins them, wherever along the bar it was.
	// p_current is the zone being shown already, which the pointer has to
	// leave by a margin before another is chosen: along a boundary the answer
	// would otherwise flicker from one pixel to the next.
	DropZone get_drop_zone_at(const Point2 &p_point, DropZone p_current = DROP_NONE) const;
	// The part of this pane a panel actually occupies, which is everything
	// below the header.
	Rect2 get_body_rect() const;

	// The five targets drawn in the middle of a pane while a panel is dragged
	// over it - into it, or split off to one side - so that where it goes can
	// be aimed at rather than found by feeling for the edges. Pane-local; an
	// empty rectangle when the pane is too small to hold them.
	Rect2 get_compass_target_rect(DropZone p_zone) const;
	DropZone get_compass_zone_at(const Point2 &p_point) const;
	bool is_point_on_header(const Point2 &p_point) const;

	// Whether a drag is offering a panel at all. Answered without asking any
	// particular pane, so that the hint lying over the panes knows whether to
	// get in the way of the drop at all - a node dragged onto a 3D view has to
	// reach that view, as it always did.
	static bool is_panel_drag(const Variant &p_data);
	// The first file in a drag that something registered can show, if any.
	static String first_openable_file(const Variant &p_data);

	// A drop somewhere over this pane, in this pane's coordinates. These are
	// what the hint calls, because during a drag the thing under the mouse is
	// the hint and not what the pane is showing.
	bool can_accept_drop(const Point2 &p_point, const Variant &p_data) const;
	// p_zone is the one the drop hint showed, so the panel lands where it was
	// seen to be going; without it the zone is worked out afresh.
	bool accept_drop(const Point2 &p_point, const Variant &p_data, DropZone p_zone = DROP_NONE);
	// Shows a file here, in whichever kind of panel suits it. Returns false if
	// nothing registered can show that kind of file.
	bool open_resource(const String &p_path);

	TabBar *get_tab_bar() const { return tab_bar; }

private:

	struct PanelEntry {
		StringName type;
		Variant subject;
		Control *control = nullptr;
	};

	// What the header is drawn in, and what the pane measures it by.
	PanelContainer *header_panel = nullptr;
	// It wraps onto a second line rather than holding the pane open at its own
	// width, so a pane can be made as narrow as what it is showing allows.
	HFlowContainer *header = nullptr;
	TabBar *tab_bar = nullptr;
	// A button per kind of view that goes beside a scene: press for one here,
	// drag for one wherever it is let go.
	HBoxContainer *palette = nullptr;
	Vector<StringName> palette_types;
	// Everywhere else the editor can take you - the script editor, the game
	// view, an addon's screen. Places to go rather than things to arrange, so
	// they cost one button between them instead of one each.
	MenuButton *more_button = nullptr;
	Vector<StringName> more_types;
	OptionButton *subject_button = nullptr;
	Button *float_button = nullptr;
	Button *split_right_button = nullptr;
	Button *split_down_button = nullptr;
	Button *close_button = nullptr;
	// Shown while this pane is over all the others, as the way back.
	Button *restore_button = nullptr;
	// Panels closed recently, to have back.
	PopupMenu *recent_menu = nullptr;

	Vector<PanelEntry> panels;
	int current = -1;
	// Rebuilding the tab bar makes it report selections of its own - adding the
	// first tab selects it - which would overwrite the panel actually chosen.
	bool rebuilding_tabs = false;

	void _build_header();
	void _update_theme();
	void _update_palette();
	void _update_tabs();
	void _update_subject_list();
	void _tab_selected(int p_index);
	void _tab_close_pressed(int p_index);
	void _palette_pressed(const StringName &p_type);
	void _more_selected(int p_index);
	Ref<Texture2D> _icon_of(const StringName &p_type) const;
	void _subject_selected(int p_index);
	void _split_pressed(bool p_vertical);
	void _float_pressed();
	void _close_pressed();
	void _restore_pressed();
	void _recent_selected(int p_index);
	void _tab_bar_input(const Ref<InputEvent> &p_event);
	void _note_closing(int p_index);
	// The color of the scene the current panel is showing, when it shows one
	// and colors are shown at all; transparent otherwise.
	Color _scene_color() const;
	void _draw_scene_color();
	void _scene_changed();
	String _title_of(const PanelEntry &p_entry) const;
	void _show_only_current();
	// Stops showing a panel: back to whoever lent it, or freed if this pane
	// built it.
	void _let_go_of(const PanelEntry &p_entry);
	void _return_everything_lent();

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
	// Where between the tabs a panel let go at this point of the bar goes: the
	// gap the bar's own drop mark is drawn in.
	int _tab_insert_index_at(const Point2 &p_in_bar) const;
	Control *_make_drag_preview(int p_index) const;
	StringName _type_for_subject(const Variant &p_subject) const;
	Variant _subject_for_type(const StringName &p_type) const;
	bool _accept_drop(const PanelDrop &p_drop, DropZone p_zone, int p_tab_index);

	Variant _tab_get_drag_data_fw(const Point2 &p_point, Control *p_from);
	bool _tab_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void _tab_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Adds a panel of a registered type, pointed at p_subject - a document's
	// history id, a resource path, or nothing for a panel that shows the same
	// thing whoever holds it - and shows it. Returns its index, or -1.
	int add_panel(const StringName &p_type, const Variant &p_subject = Variant());
	// Drops one. Refuses to drop the editor's main screen, which has to be
	// somewhere.
	void close_panel(int p_index);

	// Brings the tab showing this Control to the front. False if this pane is
	// not the one holding it.
	bool show_panel(Control *p_panel);

	int get_panel_count() const { return panels.size(); }
	int get_current_panel() const { return current; }
	void set_current_panel(int p_index);

	StringName get_panel_type_at(int p_index) const;
	Variant get_panel_subject_at(int p_index) const;
	Control *get_panel_at(int p_index) const;
	String get_panel_title_at(int p_index) const;

	// The panel being shown, for callers that only care about that.
	StringName get_panel_type() const { return get_panel_type_at(current); }
	Variant get_panel_subject() const { return get_panel_subject_at(current); }
	Control *get_panel() const { return get_panel_at(current); }

	// Makes this pane show exactly one panel of the given type.
	void set_panel_type(const StringName &p_type, const Variant &p_subject = Variant());
	// Brings the panel of this type to the front, or adds one if this pane has
	// none. Returns its index, or -1 if nothing could be made.
	int show_panel_of_type(const StringName &p_type);
	// Points the panel being shown at something else.
	void set_panel_subject(const Variant &p_subject);

	// Hands a panel over to another pane, Control and all. Nothing is rebuilt,
	// so what moves keeps its camera, its scroll and what it had selected - the
	// difference between moving a thing and making another one like it.
	bool transfer_panel_to(EditorPane *p_target, int p_index, int p_target_index = -1);

	// Whether this pane offers to be closed. The last one does not.
	void set_closable(bool p_closable);
	// Whether this pane is the one shown over all the others.
	void set_maximized(bool p_maximized);
	bool is_header_visible() const;

	EditorPane();
};

// Where a drag would land, drawn over the panes rather than by them.
//
// A pane cannot do this itself, for two reasons that are really the same one:
// what a pane shows is its child, so a hint the pane drew would be underneath
// it - and a 3D view takes the mouse for itself, so the drop would never be
// offered to the pane at all. Godot stops looking for someone to take a drop at
// the first control that swallows the mouse, and a viewport is one.
//
// So one control lies over the whole arrangement while a drag a pane would
// accept is in the air, finds the pane under the pointer, and answers for it.
class EditorPaneDropHint : public Control {
	GDCLASS(EditorPaneDropHint, Control);

	EditorPaneTree *tree = nullptr;
	// Worked out while answering whether a drop is possible - the only moment
	// the position is known - and read while drawing.
	mutable EditorPane *target = nullptr;
	mutable EditorPane::DropZone zone = EditorPane::DROP_NONE;
	mutable bool on_header = false;
	// Aiming at a target on an edge of the whole arrangement rather than at a
	// pane; `zone` says which edge.
	mutable bool at_edge = false;

	Ref<StyleBoxFlat> landing;
	Ref<StyleBoxFlat> outline;
	Color accent;

	// What is on screen glides to what the pointer is over now, rather than
	// jumping: the eye follows where the panel is going instead of hunting for
	// where the highlight went.
	Rect2 wanted_landing;
	Rect2 wanted_outline;
	real_t wanted_alpha = 0.0;
	Rect2 shown_landing;
	Rect2 shown_outline;
	real_t shown_alpha = 0.0;

	Ref<StyleBoxFlat> target_box;

	EditorPane *_pane_at(const Point2 &p_point) const;
	void _forget();
	void _aim();
	void _draw_target(const Rect2 &p_rect, EditorPane::DropZone p_zone, bool p_edge, bool p_hot);

protected:
	void _notification(int p_what);
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;

public:
	void watch(EditorPaneTree *p_tree) { tree = p_tree; }

	// A drag started in another window, which this one is never told about by
	// the engine: whoever watches that drag feeds the pointer in here, in
	// screen coordinates, so this window can show and take the drop all the
	// same.
	void track_external(const Point2 &p_screen_position, const Variant &p_data);
	bool drop_external(const Point2 &p_screen_position, const Variant &p_data);
	void end_external();

	EditorPaneDropHint();
};
