/**************************************************************************/
/*  editor_pie_menu.cpp                                                   */
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

#include "editor_pie_menu.h"

#include "core/input/input_event.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box_flat.h"

static constexpr real_t PIE_RADIUS = 96;
static constexpr real_t PIE_DEAD_ZONE = 18;
static constexpr real_t PIE_ITEM_HEIGHT = 30;
static constexpr real_t PIE_RING = 16;

Vector2 EditorPieMenu::_direction_vector(int p_direction) {
	// Where each choice sits, as a fraction of the radius. Not a circle: the
	// choices are wider than they are tall, so the top and bottom ones go
	// further out and the corner ones lower down, or they crowd each other.
	static const Vector2 vectors[DIRECTION_MAX] = {
		Vector2(-1, 0), Vector2(1, 0), Vector2(0, 1.25), Vector2(0, -1.25),
		Vector2(-0.8, -0.6), Vector2(0.8, -0.6), Vector2(-0.8, 0.6), Vector2(0.8, 0.6)
	};
	return vectors[p_direction];
}

void EditorPieMenu::clear() {
	for (int i = 0; i < DIRECTION_MAX; i++) {
		items[i] = Item();
		used[i] = false;
		item_rects[i] = Rect2();
	}
	title = String();
}

void EditorPieMenu::set_item(Direction p_direction, const Item &p_item) {
	ERR_FAIL_INDEX(p_direction, DIRECTION_MAX);
	items[p_direction] = p_item;
	used[p_direction] = true;
}

void EditorPieMenu::_layout() {
	const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Button"));
	const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Button"));
	const real_t radius = PIE_RADIUS * EDSCALE;
	const real_t height = PIE_ITEM_HEIGHT * EDSCALE;
	const real_t padding = 12 * EDSCALE;

	// Laid out once where it was asked for, then moved as a whole to be all
	// on screen: near an edge of the view, the choices stay where they point.
	for (int pass = 0; pass < 2; pass++) {
		Rect2 bounds(center - Vector2(PIE_RING, PIE_RING) * EDSCALE, Vector2(PIE_RING, PIE_RING) * 2 * EDSCALE);
		for (int i = 0; i < DIRECTION_MAX; i++) {
			if (!used[i]) {
				continue;
			}
			real_t width = padding * 2;
			if (font.is_valid()) {
				width += font->get_string_size(items[i].text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).width;
			}
			if (items[i].icon.is_valid()) {
				width += items[i].icon->get_width() + 6 * EDSCALE;
			}
			const Vector2 direction = _direction_vector(i);
			const Point2 anchor = center + direction * radius;
			Point2 position(anchor.x - width * 0.5, anchor.y - height * 0.5);
			if (direction.x > 0.3) {
				position.x = anchor.x;
			} else if (direction.x < -0.3) {
				position.x = anchor.x - width;
			}
			item_rects[i] = Rect2(position, Size2(width, height));
			bounds = bounds.merge(item_rects[i]);
		}
		if (pass == 1) {
			break;
		}
		const real_t margin = 6 * EDSCALE;
		Vector2 shift;
		if (bounds.position.x < margin) {
			shift.x = margin - bounds.position.x;
		} else if (bounds.get_end().x > get_size().width - margin) {
			shift.x = get_size().width - margin - bounds.get_end().x;
		}
		if (bounds.position.y < margin) {
			shift.y = margin - bounds.position.y;
		} else if (bounds.get_end().y > get_size().height - margin) {
			shift.y = get_size().height - margin - bounds.get_end().y;
		}
		center += shift;
	}
}

int EditorPieMenu::_item_towards(const Point2 &p_point) const {
	for (int i = 0; i < DIRECTION_MAX; i++) {
		if (used[i] && item_rects[i].has_point(p_point)) {
			return i;
		}
	}
	const Vector2 towards = p_point - center;
	if (towards.length() < PIE_DEAD_ZONE * EDSCALE) {
		return -1;
	}
	// The choice nearest in angle: the direction of the move is what counts,
	// not how far it went.
	int best = -1;
	real_t best_dot = -2;
	const Vector2 normal = towards.normalized();
	for (int i = 0; i < DIRECTION_MAX; i++) {
		if (!used[i]) {
			continue;
		}
		const real_t dot = normal.dot(_direction_vector(i).normalized());
		if (dot > best_dot) {
			best_dot = dot;
			best = i;
		}
	}
	return best;
}

void EditorPieMenu::hover_towards(const Point2 &p_point) {
	const int now = _item_towards(p_point);
	if ((p_point - center).length() >= PIE_DEAD_ZONE * EDSCALE) {
		moved = true;
	}
	if (now != hovered) {
		hovered = now;
		queue_redraw();
	}
}

void EditorPieMenu::choose_hovered() {
	if (hovered >= 0) {
		_choose(hovered);
	}
}

void EditorPieMenu::_choose(int p_direction) {
	ERR_FAIL_INDEX(p_direction, DIRECTION_MAX);
	const Callable action = items[p_direction].action;
	close();
	if (action.is_valid()) {
		action.call();
	}
}

void EditorPieMenu::open(const Point2 &p_at, Key p_key) {
	center = p_at;
	held_key = p_key;
	hovered = -1;
	moved = false;
	opened_at = OS::get_singleton()->get_ticks_msec();
	show();
	_layout();
	grab_focus();
	queue_redraw();
}

void EditorPieMenu::close() {
	if (!is_visible()) {
		return;
	}
	held_key = Key::NONE;
	hovered = -1;
	hide();
	emit_signal(SNAME("closed"));
}

