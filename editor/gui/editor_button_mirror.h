/**************************************************************************/
/*  editor_button_mirror.h                                                */
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

#include "core/object/object_id.h"
#include "core/templates/local_vector.h"
#include "core/variant/callable.h"

class Button;
class Control;
class Node;

// Copies of a dock's buttons, for a panel to offer as its own.
//
// A dock is one of a kind and its buttons act on what the editor is working
// on. A panel doing the same job in any number of panes need not know what
// each of them does: a copy looks like the original, is shown and enabled
// when it is, and pressing it presses the original - after the panel has
// brought the editor to what the panel is about. A button the dock gains
// later, or an addon puts there, turns up in the copy too.
class EditorButtonMirror {
	struct Mirror {
		Control *copy = nullptr;
		ObjectID original;
		// Where the copying started: the original counts as shown only if
		// everything between it and here is, which is how one of two lists the
		// dock switches between stays hidden with its list.
		ObjectID root;
	};
	LocalVector<Mirror> mirrors;
	Callable before_press;
	const Node *shortcut_context = nullptr;
	bool suppressed = false;

	static void _pressed(ObjectID p_original, ObjectID p_copy, const Callable &p_before_press);
	static bool _is_shown(Node *p_node, Node *p_root);
	void _mirror_into(Node *p_from, Control *p_to, Node *p_root, const Callable &p_replace);

public:
	// Called before an original is pressed or its menu shown.
	void set_before_press(const Callable &p_callable) { before_press = p_callable; }
	// A copy's shortcut only works while the focus is inside this, so that two
	// panels showing the same buttons do not both answer one key.
	void set_shortcut_context(const Node *p_context) { shortcut_context = p_context; }

	// A copy of one button.
	Button *mirror(Button *p_original, Node *p_root = nullptr);
	// Copies of the buttons and labels under p_from, in order, into p_to, as a
	// flat list. p_replace, if given, is asked first about every node with
	// (node, p_to) and answers true when it has put something of its own in
	// that node's place - or chosen to leave it out.
	void mirror_all(Node *p_from, Control *p_to, const Callable &p_replace = Callable());

	// Brings the copies up to date with their originals. Cheap enough to do
	// every frame something showing them is on screen.
	void sync();
	// Keeps every copy hidden, whatever its original is doing - for a panel
	// showing something the buttons are not about.
	void set_suppressed(bool p_suppressed) { suppressed = p_suppressed; }

	// Forgets the copies; freeing them is up to whoever placed them.
	void clear() { mirrors.clear(); }
};
