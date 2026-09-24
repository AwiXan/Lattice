/**************************************************************************/
/*  editor_view_sidebar.h                                                 */
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

#include "scene/gui/panel_container.h"

class ScrollContainer;
class TabBar;
class VBoxContainer;

// The sidebar a 2D or 3D view opens with N: a card of pages in the top right
// corner of the view, over the scene rather than beside it, and only as tall
// as the page it shows - so it covers no more of the view than it needs to.
// A page taller than the view scrolls.
//
// It lays itself out in its parent, which should be a plain Control covering
// the view. What goes on the pages is the view's business.
class EditorViewSidebar : public PanelContainer {
	GDCLASS(EditorViewSidebar, PanelContainer);

	TabBar *tabs = nullptr;
	ScrollContainer *scroll = nullptr;
	VBoxContainer *page_box = nullptr;
	LocalVector<Control *> pages;
	bool fitting = false;
	bool theming = false;

	void _tab_changed(int p_tab);
	void _parent_resized();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	int add_page(const String &p_title, Control *p_page);
	int get_page_count() const { return pages.size(); }
	Control *get_page(int p_index) const;
	int get_current_page() const;
	// Shows the card too.
	void show_page(int p_index);
	// With the card shown on another page, shows this one; on this one, hides it.
	void toggle_page(int p_index);
	void toggle();

	// Where it is and how big, from its parent's size and the page's.
	void fit();

	EditorViewSidebar();
};
