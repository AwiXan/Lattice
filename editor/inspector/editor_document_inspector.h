/**************************************************************************/
/*  editor_document_inspector.h                                           */
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

#include "editor/gui/editor_button_mirror.h"
#include "scene/gui/box_container.h"

class EditorInspector;
class EditorObjectSelector;
class LineEdit;
class Resource;

// An inspector showing whatever one open document is on.
//
// InspectorDock is the chrome around the editor's one inspector - the history
// buttons, the resource menu - and is driven by EditorNode::_edit_current(),
// which is a dispatcher for the whole editor rather than a way to fill an
// inspector. A pane cannot call that. What it can do is what the scene tree
// does: ask its document what it is on, which is that document's inspection
// history, and show it.
//
// The chrome is the dock's all the same. Its rows of buttons are copied here,
// and pressing one brings the editor to this panel's document and presses the
// dock's, whose buttons act on what the current document is on - which is what
// this panel is showing. Only the path to the object is this panel's own, and
// the filter, which narrows this inspector.
class EditorDocumentInspector : public VBoxContainer {
	GDCLASS(EditorDocumentInspector, VBoxContainer);

	VBoxContainer *toolbars = nullptr;
	EditorButtonMirror toolbar_mirror;
	EditorObjectSelector *object_selector = nullptr;
	EditorInspector *inspector = nullptr;
	LineEdit *filter = nullptr;

	// The document this panel follows, as a history id; -1 means it follows
	// whichever document is current, which is how the editor's own inspector
	// has always behaved.
	int bound_document_id = -1;
	// What is on screen, so that looking does not mean rebuilding.
	ObjectID shown;
	uint64_t last_reshown = 0;

	void _update();
	void _activate();
	void _build_toolbars();
	bool _replace_in_toolbar(Node *p_original, Control *p_to);
	void _resource_selected(const Ref<Resource> &p_resource, const String &p_property);
	void _object_id_selected(ObjectID p_id);

protected:
	void _notification(int p_what);

public:
	void bind_document(int p_document_id);
	int get_bound_document() const { return bound_document_id; }
	EditorInspector *get_inspector() const { return inspector; }
	// What this panel is showing, for anyone asking rather than watching.
	ObjectID get_shown_object() const { return shown; }

	// What the panel type registers.
	static Control *create_panel();
	static void bind_panel(Control *p_panel, int p_document_id);

	EditorDocumentInspector();
};
