/**************************************************************************/
/*  editor_pane_tree.cpp                                                  */
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

#include "editor_pane_tree.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/editor_string_names.h"
#include "editor/editor_main_screen.h"
#include "editor/gui/editor_pane.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/main/viewport.h"

void EditorPaneTree::_bind_methods() {
	ADD_SIGNAL(MethodInfo("layout_changed"));
	// A pane asking for one of its panels to be somewhere else. The arrangement
	// does not know what a window is; whoever holds it does.
	ADD_SIGNAL(MethodInfo("panel_float_requested", PropertyInfo(Variant::OBJECT, "pane"), PropertyInfo(Variant::INT, "panel")));
	ADD_SIGNAL(MethodInfo("panel_return_requested", PropertyInfo(Variant::OBJECT, "pane"), PropertyInfo(Variant::INT, "panel")));
}

// ---------------------------------------------------------------- the slots

EditorPaneTree::Slot *EditorPaneTree::_find_leaf(Slot *p_slot, const EditorPane *p_pane) const {
	if (!p_slot) {
		return nullptr;
	}
	if (p_slot->is_leaf()) {
		return p_slot->pane == p_pane ? p_slot : nullptr;
	}
	Slot *found = _find_leaf(p_slot->first, p_pane);
	return found ? found : _find_leaf(p_slot->second, p_pane);
}

EditorPaneTree::Slot *EditorPaneTree::_leaf_for(const EditorPane *p_pane) const {
	return _find_leaf(root, p_pane);
}

void EditorPaneTree::_free_slot(Slot *p_slot) {
	if (!p_slot) {
		return;
	}
	_free_slot(p_slot->first);
	_free_slot(p_slot->second);
	memdelete(p_slot);
}

void EditorPaneTree::_collect(Slot *p_slot, Vector<EditorPane *> &r_panes) const {
	if (!p_slot) {
		return;
	}
	if (p_slot->is_leaf()) {
		r_panes.push_back(p_slot->pane);
		return;
	}
	_collect(p_slot->first, r_panes);
	_collect(p_slot->second, r_panes);
}

Vector<EditorPane *> EditorPaneTree::get_panes() const {
	Vector<EditorPane *> panes;
	_collect(root, panes);
	return panes;
}

EditorPane *EditorPaneTree::get_first_pane() const {
	Vector<EditorPane *> panes = get_panes();
	return panes.is_empty() ? nullptr : panes[0];
}

EditorPane *EditorPaneTree::get_active_pane() const {
	if (active_pane && _find_leaf(root, active_pane)) {
		return active_pane;
	}
	return get_first_pane();
}

void EditorPaneTree::set_active_pane(EditorPane *p_pane) {
	active_pane = p_pane;
}

// --------------------------------------------------------------- the layout

Size2 EditorPaneTree::_min_size_of(const Slot *p_slot) const {
	if (!p_slot) {
		return Size2();
	}
	if (p_slot->is_leaf()) {
		return p_slot->pane->get_combined_minimum_size();
	}
	const Size2 a = _min_size_of(p_slot->first);
	const Size2 b = _min_size_of(p_slot->second);
	if (p_slot->vertical) {
		return Size2(MAX(a.x, b.x), a.y + b.y + theme_cache.separation);
	}
	return Size2(a.x + b.x + theme_cache.separation, MAX(a.y, b.y));
}

Size2 EditorPaneTree::get_minimum_size() const {
	return _min_size_of(root);
}

void EditorPaneTree::_lay_out(Slot *p_slot, const Rect2 &p_rect) {
	if (!p_slot) {
		return;
	}
	p_slot->rect = p_rect;

	if (p_slot->is_leaf()) {
		fit_child_in_rect(p_slot->pane, p_rect);
		return;
	}

	const real_t along = p_slot->vertical ? p_rect.size.y : p_rect.size.x;
	const real_t bar = MIN((real_t)theme_cache.separation, MAX((real_t)0, along));
	const real_t usable = MAX((real_t)0, along - bar);

	// Neither side is squeezed out of existence, but if there is not room for
	// both of them there is nothing to be done about it and they share equally.
	const Size2 min_first = _min_size_of(p_slot->first);
	const Size2 min_second = _min_size_of(p_slot->second);
	const real_t low = MIN(p_slot->vertical ? min_first.y : min_first.x, usable);
	const real_t high = MAX(low, usable - (p_slot->vertical ? min_second.y : min_second.x));
	const real_t first_size = CLAMP(usable * p_slot->ratio, low, high);

	Rect2 a = p_rect;
	Rect2 b = p_rect;
	if (p_slot->vertical) {
		a.size.y = first_size;
		b.position.y = p_rect.position.y + first_size + bar;
		b.size.y = MAX((real_t)0, p_rect.size.y - first_size - bar);
	} else {
		a.size.x = first_size;
		b.position.x = p_rect.position.x + first_size + bar;
		b.size.x = MAX((real_t)0, p_rect.size.x - first_size - bar);
	}
	_lay_out(p_slot->first, a);
	_lay_out(p_slot->second, b);
}

