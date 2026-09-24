/**************************************************************************/
/*  editor_view_pill.cpp                                                  */
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

#include "editor_view_pill.h"

#include "core/object/callable_mp.h"
#include "editor/gui/editor_view_header_group.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/base_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/resources/style_box_flat.h"

void EditorViewPill::add_mirror(PopupMenu *p_source, const Vector<int> &p_ids, const Vector<int> &p_known, const Vector<PopupMenu *> &p_known_submenus) {
	ERR_FAIL_NULL(p_source);
	Mirror mirror{ p_source->get_instance_id(), p_ids, p_known, Vector<ObjectID>() };
	for (const PopupMenu *submenu : p_known_submenus) {
		if (submenu) {
			mirror.known_submenus.push_back(submenu->get_instance_id());
		}
	}
	mirrors.push_back(mirror);
}

void EditorViewPill::_prepare_source(PopupMenu *p_menu) {
	// Whatever it does to be up to date as it opens, it does now - all but
	// what its own button does, which takes it for open: a button that
	// switches on hover would then open its neighbours under the mouse, every
	// frame, for good.
	const Object *button = Object::cast_to<BaseButton>(p_menu->get_parent());
	List<Object::Connection> connections;
	p_menu->get_signal_connection_list(SNAME("about_to_popup"), &connections);
	for (const Object::Connection &connection : connections) {
		if (button && connection.callable.get_object() == button) {
			continue;
		}
		Variant result;
		Callable::CallError error;
		connection.callable.callp(nullptr, 0, result, error);
	}
}

int EditorViewPill::_find(const PopupMenu *p_menu, int p_id) {
	for (int i = 0; i < p_menu->get_item_count(); i++) {
		if (p_menu->get_item_id(i) == p_id && !p_menu->is_item_separator(i) && !p_menu->get_item_submenu_node(i)) {
			return i;
		}
	}
	return p_menu->get_item_index(p_id);
}

void EditorViewPill::set_chevron(bool p_chevron) {
	if (chevron == p_chevron) {
		return;
	}
	chevron = p_chevron;
	if (is_inside_tree()) {
		notification(NOTIFICATION_THEME_CHANGED);
	}
}

void EditorViewPill::_end_group(PopupMenu *p_menu) {
	const int count = p_menu->get_item_count();
	if (count > 0 && !p_menu->is_item_separator(count - 1)) {
		p_menu->add_separator();
	}
}

void EditorViewPill::_about_to_popup() {
	if (mirrors.is_empty()) {
		// Its owner fills it.
		return;
	}
	PopupMenu *menu = get_popup();
	// Frees the copies of submenus made last time.
	menu->clear(true);
	for (const Mirror &mirror : mirrors) {
		PopupMenu *source = ObjectDB::get_instance<PopupMenu>(mirror.source);
		if (!source) {
			continue;
		}
		_prepare_source(source);
		_end_group(menu);
		for (int id : mirror.ids) {
			if (id == SEPARATOR) {
				_end_group(menu);
			} else if (id == THE_REST) {
				for (int i = 0; i < source->get_item_count(); i++) {
					if (source->is_item_separator(i)) {
						continue;
					}
					const PopupMenu *submenu = source->get_item_submenu_node(i);
					const int item_id = source->get_item_id(i);
					// Named, a submenu is named by its id - or, known, by itself.
					const bool named = submenu ? (mirror.known_submenus.has(submenu->get_instance_id()) || ((mirror.ids.has(item_id) || mirror.known.has(item_id)) && _find(source, item_id) == i)) : (mirror.ids.has(item_id) || mirror.known.has(item_id));
					if (!named) {
						_copy_item(menu, source, i);
					}
				}
			} else {
				const int index = _find(source, id);
				if (index >= 0) {
					_copy_item(menu, source, index);
				}
			}
		}
	}
	while (menu->get_item_count() > 0 && menu->is_item_separator(menu->get_item_count() - 1)) {
		menu->remove_item(menu->get_item_count() - 1);
	}
}

void EditorViewPill::_copy_item(PopupMenu *p_to, PopupMenu *p_from, int p_index) {
	const String label = p_from->get_item_text(p_index);
	const int id = p_from->get_item_id(p_index);
	PopupMenu *submenu = p_from->get_item_submenu_node(p_index);
	if (submenu) {
		_prepare_source(submenu);
		PopupMenu *copy = memnew(PopupMenu);
		copy->set_hide_on_checkable_item_selection(submenu->is_hide_on_checkable_item_selection());
		copy->connect(SNAME("index_pressed"), callable_mp(this, &EditorViewPill::_copy_pressed).bind(copy->get_instance_id()));
		for (int i = 0; i < submenu->get_item_count(); i++) {
			if (submenu->is_item_separator(i)) {
				copy->add_separator(submenu->get_item_text(i));
			} else {
				_copy_item(copy, submenu, i);
			}
		}
		p_to->add_submenu_node_item(label, copy, id);
	} else if (p_from->get_item_max_states(p_index) > 0) {
		p_to->add_multistate_item(label, p_from->get_item_max_states(p_index), p_from->get_item_state(p_index), id);
	} else if (p_from->is_item_radio_checkable(p_index)) {
		p_to->add_radio_check_item(label, id);
	} else if (p_from->is_item_checkable(p_index)) {
		p_to->add_check_item(label, id);
	} else {
		p_to->add_item(label, id);
	}
	const int at = p_to->get_item_count() - 1;
	p_to->set_item_checked(at, p_from->is_item_checked(p_index));
	p_to->set_item_disabled(at, p_from->is_item_disabled(p_index));
	p_to->set_item_tooltip(at, p_from->get_item_tooltip(p_index));
	p_to->set_item_icon(at, p_from->get_item_icon(p_index));
	const Ref<Shortcut> keys = p_from->get_item_shortcut(p_index);
	if (keys.is_valid()) {
		// Shown, not taken: the menu it comes from takes it.
		p_to->set_item_shortcut(at, keys);
	}
	// Where it came from, so that choosing it here chooses it there.
	Array origin;
	origin.push_back((int64_t)(uint64_t)p_from->get_instance_id());
	origin.push_back(id);
	p_to->set_item_metadata(at, origin);
}

