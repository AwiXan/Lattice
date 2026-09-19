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

#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/settings/editor_settings.h"

void EditorDocumentInspector::_update() {
	EditorData &editor_data = EditorNode::get_editor_data();

	// -1 asks for the document in context, and a document that has been closed
	// resolves to -1 as well, so a panel left pointing at one follows the
	// current document rather than going blank.
	const int index = bound_document_id < 0 ? -1 : editor_data.get_scene_index_by_history_id(bound_document_id);
	EditorSelectionHistory *history = editor_data.get_scene_selection_history(index);

	const ObjectID id = history ? history->get_current() : ObjectID();
	if (id == shown) {
		return;
	}
	shown = id;
	inspector->edit(ObjectDB::get_instance(id));
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

	inspector = memnew(EditorInspector);
	inspector->set_v_size_flags(SIZE_EXPAND_FILL);
	inspector->set_use_folding(!bool(EDITOR_GET("interface/inspector/disable_folding")));
	add_child(inspector);

	set_process(true);
}