Rect2 EditorPaneTree::_divider_rect(const Slot *p_slot) const {
	if (!p_slot || p_slot->is_leaf() || !p_slot->first) {
		return Rect2();
	}
	// The gap the layout left between the two, widened to something a mouse can
	// find without care.
	const real_t thickness = MAX(theme_cache.separation, theme_cache.grab_thickness);
	Rect2 r = p_slot->rect;
	if (p_slot->vertical) {
		const real_t edge = p_slot->first->rect.position.y + p_slot->first->rect.size.y;
		r.position.y = edge - (thickness - theme_cache.separation) * 0.5;
		r.size.y = thickness;
	} else {
		const real_t edge = p_slot->first->rect.position.x + p_slot->first->rect.size.x;
		r.position.x = edge - (thickness - theme_cache.separation) * 0.5;
		r.size.x = thickness;
	}
	return r;
}

EditorPaneTree::Slot *EditorPaneTree::_find_divider(Slot *p_slot, const Point2 &p_point) const {
	if (maximized.is_valid()) {
		// There are no dividers on screen to grab.
		return nullptr;
	}
	if (!p_slot || p_slot->is_leaf()) {
		return nullptr;
	}
	if (_divider_rect(p_slot).has_point(p_point)) {
		return p_slot;
	}
	Slot *found = _find_divider(p_slot->first, p_point);
	return found ? found : _find_divider(p_slot->second, p_point);
}

void EditorPaneTree::_draw_dividers(const Slot *p_slot) {
	if (!p_slot || p_slot->is_leaf()) {
		return;
	}
	const bool lit = p_slot == dragging || (!dragging && p_slot == hovered);
	// The gap itself, not the widened grab area: a divider that looked as thick
	// as it is easy to grab would be a bar across the editor.
	Rect2 gap = p_slot->rect;
	if (p_slot->vertical) {
		gap.position.y = p_slot->first->rect.position.y + p_slot->first->rect.size.y;
		gap.size.y = theme_cache.separation;
	} else {
		gap.position.x = p_slot->first->rect.position.x + p_slot->first->rect.size.x;
		gap.size.x = theme_cache.separation;
	}
	draw_rect(gap, lit ? theme_cache.divider_hover_color : theme_cache.divider_color);

	_draw_dividers(p_slot->first);
	_draw_dividers(p_slot->second);
}

void EditorPaneTree::_update_theme() {
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	theme_cache.separation = Math::round(4 * EDSCALE);
	theme_cache.grab_thickness = Math::round(10 * EDSCALE);
	if (base) {
		theme_cache.divider_hover_color = base->get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		theme_cache.divider_color = theme_cache.divider_hover_color * Color(1, 1, 1, 0.12);
	} else {
		theme_cache.divider_hover_color = Color(0.5, 0.7, 1.0);
		theme_cache.divider_color = Color(0, 0, 0, 0.2);
	}
	update_minimum_size();
	queue_sort();
	queue_redraw();
}

void EditorPaneTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;

		case NOTIFICATION_SORT_CHILDREN: {
			_lay_out(root, Rect2(Point2(), get_size()));
			EditorPane *maximized_pane = get_maximized_pane();
			if (maximized_pane) {
				// Everything else keeps its place, hidden, for when this is
				// asked for again.
				fit_child_in_rect(maximized_pane, Rect2(Point2(), get_size()));
			}
			// Whatever is not a pane - the drop hint - covers everything, which
			// is what it is for.
			for (int i = 0; i < get_child_count(); i++) {
				Control *child = Object::cast_to<Control>(get_child(i));
				if (child && !Object::cast_to<EditorPane>(child)) {
					fit_child_in_rect(child, Rect2(Point2(), get_size()));
				}
			}
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			if (!get_maximized_pane()) {
				_draw_dividers(root);
			}
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			if (hovered) {
				hovered = nullptr;
				queue_redraw();
			}
		} break;
	}
}

