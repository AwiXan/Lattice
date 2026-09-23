/**************************************************************************/
/*  editor_button_mirror.cpp                                              */
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

#include "editor_button_mirror.h"

#include "core/object/callable_mp.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/scene_string_names.h"

void EditorButtonMirror::_pressed(ObjectID p_original, ObjectID p_copy, const Callable &p_before_press) {
	Button *original = ObjectDB::get_instance<Button>(p_original);
	Button *copy = ObjectDB::get_instance<Button>(p_copy);
	if (!original || !copy) {
		return;
	}
	if (p_before_press.is_valid()) {
		p_before_press.call();
	}

	MenuButton *menu_button = Object::cast_to<MenuButton>(original);
	if (menu_button) {
		// Filled the moment it is about to show, as it always was, then shown
		// under the copy rather than under the original - which may be nowhere
		// on screen.
		menu_button->emit_signal(SNAME("about_to_popup"));
		PopupMenu *popup = menu_button->get_popup();
		// Belonging to the window it was opened from, which is where the copy
		// is: owned by the one the original is in, it could open behind that
		// window, and closing it would bring the other one to the front.
		popup->set_transient_to_focused(true);
		const Rect2 rect = copy->get_screen_rect();
		popup->reset_size();
		popup->set_position(rect.position + Vector2(0, rect.size.height));
		popup->popup();
		return;
	}
	if (original->is_toggle_mode()) {
		original->set_pressed(!original->is_pressed());
	} else {
		original->emit_signal(SceneStringName(pressed));
	}
}

bool EditorButtonMirror::_is_shown(Node *p_node, Node *p_root) {
	for (Node *node = p_node; node && node != p_root; node = node->get_parent()) {
		const CanvasItem *item = Object::cast_to<CanvasItem>(node);
		if (item && !item->is_visible()) {
			return false;
		}
	}
	return true;
}

Button *EditorButtonMirror::mirror(Button *p_original, Node *p_root) {
	Button *copy = memnew(Button);
	copy->set_theme_type_variation(p_original->get_theme_type_variation());
	copy->set_flat(p_original->is_flat());
	copy->set_text(p_original->get_text());
	copy->set_button_icon(p_original->get_button_icon());
	copy->set_expand_icon(p_original->is_expand_icon());
	copy->set_tooltip_text(p_original->get_tooltip_text());
	copy->set_clip_text(p_original->get_clip_text());
	copy->set_text_alignment(p_original->get_text_alignment());
	copy->set_h_size_flags(p_original->get_h_size_flags());
	copy->set_visible(_is_shown(p_original, p_root));
	copy->set_disabled(p_original->is_disabled());
	// The keys meant for whatever has the focus - a tree, an inspector - keep
	// reaching it.
	copy->set_focus_mode(Control::FOCUS_NONE);
	if (p_original->get_shortcut().is_valid()) {
		copy->set_shortcut(p_original->get_shortcut());
		copy->set_shortcut_context(shortcut_context);
	}
	copy->connect(SceneStringName(pressed), callable_mp_static(&EditorButtonMirror::_pressed).bind(p_original->get_instance_id(), copy->get_instance_id(), before_press));

	Mirror entry;
	entry.copy = copy;
	entry.original = p_original->get_instance_id();
	entry.root = p_root ? p_root->get_instance_id() : ObjectID();
	mirrors.push_back(entry);
	return copy;
}

void EditorButtonMirror::_mirror_into(Node *p_from, Control *p_to, Node *p_root, const Callable &p_replace) {
	for (int i = 0; i < p_from->get_child_count(false); i++) {
		Node *child = p_from->get_child(i, false);
		if (!Object::cast_to<Control>(child)) {
			// Dialogs and the like: what a button opens, not what it looks like.
			continue;
		}
		if (p_replace.is_valid() && bool(p_replace.call(child, p_to))) {
			continue;
		}

		Button *button = Object::cast_to<Button>(child);
		if (button) {
			// A toggle only switches what the dock lists, and what it lists is
			// what is copied. A menu button is one too, but only to look pressed
			// while its menu is open.
			if (!button->is_toggle_mode() || Object::cast_to<MenuButton>(button)) {
				p_to->add_child(mirror(button, p_root));
			}
			continue;
		}

		Label *label = Object::cast_to<Label>(child);
		if (label) {
			Label *copy = memnew(Label);
			copy->set_text(label->get_text());
			copy->set_theme_type_variation(label->get_theme_type_variation());
			copy->set_horizontal_alignment(label->get_horizontal_alignment());
			copy->set_visible(_is_shown(label, p_root));
			p_to->add_child(copy);

			Mirror entry;
			entry.copy = copy;
			entry.original = label->get_instance_id();
			entry.root = p_root->get_instance_id();
			mirrors.push_back(entry);
			continue;
		}

		_mirror_into(child, p_to, p_root, p_replace);
	}
}

void EditorButtonMirror::mirror_all(Node *p_from, Control *p_to, const Callable &p_replace) {
	ERR_FAIL_NULL(p_from);
	ERR_FAIL_NULL(p_to);
	_mirror_into(p_from, p_to, p_from, p_replace);
}

void EditorButtonMirror::sync() {
	for (const Mirror &entry : mirrors) {
		Control *original = ObjectDB::get_instance<Control>(entry.original);
		if (!original) {
			entry.copy->hide();
			continue;
		}
		Node *root = entry.root.is_valid() ? ObjectDB::get_instance<Node>(entry.root) : nullptr;
		entry.copy->set_visible(_is_shown(original, root));

		Button *original_button = Object::cast_to<Button>(original);
		Button *copy_button = Object::cast_to<Button>(entry.copy);
		if (original_button && copy_button) {
			copy_button->set_disabled(original_button->is_disabled());
			if (copy_button->get_button_icon() != original_button->get_button_icon()) {
				copy_button->set_button_icon(original_button->get_button_icon());
			}
			if (copy_button->get_text() != original_button->get_text()) {
				copy_button->set_text(original_button->get_text());
			}
			if (copy_button->get_tooltip_text() != original_button->get_tooltip_text()) {
				copy_button->set_tooltip_text(original_button->get_tooltip_text());
			}
			continue;
		}

		Label *original_label = Object::cast_to<Label>(original);
		Label *copy_label = Object::cast_to<Label>(entry.copy);
		if (original_label && copy_label && copy_label->get_text() != original_label->get_text()) {
			copy_label->set_text(original_label->get_text());
		}
	}
}
