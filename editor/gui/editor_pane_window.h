/**************************************************************************/
/*  editor_pane_window.h                                                  */
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

#include "editor/gui/window_wrapper.h"

class EditorPane;
class EditorPaneTree;

// An arrangement of panes in a window of its own.
//
// The editor's main arrangement is an EditorPaneTree in the main window. This
// is the same thing somewhere else: it can be split, it takes the same panels,
// and it saves itself the same way. A window is not a different kind of place -
// it is a pane tree that happens to be outside.
//
// Panels are sent here by asking rather than by dragging. Godot's drag and drop
// lives inside one Viewport, and a real window is its own, so a tab cannot be
// carried across the gap between two of them. The pane header has a button for
// it instead, and the same button in a window sends the panel back.
class EditorPaneWindow : public WindowWrapper {
	GDCLASS(EditorPaneWindow, WindowWrapper);

	EditorPaneTree *tree = nullptr;

protected:
	static void _bind_methods();

public:
	EditorPaneTree *get_pane_tree() const { return tree; }

	// What the window is called, taken from what it is showing. A window with
	// one panel is named after it; one with several says how many.
	void update_title();

	// The arrangement inside, and where the window is, as data.
	Dictionary save_layout() const;
	void load_layout(const Dictionary &p_layout);

	EditorPaneWindow();
};