void EditorPaneTree::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			Slot *divider = _find_divider(root, mb->get_position());
			if (divider) {
				dragging = divider;
				drag_start = divider->vertical ? mb->get_position().y : mb->get_position().x;
				drag_start_ratio = divider->ratio;
				accept_event();
			}
		} else if (dragging) {
			dragging = nullptr;
			queue_redraw();
			accept_event();
		}
		return;
	}

	const Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_null()) {
		return;
	}

	if (dragging) {
		const real_t along = dragging->vertical ? dragging->rect.size.y : dragging->rect.size.x;
		const real_t usable = MAX((real_t)1, along - theme_cache.separation);
		const real_t moved = (dragging->vertical ? mm->get_position().y : mm->get_position().x) - drag_start;
		dragging->ratio = CLAMP(drag_start_ratio + moved / usable, 0.0, 1.0);
		queue_sort();
		accept_event();
		return;
	}

	Slot *under = _find_divider(root, mm->get_position());
	if (under != hovered) {
		hovered = under;
		queue_redraw();
	}
}

Control::CursorShape EditorPaneTree::get_cursor_shape(const Point2 &p_pos) const {
	const Slot *divider = dragging ? dragging : _find_divider(root, p_pos);
	if (divider) {
		return divider->vertical ? CURSOR_VSPLIT : CURSOR_HSPLIT;
	}
	return Control::get_cursor_shape(p_pos);
}

// -------------------------------------------------------------- arrangement

void EditorPaneTree::_update_closable() {
	const Vector<EditorPane *> panes = get_panes();
	for (EditorPane *pane : panes) {
		// The last pane has nowhere to hand its space back to. It keeps its
		// header: every pane has one, so none of them is the special one.
		pane->set_closable(panes.size() > 1);
	}
}

void EditorPaneTree::_wire_pane(EditorPane *p_pane) {
	// The signal hands over "vertical" and bind() appends, so the forwarder
	// takes them in that order and calls split_pane the way it reads.
	p_pane->connect(SNAME("split_requested"), callable_mp(this, &EditorPaneTree::_pane_split_requested).bind(p_pane), CONNECT_DEFERRED);
	p_pane->connect(SNAME("close_requested"), callable_mp(this, &EditorPaneTree::close_pane).bind(p_pane), CONNECT_DEFERRED);
	p_pane->connect(SNAME("float_requested"), callable_mp(this, &EditorPaneTree::_pane_float_requested).bind(p_pane), CONNECT_DEFERRED);
	p_pane->connect(SNAME("return_requested"), callable_mp(this, &EditorPaneTree::_pane_return_requested).bind(p_pane), CONNECT_DEFERRED);
}

void EditorPaneTree::_pane_split_requested(bool p_vertical, EditorPane *p_pane) {
	split_pane(p_pane, p_vertical);
}

void EditorPaneTree::_pane_float_requested(int p_panel, EditorPane *p_pane) {
	emit_signal(SNAME("panel_float_requested"), p_pane, p_panel);
}

void EditorPaneTree::_pane_return_requested(int p_panel, EditorPane *p_pane) {
	emit_signal(SNAME("panel_return_requested"), p_pane, p_panel);
}