void EditorPieMenu::gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		hover_towards(motion->get_position());
		accept_event();
		return;
	}

	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_valid()) {
		if (button->is_pressed()) {
			if (button->get_button_index() == MouseButton::LEFT) {
				hover_towards(button->get_position());
				if (hovered >= 0) {
					_choose(hovered);
				} else {
					// A click on the middle, or away from every choice: nothing.
					close();
				}
			} else if (button->get_button_index() == MouseButton::RIGHT) {
				close();
			}
		}
		accept_event();
		return;
	}

	const Ref<InputEventKey> key = p_event;
	if (key.is_null()) {
		return;
	}
	accept_event();
	if (!key->is_pressed()) {
		// Letting go of the key that opened it, having moved towards a choice,
		// chooses it; a tap leaves it open to choose from.
		if (held_key != Key::NONE && key->get_keycode() == held_key) {
			if (moved && hovered >= 0) {
				_choose(hovered);
			} else {
				held_key = Key::NONE;
			}
		}
		return;
	}
	if (key->is_echo()) {
		return;
	}
	const Key code = key->get_keycode();
	if (code == Key::ESCAPE) {
		close();
		return;
	}
	if (code == Key::ENTER || code == Key::KP_ENTER) {
		choose_hovered();
		return;
	}
	// The digits point the way the numpad does.
	static const struct {
		Key digit;
		Key keypad;
		Direction direction;
	} digits[] = {
		{ Key::KEY_4, Key::KP_4, DIRECTION_LEFT },
		{ Key::KEY_6, Key::KP_6, DIRECTION_RIGHT },
		{ Key::KEY_2, Key::KP_2, DIRECTION_BOTTOM },
		{ Key::KEY_8, Key::KP_8, DIRECTION_TOP },
		{ Key::KEY_7, Key::KP_7, DIRECTION_TOP_LEFT },
		{ Key::KEY_9, Key::KP_9, DIRECTION_TOP_RIGHT },
		{ Key::KEY_1, Key::KP_1, DIRECTION_BOTTOM_LEFT },
		{ Key::KEY_3, Key::KP_3, DIRECTION_BOTTOM_RIGHT },
	};
	const Key physical = key->get_physical_keycode();
	for (const auto &digit : digits) {
		if ((code == digit.digit || code == digit.keypad || physical == digit.keypad) && used[digit.direction]) {
			_choose(digit.direction);
			return;
		}
	}
	// Its own key again, once let go of: the way out that was the way in.
	if (held_key == Key::NONE && OS::get_singleton()->get_ticks_msec() - opened_at > 100) {
		close();
	}
}

void EditorPieMenu::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_FOCUS_EXIT: {
			// Clicked somewhere else, or another window came up.
			close();
		} break;

		case NOTIFICATION_RESIZED: {
			if (is_visible()) {
				_layout();
				queue_redraw();
			}
		} break;

		case NOTIFICATION_DRAW: {
			const Color base = get_theme_color(SNAME("base_color"), EditorStringName(Editor));
			const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			const Color font_color = get_theme_color(SceneStringName(font_color), SNAME("Button"));
			const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Button"));
			const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Button"));

			// The middle, and which way the mouse points from it.
			const real_t ring = PIE_RING * EDSCALE;
			draw_circle(center, ring, Color(base, 0.85));
			draw_arc(center, ring, 0, Math::TAU, 32, Color(font_color, 0.25), Math::round(EDSCALE), true);
			if (hovered >= 0) {
				const real_t angle = _direction_vector(hovered).angle();
				draw_arc(center, ring - 2 * EDSCALE, angle - 0.5, angle + 0.5, 12, accent, Math::round(3 * EDSCALE), true);
			}
			if (!title.is_empty() && font.is_valid()) {
				const Vector2 size = font->get_string_size(title, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
				const Point2 at = center + Vector2(-size.width * 0.5, ring + 6 * EDSCALE + font->get_ascent(font_size));
				draw_string_outline(font, at, title, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Math::round(3 * EDSCALE), Color(0, 0, 0, 0.6));
				draw_string(font, at, title, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(font_color, 0.8));
			}

			Ref<StyleBoxFlat> style;
			style.instantiate();
			style->set_corner_radius_all(Math::round(PIE_ITEM_HEIGHT * 0.5 * EDSCALE));
			style->set_border_width_all(Math::round(EDSCALE));
			style->set_shadow_color(Color(0, 0, 0, 0.35));
			style->set_shadow_size(Math::round(4 * EDSCALE));
			for (int i = 0; i < DIRECTION_MAX; i++) {
				if (!used[i]) {
					continue;
				}
				const bool is_hovered = i == hovered;
				style->set_bg_color(is_hovered ? accent.darkened(0.25) : Color(base, 0.96));
				style->set_border_color(items[i].current ? accent : Color(font_color, 0.18));
				draw_style_box(style, item_rects[i]);

				real_t x = item_rects[i].position.x + 12 * EDSCALE;
				if (items[i].icon.is_valid()) {
					const Size2 icon_size = items[i].icon->get_size();
					draw_texture(items[i].icon, Point2(x, item_rects[i].get_center().y - icon_size.height * 0.5));
					x += icon_size.width + 6 * EDSCALE;
				}
				if (font.is_valid()) {
					const real_t baseline = item_rects[i].get_center().y + (font->get_ascent(font_size) - font->get_descent(font_size)) * 0.5;
					draw_string(font, Point2(x, baseline), items[i].text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, is_hovered ? Color(1, 1, 1) : font_color);
				}
			}
		} break;
	}
}

void EditorPieMenu::_bind_methods() {
	ADD_SIGNAL(MethodInfo("closed"));
}

EditorPieMenu::EditorPieMenu() {
	set_name("PieMenu");
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
	hide();
}