void EditorViewPill::_copy_pressed(int p_index, ObjectID p_copy) {
	PopupMenu *copy = ObjectDB::get_instance<PopupMenu>(p_copy);
	if (!copy || p_index < 0 || p_index >= copy->get_item_count()) {
		return;
	}
	const Array origin = copy->get_item_metadata(p_index);
	if (origin.size() != 2) {
		// One of the owner's own items.
		return;
	}
	PopupMenu *source = ObjectDB::get_instance<PopupMenu>(ObjectID((uint64_t)(int64_t)origin[0]));
	const int index = source ? _find(source, origin[1]) : -1;
	if (index < 0 || source->is_item_disabled(index)) {
		return;
	}
	source->activate_item(index);
	// What it switched shows as switched, while the menu stays open.
	if (ObjectDB::get_instance(p_copy)) {
		_sync(get_popup());
	}
}

void EditorViewPill::_sync(PopupMenu *p_copy) {
	for (int i = 0; i < p_copy->get_item_count(); i++) {
		PopupMenu *submenu = p_copy->get_item_submenu_node(i);
		if (submenu) {
			_sync(submenu);
			continue;
		}
		const Array origin = p_copy->get_item_metadata(i);
		if (origin.size() != 2) {
			continue;
		}
		const PopupMenu *source = ObjectDB::get_instance<PopupMenu>(ObjectID((uint64_t)(int64_t)origin[0]));
		const int index = source ? _find(source, origin[1]) : -1;
		if (index < 0) {
			continue;
		}
		p_copy->set_item_checked(i, source->is_item_checked(index));
		p_copy->set_item_disabled(i, source->is_item_disabled(index));
		if (source->get_item_max_states(index) > 0) {
			p_copy->set_item_multistate(i, source->get_item_state(index));
		}
		if (p_copy->get_item_text(i) != source->get_item_text(index)) {
			p_copy->set_item_text(i, source->get_item_text(index));
		}
	}
}

void EditorViewPill::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			if (theming) {
				break;
			}
			theming = true;
			EditorViewHeaderGroup::style_tool_button(this);
			// A little more room either side than a tool has, and the chevron's.
			const Ref<Texture2D> arrow = get_theme_icon(SNAME("arrow"), SNAME("OptionButton"));
			const real_t side = (get_text().is_empty() ? 4 : 7) * EDSCALE;
			const real_t right = chevron ? arrow->get_width() + 5 * EDSCALE : side;
			const StringName styles[] = { SNAME("normal"), SNAME("hover"), SNAME("pressed"), SNAME("hover_pressed"), SNAME("disabled") };
			for (const StringName &name : styles) {
				Ref<StyleBoxFlat> style = get_theme_stylebox(name);
				if (style.is_valid()) {
					style->set_content_margin(SIDE_LEFT, side);
					style->set_content_margin(SIDE_RIGHT, right);
					style->set_content_margin(SIDE_TOP, 3 * EDSCALE);
					style->set_content_margin(SIDE_BOTTOM, 3 * EDSCALE);
				}
			}
			// Light on the accent while it is open.
			add_theme_color_override(SNAME("font_pressed_color"), Color(1, 1, 1));
			add_theme_color_override(SNAME("font_hover_pressed_color"), Color(1, 1, 1));
			theming = false;
			update_minimum_size();
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			if (!chevron) {
				break;
			}
			const Ref<Texture2D> arrow = get_theme_icon(SNAME("arrow"), SNAME("OptionButton"));
			Color color;
			switch (get_draw_mode()) {
				case DRAW_PRESSED:
				case DRAW_HOVER_PRESSED: {
					color = get_theme_color(SNAME("font_pressed_color"));
				} break;
				case DRAW_HOVER: {
					color = get_theme_color(SNAME("font_hover_color"));
				} break;
				case DRAW_DISABLED: {
					color = get_theme_color(SNAME("font_disabled_color"));
				} break;
				default: {
					color = get_theme_color(SceneStringName(font_color));
				} break;
			}
			color.a *= 0.8;
			const Size2 size = get_size();
			const Point2 at = Point2(size.x - arrow->get_width() - 3 * EDSCALE, (size.y - arrow->get_height()) * 0.5).round();
			draw_texture(arrow, at, color);
		} break;
	}
}

EditorViewPill::EditorViewPill() {
	set_flat(false);
	set_focus_mode(FOCUS_NONE);
	// What it shows is taken by the menus it shows it from, not twice here.
	set_disable_shortcuts(true);
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	PopupMenu *menu = get_popup();
	menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	menu->set_hide_on_checkable_item_selection(false);
	menu->connect(SNAME("about_to_popup"), callable_mp(this, &EditorViewPill::_about_to_popup));
	menu->connect(SNAME("index_pressed"), callable_mp(this, &EditorViewPill::_copy_pressed).bind(menu->get_instance_id()));
}