EditorPane *EditorPaneTree::split_pane(EditorPane *p_pane, bool p_vertical, bool p_before, bool p_fill) {
	ERR_FAIL_NULL_V(p_pane, nullptr);
	Slot *leaf = _leaf_for(p_pane);
	ERR_FAIL_NULL_V_MSG(leaf, nullptr, "That pane is not in this arrangement.");
	// A pane appearing is the arrangement changing, and it should be seen.
	set_maximized_pane(nullptr);

	EditorPane *new_pane = memnew(EditorPane);
	_wire_pane(new_pane);
	// A direct child, like every other pane. Nothing that is already here moves.
	add_child(new_pane);

	Slot *kept = memnew(Slot);
	kept->pane = p_pane;
	kept->parent = leaf;
	Slot *fresh = memnew(Slot);
	fresh->pane = new_pane;
	fresh->parent = leaf;

	// The leaf becomes the branch, so whatever pointed at it still does.
	leaf->pane = nullptr;
	leaf->vertical = p_vertical;
	leaf->ratio = 0.5;
	leaf->first = p_before ? fresh : kept;
	leaf->second = p_before ? kept : fresh;

	if (p_fill) {
		// Starting on whatever it was split from, so splitting is a way to
		// compare rather than a way to lose your place. A pane showing something
		// the editor owns cannot be copied, so the new one starts on the tree.
		if (p_pane->get_panel_type() != StringName()) {
			new_pane->set_panel_type(p_pane->get_panel_type(), p_pane->get_panel_subject());
		} else if (EditorPanelRegistry::has_type("scene_tree")) {
			new_pane->set_panel_type("scene_tree");
		}
	}

	_update_closable();
	update_minimum_size();
	queue_sort();
	emit_signal(SNAME("layout_changed"));
	return new_pane;
}

EditorPane *EditorPaneTree::split_with_panel(EditorPane *p_target, bool p_vertical, bool p_before, EditorPane *p_source, int p_panel_index) {
	ERR_FAIL_NULL_V(p_target, nullptr);
	ERR_FAIL_NULL_V(p_source, nullptr);

	EditorPane *fresh = split_pane(p_target, p_vertical, p_before, false);
	if (!fresh) {
		return nullptr;
	}
	if (!p_source->transfer_panel_to(fresh, p_panel_index)) {
		// Nothing moved - the main screen refuses to wander - so the pane that
		// was made for it goes again.
		close_pane(fresh);
		return nullptr;
	}
	drop_empty_panes();
	emit_signal(SNAME("layout_changed"));
	return fresh;
}

EditorPane *EditorPaneTree::split_with_new_panel(EditorPane *p_target, bool p_vertical, bool p_before, const StringName &p_type, const Variant &p_subject) {
	ERR_FAIL_NULL_V(p_target, nullptr);

	EditorPane *fresh = split_pane(p_target, p_vertical, p_before, false);
	if (!fresh) {
		return nullptr;
	}
	if (fresh->add_panel(p_type, p_subject) < 0) {
		// The type refused to be built twice, so the pane made for it goes again.
		close_pane(fresh);
		return nullptr;
	}
	emit_signal(SNAME("layout_changed"));
	return fresh;
}

EditorPaneTree::Slot *EditorPaneTree::_first_leaf(Slot *p_slot) {
	while (p_slot && !p_slot->is_leaf()) {
		p_slot = p_slot->first;
	}
	return p_slot;
}

EditorPaneTree::Slot *EditorPaneTree::_last_leaf(Slot *p_slot) {
	while (p_slot && !p_slot->is_leaf()) {
		p_slot = p_slot->second;
	}
	return p_slot;
}

EditorPaneTree::Place EditorPaneTree::get_place_of(const EditorPane *p_pane) const {
	Place place;
	Slot *leaf = _leaf_for(p_pane);
	if (!leaf || !leaf->parent) {
		// The only pane: it is never closed, so it is always where it was.
		return place;
	}
	Slot *branch = leaf->parent;
	const bool first = branch->first == leaf;
	// Across the divider and right next to it, which is what this pane was
	// beside even when the other side is split further.
	Slot *next = first ? _first_leaf(branch->second) : _last_leaf(branch->first);
	place.neighbor = next ? next->pane->get_instance_id() : ObjectID();
	place.vertical = branch->vertical;
	place.before = first;
	place.ratio = branch->ratio;
	return place;
}

EditorPane *EditorPaneTree::make_pane_at(const Place &p_place) {
	EditorPane *neighbor = ObjectDB::get_instance<EditorPane>(p_place.neighbor);
	if (!neighbor || !_leaf_for(neighbor)) {
		return nullptr;
	}
	EditorPane *pane = split_pane(neighbor, p_place.vertical, p_place.before, false);
	Slot *leaf = pane ? _leaf_for(pane) : nullptr;
	if (leaf && leaf->parent) {
		leaf->parent->ratio = CLAMP(p_place.ratio, (real_t)0.05, (real_t)0.95);
		queue_sort();
	}
	return pane;
}

