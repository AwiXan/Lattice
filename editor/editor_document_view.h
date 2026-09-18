/**************************************************************************/
/*  editor_document_view.h                                                */
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

#include "scene/gui/box_container.h"

// A view that edits one open document. Panes talk to their contents through
// this and nothing else, so a pane does not have to know whether it is holding
// a 2D view, a 3D view or something an addon brought.
//
// Documents are named by history id, not by tab index: closing or reordering a
// tab moves every index after it, and a pane holding an index would quietly
// start showing a different scene. A binding of -1 means "whichever document is
// current", which is how the editor behaved when only one view could exist, and
// stays the default so that a single-pane layout is unchanged.
class EditorDocumentView : public VBoxContainer {
	GDCLASS(EditorDocumentView, VBoxContainer);

public:
	virtual void bind_document(int p_document_id) {}
	virtual int get_bound_document() const { return -1; }

	// Whether this view can be pointed at a document other than the current
	// one. A view that says no still works in a pane; it just follows along.
	virtual bool supports_document_binding() const { return false; }
};
