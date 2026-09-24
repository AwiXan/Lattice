/**************************************************************************/
/*  editor_view_sidebar.cpp                                               */
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

#include "editor_view_sidebar.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "scene/main/scene_tree.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/animation/tween.h"
#include "scene/gui/tab_bar.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"

static constexpr real_t SIDEBAR_WIDTH = 280;
static constexpr real_t SIDEBAR_MARGIN = 8;

int EditorViewSidebar::add_page(const String &p_title, Control *p_page) {
	ERR_FAIL_NULL_V(p_page, -1);
	pages.push_back(p_page);
	tabs->add_tab(p_title);
	p_page->set_h_size_flags(SIZE_EXPAND_FILL);
	p_page->set_visible(pages.size() == 1);
	page_box->add_child(p_page);
	// A page growing or shrinking - a section shown, a row added - resizes
	// the card with it.
	p_page->connect(SNAME("minimum_size_changed"), callable_mp(this, &EditorViewSidebar::fit));
	fit();
	return pages.size() - 1;
}

Control *EditorViewSidebar::get_page(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)pages.size(), nullptr);
	return pages[p_index];
}

int EditorViewSidebar::get_current_page() const {
	return tabs->get_current_tab();
}

void EditorViewSidebar::_tab_changed(int p_tab) {
	for (uint32_t i = 0; i < pages.size(); i++) {
		pages[i]->set_visible((int)i == p_tab);
	}
	scroll->set_v_scroll(0);
	fit();
	if (is_visible()) {
		emit_signal(SNAME("page_shown"), p_tab);
	}
}

void EditorViewSidebar::show_page(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)pages.size());
	if (tabs->get_current_tab() != p_index) {
		tabs->set_current_tab(p_index);
	}
	open();
}

void EditorViewSidebar::toggle_page(int p_index) {
	if (is_open() && get_current_page() == p_index) {
		close();
		return;
	}
	show_page(p_index);
}

void EditorViewSidebar::toggle() {
	if (is_open()) {
		close();
	} else {
		open();
	}
}

void EditorViewSidebar::_set_slide(real_t p_slide) {
	slide = p_slide;
	set_modulate(Color(1, 1, 1, 0.25 + 0.75 * slide));
	fit();
}

void EditorViewSidebar::_slide_to(real_t p_to, real_t p_seconds) {
	if (slide_tween.is_valid()) {
		slide_tween->kill();
	}
	slide_tween = create_tween();
	slide_tween->set_ease(Tween::EASE_OUT);
	slide_tween->set_trans(Tween::TRANS_CUBIC);
	slide_tween->tween_method(callable_mp(this, &EditorViewSidebar::_set_slide), slide, p_to, p_seconds);
	if (p_to <= 0.0) {
		slide_tween->tween_callback(callable_mp(this, &EditorViewSidebar::_slid_out));
	}
}

void EditorViewSidebar::open() {
	closing = false;
	if (!is_visible()) {
		// In from the right edge of the view, as a drawer would.
		slide = 0.0;
		show();
	}
	_slide_to(1.0, 0.16);
}

void EditorViewSidebar::close() {
	if (!is_visible()) {
		return;
	}
	closing = true;
	_slide_to(0.0, 0.12);
}

void EditorViewSidebar::_slid_out() {
	closing = false;
	hide();
	slide = 1.0;
	set_modulate(Color(1, 1, 1));
}

void EditorViewSidebar::finish_slide() {
	if (slide_tween.is_valid()) {
		slide_tween->kill();
		slide_tween.unref();
	}
	if (closing) {
		_slid_out();
	} else {
		_set_slide(1.0);
	}
}

void EditorViewSidebar::_parent_resized() {
	fit();
}

void EditorViewSidebar::set_top_inset(real_t p_inset) {
	if (Math::is_equal_approx(top_inset, p_inset)) {
		return;
	}
	top_inset = p_inset;
	fit();
}

void EditorViewSidebar::_queued_fit() {
	fit_queued = false;
	fit();
}