void EditorPaneTree::set_maximized_pane(EditorPane *p_pane) {
	const ObjectID id = (p_pane && _leaf_for(p_pane) && get_panes().size() > 1) ? p_pane->get_instance_id() : ObjectID();
	if (id == maximized) {
		return;
	}
	maximized = id;
	for (EditorPane *pane : get_panes()) {
		pane->set_visible(!id.is_valid() || pane->get_instance_id() == id);
		pane->set_maximized(pane->get_instance_id() == id);
	}
	dragging = nullptr;
	hovered = nullptr;
	queue_sort();
	queue_redraw();
}

EditorPane *EditorPaneTree::get_maximized_pane() const {
	return maximized.is_valid() ? ObjectDB::get_instance<EditorPane>(maximized) : nullptr;
}

void EditorPaneTree::toggle_maximized(EditorPane *p_pane) {
	set_maximized_pane(get_maximized_pane() == p_pane ? nullptr : p_pane);
}

EditorPane *EditorPaneTree::_pane_for_shortcut() const {
	// Whatever has the keyboard, then whatever is under the mouse, then the
	// pane last worked in.
	const Viewport *viewport = get_viewport();
	for (Node *n = viewport ? viewport->gui_get_focus_owner() : nullptr; n && n != this; n = n->get_parent()) {
		EditorPane *pane = Object::cast_to<EditorPane>(n);
		if (pane && _leaf_for(pane)) {
			return pane;
		}
	}
	const Point2 mouse = get_local_mouse_position();
	for (EditorPane *pane : get_panes()) {
		if (pane->is_visible() && pane->get_rect().has_point(mouse)) {
			return pane;
		}
	}
	return get_active_pane();
}

EditorPane *EditorPaneTree::_pane_towards(const EditorPane *p_from, Side p_side) const {
	const Rect2 from = p_from->get_rect();
	const bool across = p_side == SIDE_LEFT || p_side == SIDE_RIGHT;
	EditorPane *best = nullptr;
	real_t best_score = 0;
	for (EditorPane *pane : get_panes()) {
		if (pane == p_from || !pane->is_visible()) {
			continue;
		}
		const Rect2 rect = pane->get_rect();
		real_t gap = 0;
		switch (p_side) {
			case SIDE_LEFT:
				gap = from.position.x - rect.get_end().x;
				break;
			case SIDE_RIGHT:
				gap = rect.position.x - from.get_end().x;
				break;
			case SIDE_TOP:
				gap = from.position.y - rect.get_end().y;
				break;
			case SIDE_BOTTOM:
				gap = rect.position.y - from.get_end().y;
				break;
		}
		if (gap < -1) {
			continue;
		}
		// Beside it: the spans across the direction of travel overlapping.
		// One off at an angle is only taken when nothing is beside it.
		const real_t overlap = across ? MIN(from.get_end().y, rect.get_end().y) - MAX(from.position.y, rect.position.y) : MIN(from.get_end().x, rect.get_end().x) - MAX(from.position.x, rect.position.x);
		const real_t offset = across ? Math::abs(rect.get_center().y - from.get_center().y) : Math::abs(rect.get_center().x - from.get_center().x);
		const real_t score = gap + (overlap > 0 ? offset * 0.1 : 100000 + offset);
		if (!best || score < best_score) {
			best = pane;
			best_score = score;
		}
	}
	return best;
}

