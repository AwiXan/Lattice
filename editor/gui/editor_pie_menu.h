/**************************************************************************/
/*  editor_pie_menu.h                                                     */
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

#include "scene/gui/control.h"

// A menu around the mouse, one choice per direction, the way Blender has
// them: hold its key, move towards a choice and let go; or tap the key and
// pick with a click or with the numpad digit pointing the same way (8 up,
// 4 left, 9 up and right...). Escape, the right button or the key again
// closes it without choosing.
//
// It covers its parent - the view it was opened in - while open, and is
// hidden otherwise. What the choices do is the view's business.
class EditorPieMenu : public Control {
	GDCLASS(EditorPieMenu, Control);

public:
	// Numbered as the numpad points: 4 is left, 8 up.
	enum Direction {
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_BOTTOM,
		DIRECTION_TOP,
		DIRECTION_TOP_LEFT,
		DIRECTION_TOP_RIGHT,
		DIRECTION_BOTTOM_LEFT,
		DIRECTION_BOTTOM_RIGHT,
		DIRECTION_MAX
	};

	struct Item {
		String text;
		Ref<Texture2D> icon;
		Callable action;
		// What is in effect now, drawn so.
		bool current = false;
	};

private:
	Item items[DIRECTION_MAX];
	bool used[DIRECTION_MAX] = {};
	String title;
	Point2 center;
	int hovered = -1;
	Key held_key = Key::NONE;
	uint64_t opened_at = 0;
	bool moved = false;

	Rect2 item_rects[DIRECTION_MAX];

	static Vector2 _direction_vector(int p_direction);
	void _layout();
	int _item_towards(const Point2 &p_point) const;
	void _choose(int p_direction);

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	static void _bind_methods();

public:
	void clear();
	void set_item(Direction p_direction, const Item &p_item);
	void set_title(const String &p_title) { title = p_title; }
	// Opens around p_at, in this control's coordinates. p_key is the key that
	// opened it: letting it go after moving towards a choice chooses it.
	void open(const Point2 &p_at, Key p_key);
	void close();
	bool is_open() const { return is_visible(); }

	// For checking it without a mouse.
	int get_hovered() const { return hovered; }
	void hover_towards(const Point2 &p_point);
	void choose_hovered();
	Rect2 get_item_rect(Direction p_direction) const { return item_rects[p_direction]; }
	Point2 get_center() const { return center; }

	EditorPieMenu();
};
