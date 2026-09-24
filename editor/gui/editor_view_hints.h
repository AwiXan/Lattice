/**************************************************************************/
/*  editor_view_hints.h                                                   */
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

#include "scene/gui/margin_container.h"

class RichTextLabel;

// A line under a 2D or 3D view saying what the mouse buttons and keys do
// right now: selecting, dragging a manipulator, flying. It changes with what
// is going on, so the view asks for it to be said again whenever that might
// have changed; saying the same thing twice costs nothing.
class EditorViewHints : public MarginContainer {
	GDCLASS(EditorViewHints, MarginContainer);

public:
	struct Hint {
		String keys;
		String action;
	};

private:
	RichTextLabel *label = nullptr;
	Vector<Hint> shown;

	void _update_text();

protected:
	void _notification(int p_what);

public:
	void set_hints(const Vector<Hint> &p_hints);
	String get_text() const;

	// The name of a mouse button in a hint: LMB, MMB, RMB.
	static String mouse_button_name(MouseButton p_button);
	// The first key an input action is bound to, or nothing.
	static String action_key(const StringName &p_action);

	EditorViewHints();
};