void EditorViewSidebar::fit() {
	Control *parent = Object::cast_to<Control>(get_parent());
	if (!parent || fitting) {
		return;
	}
	const uint64_t frame = Engine::get_singleton()->get_process_frames();
	if (frame != fit_frame) {
		fit_frame = frame;
		fits_this_frame = 0;
	}
	if (++fits_this_frame > 8) {
		if (is_inside_tree() && !get_tree()->is_connected(SNAME("process_frame"), callable_mp(this, &EditorViewSidebar::fit))) {
			get_tree()->connect(SNAME("process_frame"), callable_mp(this, &EditorViewSidebar::fit), CONNECT_ONE_SHOT | CONNECT_DEFERRED);
		}
		return;
	}
	fitting = true;
	const real_t margin = SIDEBAR_MARGIN * EDSCALE;
	const Size2 room = parent->get_size() - Size2(margin, margin) * 2 - Size2(0, top_inset);

	// As tall as the tabs and the page shown, and no taller than the view.
	real_t content = tabs->get_combined_minimum_size().height + page_box->get_combined_minimum_size().height;
	content += Object::cast_to<Control>(tabs->get_parent())->get_theme_constant(SNAME("separation"), SNAME("VBoxContainer"));
	const Ref<StyleBox> style = get_theme_stylebox(SceneStringName(panel));
	if (style.is_valid()) {
		content += style->get_minimum_size().height;
	}
	const real_t width = MIN(MAX(SIDEBAR_WIDTH * EDSCALE, get_combined_minimum_size().width), MAX(room.width, 0));
	const real_t height = MIN(content, MAX(room.height, 0));

	// Held by its top right corner: should it have to be wider than it was
	// told, it grows into the view rather than out of it.
	set_anchor(SIDE_LEFT, ANCHOR_END);
	set_anchor(SIDE_RIGHT, ANCHOR_END);
	set_anchor(SIDE_TOP, ANCHOR_BEGIN);
	set_anchor(SIDE_BOTTOM, ANCHOR_BEGIN);
	set_h_grow_direction(GROW_DIRECTION_BEGIN);
	// Out to the right by as much as it has yet to slide in.
	const real_t away = (1.0 - slide) * (width + margin);
	set_offset(SIDE_RIGHT, -margin + away);
	set_offset(SIDE_LEFT, -margin - width + away);
	set_offset(SIDE_TOP, top_inset + margin);
	set_offset(SIDE_BOTTOM, top_inset + margin + height);
	fitting = false;
	emit_signal(SNAME("fitted"));
}

void EditorViewSidebar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Control *parent = Object::cast_to<Control>(get_parent());
			if (parent && !parent->is_connected(SceneStringName(resized), callable_mp(this, &EditorViewSidebar::_parent_resized))) {
				parent->connect(SceneStringName(resized), callable_mp(this, &EditorViewSidebar::_parent_resized));
			}
		} break;

		case NOTIFICATION_RESIZED: {
			// Grown by its own minimum settling later than its page's - a font
			// arriving, a label getting its text: put back in its corner, and
			// whatever keeps clear of it told.
			if (!fitting && !fit_queued) {
				fit_queued = true;
				callable_mp(this, &EditorViewSidebar::_queued_fit).call_deferred();
			}
		} break;

		case NOTIFICATION_UNPARENTED: {
			// The old parent is gone from get_parent() by now; its connection
			// goes with this object when it is freed.
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			// Setting the style is itself a theme change; once is enough.
			if (!theming) {
				theming = true;
				Ref<StyleBoxFlat> style;
				style.instantiate();
				const Color base = get_theme_color(SNAME("base_color"), EditorStringName(Editor));
				style->set_bg_color(Color(base, 0.97));
				style->set_border_color(get_theme_color(SNAME("dark_color_3"), EditorStringName(Editor)));
				style->set_border_width_all(Math::round(EDSCALE));
				style->set_corner_radius_all(Math::round(6 * EDSCALE));
				style->set_content_margin_all(Math::round(6 * EDSCALE));
				style->set_shadow_color(Color(0, 0, 0, 0.3));
				style->set_shadow_size(Math::round(4 * EDSCALE));
				add_theme_style_override(SceneStringName(panel), style);
				theming = false;
			}
			fit();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				fit();
				emit_signal(SNAME("page_shown"), get_current_page());
			} else {
				emit_signal(SNAME("fitted"));
			}
		} break;
	}
}

void EditorViewSidebar::_bind_methods() {
	// Placed or sized again, shown or hidden: whatever keeps clear of it - the
	// view's own controls in the corner - moves with it.
	ADD_SIGNAL(MethodInfo("fitted"));
	// A page coming into sight - the card opened on it, or its tab chosen -
	// for whatever it shows to be brought up to date.
	ADD_SIGNAL(MethodInfo("page_shown", PropertyInfo(Variant::INT, "page")));
}

EditorViewSidebar::EditorViewSidebar() {
	set_name("Sidebar");
	set_mouse_filter(MOUSE_FILTER_STOP);
	hide();

	VBoxContainer *vbox = memnew(VBoxContainer);
	add_child(vbox);

	tabs = memnew(TabBar);
	tabs->set_clip_tabs(false);
	tabs->connect("tab_changed", callable_mp(this, &EditorViewSidebar::_tab_changed));
	vbox->add_child(tabs);

	scroll = memnew(ScrollContainer);
	scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	// Room kept for the scroll bar whether it shows or not: a page as tall as
	// the view would otherwise be narrower with it, and so taller, and so need
	// it - and then wider without it, and so shorter, and not need it.
	scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_RESERVE);
	vbox->add_child(scroll);

	page_box = memnew(VBoxContainer);
	page_box->set_h_size_flags(SIZE_EXPAND_FILL);
	scroll->add_child(page_box);
}
