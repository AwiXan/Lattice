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

#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_panel_registry.h"
#include "editor/gui/editor_pane.h"
#include "scene/gui/split_container.h"

void EditorPaneTree::_bind_methods() {
	ADD_SIGNAL(MethodInfo("layout_changed"));
}

EditorPane *EditorPaneTree::get_first_pane() const {
	Vector<EditorPane *> panes = get_panes();
	return panes.is_empty() ? nullptr : panes[0];
}

void EditorPaneTree::_collect_panes(Control *p_node, Vector<EditorPane *> &r_panes) const {
	if (!p_node) {
		return;
	}
	EditorPane *pane = Object::cast_to<EditorPane>(p_node);
	if (pane) {
		r_panes.push_back(pane);
		return;
	}
	SplitContainer *split = Object::cast_to<SplitContainer>(p_node);
	if (split) {
		for (int i = 0; i < split->get_child_count(); i++) {
			_collect_panes(Object::cast_to<Control>(split->get_child(i)), r_panes);
		}
	}
}

Vector<EditorPane *> EditorPaneTree::get_panes() const {
	Vector<EditorPane *> panes;
	_collect_panes(root, panes);
	return panes;
}

void EditorPaneTree::_update_closable() {
	const Vector<EditorPane *> panes = get_panes();
	for (EditorPane *pane : panes) {
		// The last pane has nowhere to hand its space back to, and nothing to
		// choose between, so it shows no header at all.
		pane->set_closable(panes.size() > 1 && !pane->is_adopting());
		pane->set_header_visible(panes.size() > 1);
	}
}

EditorPane *EditorPaneTree::split_pane(EditorPane *p_pane, bool p_vertical, bool p_before, bool p_fill) {
	ERR_FAIL_NULL_V(p_pane, nullptr);

	Node *parent = p_pane->get_parent();
	ERR_FAIL_NULL_V(parent, nullptr);
	const int index = p_pane->get_index();

	SplitContainer *split = p_vertical ? (SplitContainer *)memnew(VSplitContainer) : (SplitContainer *)memnew(HSplitContainer);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	split->set_h_size_flags(SIZE_EXPAND_FILL);

	EditorPane *new_pane = memnew(EditorPane);
	_wire_pane(new_pane);

	// The pane takes the place of the one it was split from, which then moves
	// inside it beside its new neighbour.
	parent->remove_child(p_pane);
	parent->add_child(split);
	parent->move_child(split, index);
	if (p_before) {
		split->add_child(new_pane);
		split->add_child(p_pane);
	} else {
		split->add_child(p_pane);
		split->add_child(new_pane);
	}

	if (root == p_pane) {
		root = split;
	}

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
	emit_signal(SNAME("layout_changed"));
	return new_pane;
}

void EditorPaneTree::_wire_pane(EditorPane *p_pane) {
	// The signal hands over "vertical" and bind() appends, so the forwarder
	// takes them in that order and calls split_pane the way it reads.
	p_pane->connect(SNAME("split_requested"), callable_mp(this, &EditorPaneTree::_pane_split_requested).bind(p_pane), CONNECT_DEFERRED);
	p_pane->connect(SNAME("close_requested"), callable_mp(this, &EditorPaneTree::close_pane).bind(p_pane), CONNECT_DEFERRED);
}