void EditorPaneTree::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());
	if (!p_event->is_pressed() || p_event->is_echo()) {
		return;
	}
	if (ED_IS_SHORTCUT("editor/toggle_maximize_pane", p_event)) {
		EditorPane *pane = _pane_for_shortcut();
		if (pane && get_panes().size() > 1) {
			toggle_maximized(pane);
			accept_event();
		}
		return;
	}
	if (ED_IS_SHORTCUT("editor/reopen_closed_panel", p_event)) {
		EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
		if (main_screen && main_screen->reopen_closed_panel()) {
			accept_event();
		}
		return;
	}

	const struct {
		const char *shortcut;
		Side side;
	} directions[] = {
		{ "editor/focus_pane_left", SIDE_LEFT },
		{ "editor/focus_pane_right", SIDE_RIGHT },
		{ "editor/focus_pane_up", SIDE_TOP },
		{ "editor/focus_pane_down", SIDE_BOTTOM },
	};
	for (const auto &direction : directions) {
		if (ED_IS_SHORTCUT(direction.shortcut, p_event)) {
			EditorPane *from = _pane_for_shortcut();
			EditorPane *to = from ? _pane_towards(from, direction.side) : nullptr;
			if (to) {
				set_active_pane(to);
				to->focus_current_panel();
			}
			accept_event();
			return;
		}
	}

	EditorPane *pane = nullptr;
	if (ED_IS_SHORTCUT("editor/next_pane_tab", p_event)) {
		pane = _pane_for_shortcut();
		if (pane) {
			pane->cycle_panel(1);
		}
	} else if (ED_IS_SHORTCUT("editor/previous_pane_tab", p_event)) {
		pane = _pane_for_shortcut();
		if (pane) {
			pane->cycle_panel(-1);
		}
	} else if (ED_IS_SHORTCUT("editor/close_pane_tab", p_event)) {
		pane = _pane_for_shortcut();
		if (pane) {
			pane->close_current_panel();
		}
	}
	if (pane) {
		accept_event();
	}
}

Rect2 EditorPaneTree::get_edge_target_rect(EditorPane::DropZone p_zone) const {
	if (get_panes().size() < 2 || maximized.is_valid()) {
		return Rect2();
	}
	const real_t size = 34 * EDSCALE;
	const real_t margin = 8 * EDSCALE;
	const Size2 all = get_size();
	// Clear of the tabs along the top, which a drop at the top edge would
	// otherwise be fighting for.
	const Slot *first = _first_leaf(root);
	const real_t top = (first && first->pane ? first->pane->get_body_rect().position.y : 0) + margin;
	switch (p_zone) {
		case EditorPane::DROP_LEFT:
			return Rect2(Point2(margin, (all.y - size) * 0.5), Size2(size, size));
		case EditorPane::DROP_RIGHT:
			return Rect2(Point2(all.x - margin - size, (all.y - size) * 0.5), Size2(size, size));
		case EditorPane::DROP_TOP:
			return Rect2(Point2((all.x - size) * 0.5, top), Size2(size, size));
		case EditorPane::DROP_BOTTOM:
			return Rect2(Point2((all.x - size) * 0.5, all.y - margin - size), Size2(size, size));
		default:
			return Rect2();
	}
}

EditorPane::DropZone EditorPaneTree::get_edge_target_at(const Point2 &p_point) const {
	const EditorPane::DropZone edges[] = { EditorPane::DROP_LEFT, EditorPane::DROP_RIGHT, EditorPane::DROP_TOP, EditorPane::DROP_BOTTOM };
	for (const EditorPane::DropZone edge : edges) {
		const Rect2 rect = get_edge_target_rect(edge);
		if (rect.has_area() && rect.has_point(p_point)) {
			return edge;
		}
	}
	return EditorPane::DROP_NONE;
}

EditorPane *EditorPaneTree::split_root(bool p_vertical, bool p_before, real_t p_share) {
	set_maximized_pane(nullptr);

	EditorPane *new_pane = memnew(EditorPane);
	_wire_pane(new_pane);
	add_child(new_pane);

	// Everything there is becomes one side of a new branch at the top.
	Slot *fresh = memnew(Slot);
	fresh->pane = new_pane;
	Slot *branch = memnew(Slot);
	branch->vertical = p_vertical;
	branch->first = p_before ? fresh : root;
	branch->second = p_before ? root : fresh;
	const real_t share = CLAMP(p_share, (real_t)0.05, (real_t)0.95);
	branch->ratio = p_before ? share : 1.0 - share;
	fresh->parent = branch;
	root->parent = branch;
	root = branch;

	_update_closable();
	update_minimum_size();
	queue_sort();
	emit_signal(SNAME("layout_changed"));
	return new_pane;
}

void EditorPaneTree::drop_empty_panes() {
	// Repeated, because closing one collapses a branch and can leave the next
	// one somewhere else in the arrangement.
	bool again = true;
	while (again) {
		again = false;
		for (EditorPane *pane : get_panes()) {
			if (pane->get_panel_count() == 0 && get_panes().size() > 1) {
				close_pane(pane);
				again = true;
				break;
			}
		}
	}
}

