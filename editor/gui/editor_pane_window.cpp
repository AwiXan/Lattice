/**************************************************************************/
/*  editor_pane_window.cpp                                                */
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

#include "editor_pane_window.h"

#include "editor/gui/editor_pane.h"
#include "editor/gui/editor_pane_tree.h"

void EditorPaneWindow::_bind_methods() {
}

void EditorPaneWindow::update_title() {
	const Vector<EditorPane *> panes = tree->get_panes();
	int panels = 0;
	String first;
	for (const EditorPane *pane : panes) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (first.is_empty()) {
				first = pane->get_panel_title_at(i);
			}
			panels++;
		}
	}

	String title;
	if (panels == 0) {
		title = TTR("Empty");
	} else if (panels == 1) {
		title = first;
	} else {
		title = vformat(TTR("%s and %d more"), first, panels - 1);
	}
	set_window_title(vformat(TTR("%s - Godot Engine"), title));
}

Dictionary EditorPaneWindow::save_layout() const {
	Dictionary data;
	data["panes"] = tree->save_layout();
	data["rect"] = get_window_rect();
	data["screen"] = get_window_screen();
	return data;
}

void EditorPaneWindow::load_layout(const Dictionary &p_layout) {
	const Dictionary panes = p_layout.get("panes", Dictionary());
	if (!panes.is_empty()) {
		tree->load_layout(panes);
	}
	update_title();

	const Rect2i rect = p_layout.get("rect", Rect2i());
	if (rect.size.x > 0 && rect.size.y > 0) {
		restore_window(rect, p_layout.get("screen", -1));
	} else {
		enable_window_on_screen();
	}
}

EditorPaneWindow::EditorPaneWindow() {
	set_margins_enabled(true);

	tree = memnew(EditorPaneTree);
	tree->set_windowed(true);
	set_wrapped_control(tree);
}
