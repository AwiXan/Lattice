/**************************************************************************/
/*  editor_view_hints.cpp                                                 */
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

#include "editor_view_hints.h"

#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/rich_text_label.h"
#include "scene/resources/font.h"

String EditorViewHints::mouse_button_name(MouseButton p_button) {
	switch (p_button) {
		case MouseButton::LEFT:
			return TTR("LMB");
		case MouseButton::MIDDLE:
			return TTR("MMB");
		case MouseButton::RIGHT:
			return TTR("RMB");
		case MouseButton::MB_XBUTTON1:
			return TTR("Mouse 4");
		case MouseButton::MB_XBUTTON2:
			return TTR("Mouse 5");
		default:
			return String();
	}
}

String EditorViewHints::action_key(const StringName &p_action) {
	InputMap *input_map = InputMap::get_singleton();
	if (!input_map->has_action(p_action)) {
		return String();
	}
	const List<Ref<InputEvent>> *events = input_map->action_get_events(p_action);
	if (!events) {
		return String();
	}
	for (const Ref<InputEvent> &event : *events) {
		const Ref<InputEventKey> key = event;
		if (key.is_valid()) {
			return key->as_text();
		}
	}
	return String();
}

void EditorViewHints::set_hints(const Vector<Hint> &p_hints) {
	bool same = p_hints.size() == shown.size();
	for (int i = 0; same && i < p_hints.size(); i++) {
		same = p_hints[i].keys == shown[i].keys && p_hints[i].action == shown[i].action;
	}
	if (same) {
		return;
	}
	shown = p_hints;
	_update_text();
}

String EditorViewHints::get_text() const {
	return label->get_parsed_text();
}

void EditorViewHints::_update_text() {
	const Color keys_color = get_theme_color(SceneStringName(font_color), SNAME("Label"));
	const Color action_color = get_theme_color(SNAME("readonly_font_color"), EditorStringName(Editor));
	String text;
	for (int i = 0; i < shown.size(); i++) {
		if (i > 0) {
			text += "      ";
		}
		text += vformat("[color=%s][b]%s[/b][/color] [color=%s]%s[/color]", keys_color.to_html(false), shown[i].keys.xml_escape(), action_color.to_html(false), shown[i].action.xml_escape());
	}
	label->set_text(text);
}

void EditorViewHints::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			// One line high, and as narrow as the view gets: a label fitted to
			// its text would make the view as wide as the hints, and a pane
			// holding it could not be made narrower than that.
			const Ref<Font> font = label->get_theme_font(SNAME("bold_font"));
			const int font_size = label->get_theme_font_size(SNAME("bold_font_size"));
			label->set_custom_minimum_size(Size2(1, font.is_valid() ? font->get_height(font_size) + 2 * EDSCALE : 20 * EDSCALE));
			_update_text();
		} break;
	}
}

EditorViewHints::EditorViewHints() {
	set_name("KeyHints");
	add_theme_constant_override("margin_left", 8 * EDSCALE);
	add_theme_constant_override("margin_right", 8 * EDSCALE);
	add_theme_constant_override("margin_top", 2 * EDSCALE);
	add_theme_constant_override("margin_bottom", 2 * EDSCALE);

	label = memnew(RichTextLabel);
	label->set_use_bbcode(true);
	label->set_scroll_active(false);
	label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	label->set_clip_contents(true);
	label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	label->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(label);
}