void EditorPaneTree::close_pane(EditorPane *p_pane) {
	ERR_FAIL_NULL(p_pane);
	if (get_panes().size() < 2) {
		// Nothing would be left to give the space to.
		return;
	}
	Slot *leaf = _leaf_for(p_pane);
	ERR_FAIL_NULL(leaf);
	Slot *branch = leaf->parent;
	ERR_FAIL_NULL_MSG(branch, "A pane that is not the last one always has a sibling.");
	if (maximized == p_pane->get_instance_id()) {
		set_maximized_pane(nullptr);
	}

	Slot *survivor = branch->first == leaf ? branch->second : branch->first;
	ERR_FAIL_NULL(survivor);

	// The branch becomes whatever is left of it, in place, so nothing else in
	// the arrangement has to be told.
	branch->pane = survivor->pane;
	branch->vertical = survivor->vertical;
	branch->ratio = survivor->ratio;
	branch->first = survivor->first;
	branch->second = survivor->second;
	if (branch->first) {
		branch->first->parent = branch;
	}
	if (branch->second) {
		branch->second->parent = branch;
	}
	survivor->first = nullptr;
	survivor->second = nullptr;
	memdelete(survivor);
	memdelete(leaf);

	remove_child(p_pane);
	memdelete(p_pane);

	if (dragging == branch || hovered == branch) {
		dragging = nullptr;
		hovered = nullptr;
	}

	_update_closable();
	update_minimum_size();
	queue_sort();
	emit_signal(SNAME("layout_changed"));
}

// ------------------------------------------------------------- saving it all

Variant EditorPaneTree::_subject_to_saved(const StringName &p_type, const Variant &p_subject) {
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_type);
	if (!type || type->binding != EditorPanelRegistry::BINDING_DOCUMENT) {
		// A resource is already named by its path, and a panel bound to nothing
		// has nothing to write down.
		return p_subject;
	}

	// A history id means nothing next time the editor runs. The scene's path
	// does. An empty path is "follow whichever document is current", which is
	// what a binding of -1 says while running.
	EditorData &editor_data = EditorNode::get_editor_data();
	if (p_subject.get_type() != Variant::INT || (int)p_subject < 0) {
		return String();
	}
	const int index = editor_data.get_scene_index_by_history_id((int)p_subject);
	if (index < 0) {
		return String();
	}
	return editor_data.get_scene_path(index);
}

Variant EditorPaneTree::_subject_from_saved(const StringName &p_type, const Variant &p_saved) {
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_type);
	if (!type || type->binding != EditorPanelRegistry::BINDING_DOCUMENT) {
		return p_saved;
	}

	const String path = p_saved;
	if (path.is_empty()) {
		return -1;
	}
	EditorData &editor_data = EditorNode::get_editor_data();
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == path) {
			return editor_data.get_scene_history_id(i);
		}
	}
	// The scene this pane was on is not open. Rather than open files behind the
	// user's back, the pane follows the current document until told otherwise.
	return -1;
}

Dictionary EditorPaneTree::_save_slot(const Slot *p_slot) const {
	Dictionary data;
	if (!p_slot) {
		return data;
	}

	if (p_slot->is_leaf()) {
		const EditorPane *pane = p_slot->pane;
		data["kind"] = "pane";
		// Every panel the pane holds, in tab order: type ids and subjects, never
		// class names.
		Array saved_panels;
		for (int i = 0; i < pane->get_panel_count(); i++) {
			Dictionary panel;
			panel["panel"] = String(pane->get_panel_type_at(i));
			panel["subject"] = _subject_to_saved(pane->get_panel_type_at(i), pane->get_panel_subject_at(i));
			const Dictionary state = EditorPanelRegistry::save_panel_state(pane->get_panel_type_at(i), pane->get_panel_at(i));
			if (!state.is_empty()) {
				panel["state"] = state;
			}
			saved_panels.push_back(panel);
		}
		data["panels"] = saved_panels;
		data["current"] = pane->get_current_panel();
		return data;
	}

	data["kind"] = "split";
	data["vertical"] = p_slot->vertical;
	// A share of the space rather than a number of pixels, so an arrangement
	// saved on one screen still looks like itself on another.
	data["ratio"] = p_slot->ratio;
	Array children;
	children.push_back(_save_slot(p_slot->first));
	children.push_back(_save_slot(p_slot->second));
	data["children"] = children;
	return data;
}

