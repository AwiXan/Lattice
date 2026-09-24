/**************************************************************************/
/*  editor_view_pill.h                                                    */
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

#include "scene/gui/menu_button.h"

// A dropdown of a 2D or 3D view, standing over what the view shows the way
// other editors have theirs: an icon, what it is set to, and a chevron that
// says it opens. It goes in an EditorViewHeaderGroup made to stand over a
// view.
//
// It can show the items of menus kept elsewhere - a view's own menus, which
// hold the state and which addons know - as they are each time it opens;
// choosing one here chooses it there.
class EditorViewPill : public MenuButton {
	GDCLASS(EditorViewPill, MenuButton);

public:
	static constexpr int SEPARATOR = -1;
	// Every item of the source that neither the list nor the ids known to the
	// owner name: whatever an addon added to that menu.
	static constexpr int THE_REST = -2;

private:
	struct Mirror {
		ObjectID source;
		Vector<int> ids;
		Vector<int> known;
		Vector<ObjectID> known_submenus;
	};
	Vector<Mirror> mirrors;
	bool chevron = true;
	bool theming = false;

	void _about_to_popup();
	void _copy_item(PopupMenu *p_to, PopupMenu *p_from, int p_index);
	void _copy_pressed(int p_index, ObjectID p_copy);
	void _sync(PopupMenu *p_copy);
	static void _end_group(PopupMenu *p_menu);
	static void _prepare_source(PopupMenu *p_menu);
	// A submenu added without an id has its index for one, which can be any
	// other item's id too: an id means the item that is not a submenu first.
	static int _find(const PopupMenu *p_menu, int p_id);

protected:
	void _notification(int p_what);

public:
	// Shows p_ids of p_source, in that order, after whatever it shows already.
	// SEPARATOR and THE_REST may be among them; p_known and p_known_submenus
	// are what other pills show of the same menu, left out of THE_REST.
	void add_mirror(PopupMenu *p_source, const Vector<int> &p_ids, const Vector<int> &p_known = Vector<int>(), const Vector<PopupMenu *> &p_known_submenus = Vector<PopupMenu *>());
	void set_chevron(bool p_chevron);

	EditorViewPill();
};
