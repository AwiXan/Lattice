/**************************************************************************/
/*  editor_document_inspector.cpp                                         */
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

#include "editor_document_inspector.h"

#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"

#include "core/object/callable_mp.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_object_selector.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/settings/editor_settings.h"
#include "core/os/os.h"

void EditorDocumentInspector::_update() {
	EditorData &editor_data = EditorNode::get_editor_data();

	// -1 asks for the document in context, and a document that has been closed
	// resolves to -1 as well, so a panel left pointing at one follows the
	// current document rather than going blank.
	const int index = bound_document_id < 0 ? -1 : editor_data.get_scene_index_by_history_id(bound_document_id);
	EditorSelectionHistory *history = editor_data.get_scene_selection_history(index);

	ObjectID id = history ? history->get_current() : ObjectID();
	Object *wanted = ObjectDB::get_instance(id);
	if (!wanted) {
		// What the history pointed at is gone - the scene was reloaded, an undo
		// made the node anew - while something is still selected: that, rather
		// than nothing until another node is clicked and this one again.
		Node *root = editor_data.get_edited_scene_root(index);
		if (root) {
			const List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list_for(root);
			if (!selected.is_empty()) {
				wanted = selected.front()->get();
				id = wanted->get_instance_id();
			}
		}
	}
	if (id == shown) {
		// The inspector lets go of a node by itself when the node leaves the
		// tree - even only to be moved elsewhere in it - with the history still
		// on the node. Shown again then; not more often than a few times a
		// second, in case it will not have it.
		if (inspector->get_edited_object() == wanted) {
			return;
		}
		const uint64_t now = OS::get_singleton()->get_ticks_msec();
		if (now - last_reshown < 250) {
			return;
		}
		last_reshown = now;
	}
	shown = id;
	inspector->edit(wanted);
	if (object_selector) {
		object_selector->update_path();
	}
}

void EditorDocumentInspector::_activate() {
	// The dock's buttons act on what the current document is on, so the
	// editor comes to this panel's document first - and "Expand All" is to
	// expand this inspector, not the dock's.
	if (bound_document_id >= 0) {
		EditorData &editor_data = EditorNode::get_editor_data();
		const int index = editor_data.get_scene_index_by_history_id(bound_document_id);
		if (index >= 0 && index != editor_data.get_edited_scene()) {
			EditorNode::get_singleton()->set_current_scene_index(index);
		}
	}
	InspectorDock *dock = InspectorDock::get_singleton();
	if (dock) {
		dock->set_menu_inspector(inspector);
	}
}

void EditorDocumentInspector::_resource_selected(const Ref<Resource> &p_resource, const String &p_property) {
	// A sub-resource opened from here: what the dock does with its own.
	if (p_resource.is_null()) {
		return;
	}
	_activate();
	EditorNode::get_singleton()->push_item(p_resource.ptr(), p_property);
}

bool EditorDocumentInspector::_replace_in_toolbar(Node *p_original, Control *p_to) {
	if (Object::cast_to<EditorInspector>(p_original)) {
		// Never the inside of an inspector: its buttons are its properties.
		return true;
	}
	if (Object::cast_to<EditorObjectSelector>(p_original)) {
		// The path is to what this panel shows, so it is this panel's own.
		object_selector = memnew(EditorObjectSelector);
		object_selector->set_h_size_flags(SIZE_EXPAND_FILL);
		p_to->add_child(object_selector);
		object_selector->update_path();
		return true;
	}
	LineEdit *original_filter = Object::cast_to<LineEdit>(p_original);
	if (original_filter) {
		// Where the dock has its filter, this panel has its own.
		filter->set_placeholder(original_filter->get_placeholder());
		filter->set_tooltip_text(original_filter->get_tooltip_text());
		filter->set_h_size_flags(original_filter->get_h_size_flags());
		filter->get_parent()->remove_child(filter);
		p_to->add_child(filter);
		return true;
	}
	return false;
}

void EditorDocumentInspector::_build_toolbars() {
	InspectorDock *dock = InspectorDock::get_singleton();
	if (!dock || !dock->get_toolbars() || toolbars->get_child_count() > 1) {
		return;
	}
	Control *source = dock->get_toolbars();
	for (int i = 0; i < source->get_child_count(false); i++) {
		Node *child = source->get_child(i, false);
		if (Object::cast_to<HBoxContainer>(child)) {
			// A row of the dock's, as a row here.
			HBoxContainer *row = memnew(HBoxContainer);
			toolbars->add_child(row);
			toolbar_mirror.mirror_all(child, row, callable_mp(this, &EditorDocumentInspector::_replace_in_toolbar));
			continue;
		}
		Button *button = Object::cast_to<Button>(child);
		if (button) {
			// The notice that what is shown is read-only, and why.
			toolbars->add_child(toolbar_mirror.mirror(button, source));
		}
		// Anything else is the inspector itself, or a dialog.
	}
	if (filter->get_parent() == toolbars) {
		// The dock had no filter to take the place of: it goes last.
		toolbars->move_child(filter, -1);
	}
}

void EditorDocumentInspector::bind_document(int p_document_id) {
	if (bound_document_id == p_document_id) {
		return;
	}
	bound_document_id = p_document_id;
	if (is_inside_tree()) {
		_update();
	}
}

void EditorDocumentInspector::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_build_toolbars();
			_update();
		} break;

		case NOTIFICATION_PROCESS: {
			// Looked at rather than listened for: a document's inspection
			// history is a plain object with no signal to connect to, and it
			// changes by more routes than the selection does - drilling into a
			// sub-resource moves it without the selection changing at all.
			// Looking costs one comparison, and only a different object is
			// built.
			_update();
			// The copied buttons follow the dock's - which of them can be
			// pressed depends on what is being inspected - and nothing says
			// when that changes either.
			if (is_visible_in_tree()) {
				toolbar_mirror.sync();
			}
		} break;
	}
}

Control *EditorDocumentInspector::create_panel() {
	return memnew(EditorDocumentInspector);
}

void EditorDocumentInspector::bind_panel(Control *p_panel, int p_document_id) {
	EditorDocumentInspector *panel = Object::cast_to<EditorDocumentInspector>(p_panel);
	if (panel) {
		panel->bind_document(p_document_id);
	}
}

EditorDocumentInspector::EditorDocumentInspector() {
	set_v_size_flags(SIZE_EXPAND_FILL);

	// The same inspector the Inspector dock is, made the same way: categories
	// for each class the object is, the documentation on hover, the script and
	// its metadata, and a filter above it. It was a bare EditorInspector, which
	// showed every property in one heap with no way to look for one.
	toolbars = memnew(VBoxContainer);
	add_child(toolbars);
	toolbar_mirror.set_before_press(callable_mp(this, &EditorDocumentInspector::_activate));
	toolbar_mirror.set_shortcut_context(this);

	filter = memnew(LineEdit);
	filter->set_placeholder(TTRC("Filter Properties"));
	filter->set_clear_button_enabled(true);
	filter->set_h_size_flags(SIZE_EXPAND_FILL);
	toolbars->add_child(filter);

	inspector = EditorInspector::create_default_inspector(filter);
	inspector->set_v_size_flags(SIZE_EXPAND_FILL);
	inspector->connect("resource_selected", callable_mp(this, &EditorDocumentInspector::_resource_selected));
	add_child(inspector);

	set_process(true);
}