Dictionary EditorPaneTree::save_layout() const {
	return _save_slot(root);
}

EditorPaneTree::Slot *EditorPaneTree::_load_slot(const Dictionary &p_data, Slot *p_parent) {
	const String kind = p_data.get("kind", "");

	if (kind == "split") {
		const Array children = p_data.get("children", Array());
		if (children.size() != 2) {
			return nullptr;
		}
		Slot *slot = memnew(Slot);
		slot->parent = p_parent;
		slot->vertical = p_data.get("vertical", false);
		slot->ratio = CLAMP((real_t)(double)p_data.get("ratio", 0.5), (real_t)0, (real_t)1);
		slot->first = _load_slot(children[0], slot);
		slot->second = _load_slot(children[1], slot);
		if (!slot->first || !slot->second) {
			// Half an arrangement is not one. Whichever side survived takes the
			// whole rectangle.
			Slot *survivor = slot->first ? slot->first : slot->second;
			if (!survivor) {
				memdelete(slot);
				return nullptr;
			}
			survivor->parent = p_parent;
			slot->first = nullptr;
			slot->second = nullptr;
			memdelete(slot);
			return survivor;
		}
		return slot;
	}

	if (kind == "pane") {
		EditorPane *pane = memnew(EditorPane);
		_wire_pane(pane);
		add_child(pane);

		const Array saved_panels = p_data.get("panels", Array());
		for (int i = 0; i < saved_panels.size(); i++) {
			const Dictionary panel_data = saved_panels[i];
			const StringName panel = StringName(String(panel_data.get("panel", "")));
			// A type that is no longer registered - an addon removed since -
			// leaves that tab out rather than losing the whole arrangement.
			if (panel != StringName() && EditorPanelRegistry::has_type(panel)) {
				const int at = pane->add_panel(panel, _subject_from_saved(panel, panel_data.get("subject", Variant())));
				if (at >= 0) {
					EditorPanelRegistry::load_panel_state(panel, pane->get_panel_at(at), panel_data.get("state", Dictionary()));
				}
			}
		}
		pane->set_current_panel(p_data.get("current", 0));

		Slot *slot = memnew(Slot);
		slot->parent = p_parent;
		slot->pane = pane;
		return slot;
	}

	return nullptr;
}

void EditorPaneTree::load_layout(const Dictionary &p_layout) {
	if (p_layout.is_empty()) {
		return;
	}

	// The old arrangement goes before the new one is built, not after. There is
	// one FileSystem: if the old panes still held it, the new ones asking for it
	// would be told it was taken, and it would be missing from the arrangement
	// that was supposed to have it.
	const Vector<EditorPane *> old_panes = get_panes();
	_free_slot(root);
	root = nullptr;
	dragging = nullptr;
	hovered = nullptr;
	// Whatever was shown over the others is going too.
	maximized = ObjectID();
	for (EditorPane *pane : old_panes) {
		remove_child(pane);
		memdelete(pane);
	}

	root = _load_slot(p_layout, nullptr);
	if (!root) {
		// Nothing readable in it, and the old arrangement is already gone. One
		// empty pane is still an editor.
		EditorPane *fresh = memnew(EditorPane);
		_wire_pane(fresh);
		add_child(fresh);
		root = memnew(Slot);
		root->pane = fresh;
	}

	_update_closable();
	update_minimum_size();
	queue_sort();
	emit_signal(SNAME("layout_changed"));
}

EditorPaneTree::EditorPaneTree() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	// The panes cover everything except the gaps between them, and the gaps are
	// what this has to hear about.
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_clip_contents(true);

	drop_hint = memnew(EditorPaneDropHint);
	drop_hint->watch(this);
	add_child(drop_hint);

	set_process_shortcut_input(true);

	EditorPane *first = memnew(EditorPane);
	_wire_pane(first);
	add_child(first);
	root = memnew(Slot);
	root->pane = first;

	_update_closable();
}

EditorPaneTree::~EditorPaneTree() {
	_free_slot(root);
	root = nullptr;
}