void EditorPaneTree::_pane_split_requested(bool p_vertical, EditorPane *p_pane) {
	split_pane(p_pane, p_vertical);
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

void EditorPaneTree::drop_empty_panes() {
	// Repeated, because closing one collapses a split and can leave the next
	// one somewhere else in the tree.
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

void EditorPaneTree::_collapse_split(SplitContainer *p_split, Control *p_survivor) {
	Node *parent = p_split->get_parent();
	const int index = p_split->get_index();

	p_split->remove_child(p_survivor);
	parent->remove_child(p_split);
	memdelete(p_split);

	parent->add_child(p_survivor);
	parent->move_child(p_survivor, index);

	if (root == p_split) {
		root = p_survivor;
	}
}

void EditorPaneTree::close_pane(EditorPane *p_pane) {
	ERR_FAIL_NULL(p_pane);
	if (get_panes().size() < 2) {
		// Nothing would be left to give the space to.
		return;
	}
	if (p_pane->is_adopting()) {
		// This one holds the editor's main screen, which has to be somewhere.
		return;
	}

	SplitContainer *split = Object::cast_to<SplitContainer>(p_pane->get_parent());
	ERR_FAIL_NULL_MSG(split, "A pane that is not the last one is always inside a split.");

	Control *survivor = nullptr;
	for (int i = 0; i < split->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(split->get_child(i));
		if (child && child != p_pane) {
			survivor = child;
			break;
		}
	}
	ERR_FAIL_NULL(survivor);

	split->remove_child(p_pane);
	memdelete(p_pane);

	_collapse_split(split, survivor);
	_update_closable();
	emit_signal(SNAME("layout_changed"));
}

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

Dictionary EditorPaneTree::_save_node(Control *p_node) const {
	Dictionary data;
	if (!p_node) {
		return data;
	}

	const EditorPane *pane = Object::cast_to<EditorPane>(p_node);
	if (pane) {
		data["kind"] = "pane";
		// Every panel the pane holds, in tab order. Type ids and subjects, never
		// classes; "adopted" marks the one that is the editor's main screen, so
		// that restoring an arrangement puts it back rather than losing it.
		Array saved_panels;
		for (int i = 0; i < pane->get_panel_count(); i++) {
			Dictionary panel;
			panel["panel"] = String(pane->get_panel_type_at(i));
			panel["subject"] = _subject_to_saved(pane->get_panel_type_at(i), pane->get_panel_subject_at(i));
			panel["adopted"] = pane->is_panel_adopted_at(i);
			saved_panels.push_back(panel);
		}
		data["panels"] = saved_panels;
		data["current"] = pane->get_current_panel();
		return data;
	}

	const SplitContainer *split = Object::cast_to<SplitContainer>(p_node);
	if (split) {
		data["kind"] = "split";
		data["vertical"] = split->is_vertical();
		data["offset"] = split->get_split_offset();
		Array children;
		for (int i = 0; i < split->get_child_count(); i++) {
			// Only what was put there: a SplitContainer keeps a dragger of its
			// own among its children, and that is not part of the arrangement.
			Control *child = Object::cast_to<Control>(split->get_child(i));
			if (Object::cast_to<EditorPane>(child) || Object::cast_to<SplitContainer>(child)) {
				children.push_back(_save_node(child));
			}
		}
		data["children"] = children;
	}
	return data;
}

Dictionary EditorPaneTree::save_layout() const {
	return _save_node(root);
}

Control *EditorPaneTree::_load_node(const Dictionary &p_data) {
	const String kind = p_data.get("kind", "");
	if (kind == "split") {
		const bool vertical = p_data.get("vertical", false);
		SplitContainer *split = vertical ? (SplitContainer *)memnew(VSplitContainer) : (SplitContainer *)memnew(HSplitContainer);
		split->set_v_size_flags(SIZE_EXPAND_FILL);
		split->set_h_size_flags(SIZE_EXPAND_FILL);

		const Array children = p_data.get("children", Array());
		for (int i = 0; i < children.size(); i++) {
			const Dictionary child_data = children[i];
			if (child_data.is_empty()) {
				continue;
			}
			Control *child = _load_node(child_data);
			if (child) {
				split->add_child(child);
			}
		}
		split->set_split_offset(p_data.get("offset", 0));
		return split;
	}

	if (kind == "pane") {
		EditorPane *pane = memnew(EditorPane);
		_wire_pane(pane);

		const Array saved_panels = p_data.get("panels", Array());
		for (int i = 0; i < saved_panels.size(); i++) {
			const Dictionary panel_data = saved_panels[i];
			if (bool(panel_data.get("adopted", false))) {
				// The editor's main screen. It is given back to this pane once
				// the old arrangement has let go of it.
				pending_main_screen_host = pane;
				continue;
			}
			const StringName panel = StringName(String(panel_data.get("panel", "")));
			// A type that is no longer registered - an addon removed since -
			// leaves that tab out rather than losing the whole arrangement.
			if (panel != StringName() && EditorPanelRegistry::has_type(panel)) {
				pane->add_panel(panel, _subject_from_saved(panel, panel_data.get("subject", Variant())));
			}
		}
		pane->set_current_panel(p_data.get("current", 0));
		return pane;
	}

	return nullptr;
}

void EditorPaneTree::load_layout(const Dictionary &p_layout) {
	pending_main_screen_host = nullptr;
	Control *loaded = _load_node(p_layout);
	if (!loaded) {
		return;
	}

	// The editor's main screen is shown by one of the panes about to be thrown
	// away. Take it out first: destroying the arrangement must not destroy the
	// editor with it.
	Control *main_screen = nullptr;
	String main_screen_title;
	for (EditorPane *pane : get_panes()) {
		if (pane->is_adopting()) {
			main_screen_title = pane->get_adopted_title();
			main_screen = pane->release_adopted_panel();
			break;
		}
	}

	if (root) {
		remove_child(root);
		memdelete(root);
	}
	root = loaded;
	add_child(root);

	if (main_screen) {
		// An arrangement that says nothing about where the main screen goes -
		// saved by an older build, or hand-edited - still has to put it
		// somewhere, and the first pane is where it started.
		EditorPane *host = pending_main_screen_host ? pending_main_screen_host : get_first_pane();
		if (host) {
			host->adopt_panel(main_screen, main_screen_title);
		}
	}
	pending_main_screen_host = nullptr;

	_update_closable();
	emit_signal(SNAME("layout_changed"));
}

void EditorPaneTree::adopt_main_screen(Control *p_main_screen, const String &p_title) {
	EditorPane *first = get_first_pane();
	ERR_FAIL_NULL(first);
	first->adopt_panel(p_main_screen, p_title);
}

EditorPaneTree::EditorPaneTree() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);

	EditorPane *first = memnew(EditorPane);
	_wire_pane(first);
	root = first;
	add_child(root);
	_update_closable();
}
