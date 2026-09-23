/**************************************************************************/
/*  editor_pane.cpp                                                       */
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

#include "editor_pane.h"

#include "core/object/callable_mp.h"
#include "core/input/input_event.h"
#include "editor/editor_data.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_pane_tree.h"
#include "editor/gui/editor_panel_button.h"
#include "editor/gui/editor_panel_picker.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"

void EditorPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::BOOL, "vertical")));
	ADD_SIGNAL(MethodInfo("float_requested", PropertyInfo(Variant::INT, "panel")));
	ADD_SIGNAL(MethodInfo("close_requested"));
	ADD_SIGNAL(MethodInfo("panels_changed"));
}

void EditorPane::_build_header() {
	header_panel = memnew(PanelContainer);
	header_panel->connect(SceneStringName(draw), callable_mp(this, &EditorPane::_draw_scene_color));
	add_child(header_panel);

	header = memnew(HFlowContainer);
	header_panel->add_child(header);

	tab_bar = memnew(TabBar);
	tab_bar->set_h_size_flags(SIZE_EXPAND_FILL);
	// The bar offers arrows rather than demanding room for every tab, which is
	// what lets a pane be narrower than its tabs laid end to end.
	tab_bar->set_clip_tabs(true);
	tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ACTIVE_ONLY);
	tab_bar->connect(SNAME("tab_selected"), callable_mp(this, &EditorPane::_tab_selected));
	tab_bar->connect(SNAME("tab_close_pressed"), callable_mp(this, &EditorPane::_tab_close_pressed));
	tab_bar->connect(SceneStringName(gui_input), callable_mp(this, &EditorPane::_tab_bar_input));
	// Dragging a tab is how a panel is moved, split off or torn out. The bar
	// forwards to this pane, which is the thing that knows what a tab means.
	tab_bar->set_drag_forwarding(
			callable_mp(this, &EditorPane::_tab_get_drag_data_fw).bind(tab_bar),
			callable_mp(this, &EditorPane::_tab_can_drop_data_fw).bind(tab_bar),
			callable_mp(this, &EditorPane::_tab_drop_data_fw).bind(tab_bar));
	header->add_child(tab_bar);

	subject_button = memnew(OptionButton);
	subject_button->set_flat(true);
	subject_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	subject_button->set_tooltip_text(TTRC("The scene this panel is showing. It does not have to be the one the tab bar has selected."));
	subject_button->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorPane::_update_subject_list));
	subject_button->connect(SceneStringName(item_selected), callable_mp(this, &EditorPane::_subject_selected));
	subject_button->set_custom_minimum_size(Size2(60 * EDSCALE, 0));
	subject_button->hide();
	header->add_child(subject_button);

	header->add_child(memnew(VSeparator));

	palette = memnew(HBoxContainer);
	palette->add_theme_constant_override("separation", 0);
	header->add_child(palette);

	more_button = memnew(MenuButton);
	more_button->set_flat(true);
	more_button->set_focus_mode(FOCUS_NONE);
	more_button->set_tooltip_text(TTRC("Show something else here."));
	// Filled when opened rather than kept in step with the registry, so a type
	// registered later needs to tell nobody.
	more_button->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorPane::_update_palette));
	more_button->get_popup()->connect(SNAME("index_pressed"), callable_mp(this, &EditorPane::_more_selected));
	header->add_child(more_button);

	recent_menu = memnew(PopupMenu);
	recent_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorPane::_recent_selected));
	more_button->get_popup()->add_child(recent_menu);

	header->add_child(memnew(VSeparator));

	restore_button = memnew(Button);
	restore_button->set_flat(true);
	restore_button->set_focus_mode(FOCUS_NONE);
	restore_button->set_tooltip_text(TTRC("Show the other panes again."));
	restore_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_restore_pressed));
	restore_button->hide();
	header->add_child(restore_button);

	float_button = memnew(Button);
	float_button->set_flat(true);
	float_button->set_focus_mode(FOCUS_NONE);
	float_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_float_pressed));
	header->add_child(float_button);

	split_right_button = memnew(Button);
	split_right_button->set_flat(true);
	split_right_button->set_focus_mode(FOCUS_NONE);
	split_right_button->set_tooltip_text(TTRC("Put another pane beside this one."));
	split_right_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_split_pressed).bind(false));
	header->add_child(split_right_button);

	split_down_button = memnew(Button);
	split_down_button->set_flat(true);
	split_down_button->set_focus_mode(FOCUS_NONE);
	split_down_button->set_tooltip_text(TTRC("Put another pane below this one."));
	split_down_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_split_pressed).bind(true));
	header->add_child(split_down_button);

	close_button = memnew(Button);
	close_button->set_flat(true);
	close_button->set_focus_mode(FOCUS_NONE);
	close_button->set_tooltip_text(TTRC("Close this pane."));
	close_button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_close_pressed));
	header->add_child(close_button);
}

void EditorPane::_update_theme() {
	if (!is_inside_tree()) {
		return;
	}
	// From the editor's own base rather than from here: a pane is reparented
	// whenever the arrangement changes, and between parents it has no theme of
	// its own to ask.
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	if (!base) {
		return;
	}
	more_button->set_button_icon(base->get_editor_theme_icon(SNAME("Add")));

	const EditorPaneTree *tree = _get_pane_tree();
	const bool windowed = tree && tree->is_windowed();
	restore_button->set_button_icon(base->get_editor_theme_icon(SNAME("DistractionFree")));
	float_button->set_button_icon(base->get_editor_theme_icon(windowed ? SNAME("Back") : SNAME("MakeFloating")));
	float_button->set_tooltip_text(windowed
					? TTRC("Put this panel back in the main window.")
					: TTRC("Open this panel in a window of its own."));
	float_button->set_visible(EditorNode::get_singleton()->is_multi_window_enabled());
	split_right_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2Alt")));
	split_down_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2")));
	close_button->set_button_icon(base->get_editor_theme_icon(SNAME("Close")));

	// Its own background, a shade off the pane's, so a header reads as the
	// pane's edge rather than as part of what the pane is showing.
	Ref<StyleBoxFlat> header_style;
	header_style.instantiate();
	header_style->set_bg_color(base->get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor)));
	const int side = Math::round(2 * EDSCALE);
	header_style->set_content_margin_all(side);
	header_style->set_border_width(SIDE_BOTTOM, Math::round(1 * EDSCALE));
	header_style->set_border_color(base->get_theme_color(SNAME("dark_color_3"), EditorStringName(Editor)));
	header_panel->add_theme_style_override(SceneStringName(panel), header_style);

	_update_palette();
}

Ref<Texture2D> EditorPane::_icon_of(const StringName &p_type) const {
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_type);
	if (!type) {
		return Ref<Texture2D>();
	}
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	if (base && type->icon != StringName() && base->has_theme_icon(type->icon, EditorStringName(EditorIcons))) {
		return base->get_editor_theme_icon(type->icon);
	}
	// A plugin brings its own rather than naming one in the theme.
	return type->icon_texture;
}

void EditorPane::_update_palette() {
	if (!EditorNode::get_singleton()) {
		return;
	}

	// Three tiers, so the header stays a handful of icons however many kinds of
	// panel there are. The few a pane is most often for - the 2D and 3D views -
	// get buttons of their own, which can also be dragged somewhere. The ones
	// reached for all the time are in the "+" menu, which lists whatever the
	// interface/panes/quick_panels setting names. Everything else is one click
	// further, in a window made for looking through them.
	Vector<StringName> wanted;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		if (type && type->featured) {
			wanted.push_back(id);
		}
	}

	if (more_button) {
		PopupMenu *popup = more_button->get_popup();
		// Not freeing submenus: the recently closed one is kept and refilled.
		popup->clear(false);
		more_types.clear();
		const PackedStringArray quick = EDITOR_GET("interface/panes/quick_panels");
		for (const String &name : quick) {
			const StringName id = EditorPanelRegistry::resolve(StringName(name));
			const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
			if (!type || type->featured || more_types.has(id)) {
				// Gone, already a button of its own, or listed once already under
				// another of its names.
				continue;
			}
			popup->add_icon_item(_icon_of(id), type->title.is_empty() ? name : type->title);
			more_types.push_back(id);
		}
		if (!more_types.is_empty()) {
			popup->add_separator();
		}
		EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
		recent_menu->clear();
		const int closed = main_screen ? main_screen->get_closed_panel_count() : 0;
		// Newest first, as they are reopened.
		for (int i = closed - 1; i >= 0; i--) {
			recent_menu->add_icon_item(_icon_of(main_screen->get_closed_panel_type(i)), main_screen->get_closed_panel_title(i), i);
		}
		if (closed > 0) {
			popup->add_submenu_node_item(TTR("Recently Closed"), recent_menu);
			const Ref<Shortcut> reopen = ED_GET_SHORTCUT("editor/reopen_closed_panel");
			if (reopen.is_valid()) {
				recent_menu->set_item_shortcut(0, reopen, true);
			}
		}
		// Not a panel: the way to all the others. Its index is one past the
		// last quick panel, which is how a pick tells the two apart.
		Control *base = EditorNode::get_singleton()->get_gui_base();
		popup->add_icon_item(base->get_editor_theme_icon(SNAME("GuiTabMenuHl")), TTR("More..."));
	}

	if (wanted == palette_types && palette->get_child_count() > 0) {
		// Nothing new registered; only the icons need saying again.
		for (int i = 0; i < palette->get_child_count(); i++) {
			EditorPanelButton *button = Object::cast_to<EditorPanelButton>(palette->get_child(i));
			if (button) {
				button->set_button_icon(_icon_of(button->get_panel_type()));
			}
		}
		return;
	}

	palette_types = wanted;
	for (int i = palette->get_child_count() - 1; i >= 0; i--) {
		memdelete(palette->get_child(i));
	}

	for (const StringName &id : palette_types) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		const String title = type->title.is_empty() ? String(id) : type->title;

		EditorPanelButton *button = memnew(EditorPanelButton);
		button->set_panel_type(id);
		const Ref<Texture2D> icon = _icon_of(id);
		if (icon.is_valid()) {
			button->set_button_icon(icon);
		} else {
			button->set_text(title);
		}
		button->set_tooltip_text(vformat(TTR("Add a %s panel here, or drag it onto a pane to put one there."), title));
		button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_palette_pressed).bind(id));
		palette->add_child(button);
	}
}

void EditorPane::_more_selected(int p_index) {
	if (p_index >= 0 && p_index < more_types.size()) {
		_palette_pressed(more_types[p_index]);
		return;
	}
	// Past the quick panels and the separator: "More...".
	EditorPanelPicker::get_shared()->pick(callable_mp(this, &EditorPane::_palette_pressed));
}

void EditorPane::_palette_pressed(const StringName &p_type) {
	// Pressing is the same as dropping it here, so it goes the same way and
	// gets the same answer about what to show.
	if (show_panel_of_type(p_type) >= 0) {
		return;
	}

	// There is one of it and it is already somewhere. Asking for it here means
	// wanting it here, so it comes, rather than nothing happening at all.
	EditorPaneTree *tree = _get_pane_tree();
	if (!tree) {
		return;
	}
	for (EditorPane *pane : tree->get_panes()) {
		if (pane == this) {
			continue;
		}
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_type_at(i) == p_type) {
				pane->transfer_panel_to(this, i);
				tree->drop_empty_panes();
				return;
			}
		}
	}
}

String EditorPane::_title_of(const PanelEntry &p_entry) const {
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_entry.type);
	return type && !type->title.is_empty() ? type->title : String(p_entry.type);
}

void EditorPane::_update_tabs() {
	rebuilding_tabs = true;
	tab_bar->clear_tabs();
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	for (const PanelEntry &entry : panels) {
		tab_bar->add_tab(_title_of(entry));
		// The same icon the header offers this kind by, so a row of tabs can be
		// read at a glance rather than by their names.
		const Ref<Texture2D> icon = _icon_of(entry.type);
		if (icon.is_valid()) {
			tab_bar->set_tab_icon(tab_bar->get_tab_count() - 1, icon);
		}
	}
	if (current >= 0 && current < tab_bar->get_tab_count()) {
		tab_bar->set_current_tab(current);
	}

	const EditorPanelRegistry::PanelType *type = (current >= 0 && current < panels.size()) ? EditorPanelRegistry::get_type(panels[current].type) : nullptr;
	// Only a panel that shows one document offers a scene to choose; a resource
	// panel is pointed at its resource by whatever opened it.
	subject_button->set_visible(type && type->binding == EditorPanelRegistry::BINDING_DOCUMENT);
	header_panel->queue_redraw();
	if (subject_button->is_visible()) {
		_update_subject_list();
	}
	// A type registered after this pane was built - an addon's - belongs in the
	// header too, and this is the moment anything about the pane has changed.
	_update_palette();
	rebuilding_tabs = false;
}

void EditorPane::_show_only_current() {
	for (int i = 0; i < panels.size(); i++) {
		if (panels[i].control) {
			panels[i].control->set_visible(i == current);
		}
	}
}

static Ref<Texture2D> _scene_color_dot(const Color &p_color) {
	// A dot for the list of scenes, in the scene's color.
	const int size = MAX(8, int(10 * EDSCALE));
	Ref<Image> image = Image::create_empty(size, size, false, Image::FORMAT_RGBA8);
	const real_t radius = size * 0.5;
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			const real_t distance = Vector2(x + 0.5 - radius, y + 0.5 - radius).length();
			const real_t coverage = CLAMP(radius - distance, (real_t)0.0, (real_t)1.0);
			image->set_pixel(x, y, Color(p_color.r, p_color.g, p_color.b, coverage));
		}
	}
	return ImageTexture::create_from_image(image);
}

void EditorPane::_update_subject_list() {
	subject_button->clear();

	EditorData &editor_data = EditorNode::get_editor_data();
	const Variant subject = (current >= 0 && current < panels.size()) ? panels[current].subject : Variant();

	subject_button->add_item(TTRC("Follow the current scene"), 0);
	subject_button->set_item_metadata(0, -1);
	if (subject.get_type() != Variant::INT || (int)subject < 0) {
		subject_button->select(0);
	}

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		const int history_id = editor_data.get_scene_history_id(i);
		String title = editor_data.get_scene_title(i);
		if (title.is_empty()) {
			title = TTR("[unsaved]");
		}
		const int index = subject_button->get_item_count();
		subject_button->add_item(title, index);
		subject_button->set_item_metadata(index, history_id);
		if (editor_data.are_scene_colors_shown()) {
			subject_button->set_item_icon(index, _scene_color_dot(editor_data.get_scene_color(i)));
		}
		if (subject.get_type() == Variant::INT && (int)subject == history_id) {
			subject_button->select(index);
		}
	}
}

void EditorPane::_tab_selected(int p_index) {
	if (rebuilding_tabs) {
		return;
	}
	set_current_panel(p_index);
}

Color EditorPane::_scene_color() const {
	if (current < 0 || current >= panels.size()) {
		return Color(0, 0, 0, 0);
	}
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(panels[current].type);
	EditorData &editor_data = EditorNode::get_editor_data();
	if (!type || type->binding != EditorPanelRegistry::BINDING_DOCUMENT || !editor_data.are_scene_colors_shown()) {
		return Color(0, 0, 0, 0);
	}
	// Pointed at a scene, or following whichever is current - and a scene
	// closed since means following too.
	const Variant subject = panels[current].subject;
	int index = subject.get_type() == Variant::INT && int(subject) >= 0 ? editor_data.get_scene_index_by_history_id(subject) : -1;
	if (index < 0) {
		index = editor_data.get_edited_scene();
	}
	return index >= 0 ? editor_data.get_scene_color(index) : Color(0, 0, 0, 0);
}

void EditorPane::_draw_scene_color() {
	const Color color = _scene_color();
	if (color.a <= 0) {
		return;
	}
	header_panel->draw_rect(Rect2(0, 0, header_panel->get_size().x, Math::round(2 * EDSCALE)), color);
}

void EditorPane::_scene_changed() {
	header_panel->queue_redraw();
}

void EditorPane::_note_closing(int p_index) {
	// Only closing by hand is noted: a layout being loaded or a pane being
	// merged away is not something anyone wants back.
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	if (main_screen) {
		main_screen->note_panel_closing(this, p_index);
	}
}

void EditorPane::_tab_close_pressed(int p_index) {
	_note_closing(p_index);
	close_panel(p_index);
}

void EditorPane::_tab_bar_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_null() || !mb->is_pressed()) {
		return;
	}
	const int tab = tab_bar->get_tab_idx_at_point(mb->get_position());
	if (tab < 0) {
		return;
	}
	if (mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
		// A tab double-clicked is this pane over all the others, and back.
		EditorPaneTree *tree = _get_pane_tree();
		if (tree) {
			tree->toggle_maximized(this);
			tab_bar->accept_event();
		}
	} else if (mb->get_button_index() == MouseButton::MIDDLE) {
		// What a middle click on a tab does everywhere else.
		_tab_close_pressed(tab);
		tab_bar->accept_event();
	}
}

void EditorPane::_restore_pressed() {
	EditorPaneTree *tree = _get_pane_tree();
	if (tree) {
		tree->set_maximized_pane(nullptr);
	}
}

void EditorPane::_recent_selected(int p_index) {
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	if (main_screen) {
		main_screen->reopen_closed_panel(p_index);
	}
}

void EditorPane::set_maximized(bool p_maximized) {
	restore_button->set_visible(p_maximized);
}

void EditorPane::_subject_selected(int p_index) {
	set_panel_subject(subject_button->get_item_metadata(p_index));
}

void EditorPane::_split_pressed(bool p_vertical) {
	emit_signal(SNAME("split_requested"), p_vertical);
}

void EditorPane::_float_pressed() {
	// The same button both ways: out of the main window, or back into it. Which
	// one it is depends on where this pane already is.
	emit_signal(SNAME("float_requested"), current);
}

void EditorPane::_close_pressed() {
	// Last first, so that reopening brings the first tab back first.
	for (int i = panels.size() - 1; i >= 0; i--) {
		_note_closing(i);
	}
	emit_signal(SNAME("close_requested"));
}

int EditorPane::add_panel(const StringName &p_type, const Variant &p_subject) {
	Control *control = EditorPanelRegistry::create_panel(p_type);
	if (!control) {
		// The type refused - a plugin that cannot be shown twice says so by
		// handing back nothing.
		return -1;
	}

	PanelEntry entry;
	// By the id it answers to now, so that an old name and a new one for the
	// same thing are seen to be the same panel.
	entry.type = EditorPanelRegistry::resolve(p_type);
	entry.subject = p_subject;
	entry.control = control;
	control->set_v_size_flags(SIZE_EXPAND_FILL);
	control->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(control);
	EditorPanelRegistry::bind_panel(p_type, control, p_subject);

	panels.push_back(entry);
	current = panels.size() - 1;
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
	return current;
}

void EditorPane::close_panel(int p_index) {
	ERR_FAIL_INDEX(p_index, panels.size());
	_let_go_of(panels[p_index]);
	panels.remove_at(p_index);

	if (panels.is_empty()) {
		current = -1;
	} else {
		current = CLAMP(current >= p_index ? current - 1 : current, 0, panels.size() - 1);
	}
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
}

void EditorPane::set_current_panel(int p_index) {
	if (p_index < 0 || p_index >= panels.size() || p_index == current) {
		return;
	}
	EditorPaneTree *tree = _get_pane_tree();
	if (tree) {
		// Choosing something here is working here.
		tree->set_active_pane(this);
	}
	current = p_index;
	_show_only_current();
	_update_tabs();
}

bool EditorPane::show_panel(Control *p_panel) {
	for (int i = 0; i < panels.size(); i++) {
		if (panels[i].control == p_panel) {
			set_current_panel(i);
			return true;
		}
	}
	return false;
}

StringName EditorPane::get_panel_type_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return StringName();
	}
	return panels[p_index].type;
}

Variant EditorPane::get_panel_subject_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return Variant();
	}
	return panels[p_index].subject;
}

Control *EditorPane::get_panel_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return nullptr;
	}
	return panels[p_index].control;
}

String EditorPane::get_panel_title_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return String();
	}
	return _title_of(panels[p_index]);
}

void EditorPane::set_panel_type(const StringName &p_type, const Variant &p_subject) {
	for (int i = panels.size() - 1; i >= 0; i--) {
		_let_go_of(panels[i]);
		panels.remove_at(i);
	}
	current = -1;
	add_panel(p_type, p_subject);
}

int EditorPane::show_panel_of_type(const StringName &p_type) {
	const StringName type = EditorPanelRegistry::resolve(p_type);
	for (int i = 0; i < panels.size(); i++) {
		if (panels[i].type == type) {
			set_current_panel(i);
			return i;
		}
	}
	return add_panel(p_type, _subject_for_type(p_type));
}

void EditorPane::set_panel_subject(const Variant &p_subject) {
	if (current < 0 || current >= panels.size()) {
		return;
	}
	panels.write[current].subject = p_subject;
	EditorPanelRegistry::bind_panel(panels[current].type, panels[current].control, p_subject);
}

void EditorPane::_let_go_of(const PanelEntry &p_entry) {
	if (!p_entry.control) {
		return;
	}
	// A lent panel goes home; one this pane had built is this pane's to free.
	// Either way it stops being a child first, so that a lender putting it
	// somewhere else is not fighting this pane for it.
	const bool given_back = EditorPanelRegistry::release_panel(p_entry.type, p_entry.control);
	if (p_entry.control->get_parent() == this) {
		remove_child(p_entry.control);
	}
	if (!given_back) {
		memdelete(p_entry.control);
	}
}

void EditorPane::_return_everything_lent() {
	// Whatever was lent to this pane goes back rather than down with it: the
	// editor has one FileSystem and one script editor, and a pane closing is
	// not a reason to lose either.
	for (const PanelEntry &entry : panels) {
		if (!entry.control) {
			continue;
		}
		if (EditorPanelRegistry::release_panel(entry.type, entry.control)) {
			if (entry.control->get_parent() == this) {
				remove_child(entry.control);
			}
		}
	}
	panels.clear();
}

EditorPaneTree *EditorPane::_get_pane_tree() const {
	for (Node *n = get_parent(); n; n = n->get_parent()) {
		EditorPaneTree *tree = Object::cast_to<EditorPaneTree>(n);
		if (tree) {
			return tree;
		}
	}
	return nullptr;
}

StringName EditorPane::_type_for_subject(const Variant &p_subject) const {
	// A history id names a document, a path names a resource. Nothing else is
	// offered yet, and a binding added later decides the same way.
	const EditorPanelRegistry::Binding binding = p_subject.get_type() == Variant::INT
			? EditorPanelRegistry::BINDING_DOCUMENT
			: EditorPanelRegistry::BINDING_RESOURCE;

	// The kind of panel already here, so dropping a scene on a pane showing a 2D
	// view gives a 2D view of it rather than something else.
	const EditorPanelRegistry::PanelType *here = (current >= 0 && current < panels.size())
			? EditorPanelRegistry::get_type(panels[current].type)
			: nullptr;
	if (here && here->binding == binding) {
		return here->id;
	}
	// Failing that, the kind last worked in.
	return EditorPanelRegistry::get_default_type_for(binding);
}

Variant EditorPane::_subject_for_type(const StringName &p_type) const {
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_type);
	if (!type || !EditorPanelRegistry::binding_takes_subject(type->binding)) {
		return Variant();
	}

	// What this pane is already showing, so a view added to a pane on one scene
	// is another view of that scene rather than of whatever is current.
	const Variant here = get_panel_subject();
	if (type->binding == EditorPanelRegistry::BINDING_DOCUMENT) {
		return here.get_type() == Variant::INT ? here : Variant(-1);
	}
	return here.get_type() == Variant::STRING ? here : Variant(String());
}

EditorPane::PanelDrop EditorPane::_read_drop(const Variant &p_data) const {
	PanelDrop drop;
	if (p_data.get_type() != Variant::DICTIONARY) {
		return drop;
	}
	const Dictionary data = p_data;

	if (String(data.get("type", "")) == "editor_pane_panel") {
		EditorPane *pane = ObjectDB::get_instance<EditorPane>(ObjectID((uint64_t)(int64_t)data.get("pane", 0)));
		const int index = data.get("index", -1);
		if (pane && index >= 0 && index < pane->get_panel_count()) {
			drop.source = pane;
			drop.source_index = index;
		}
		return drop;
	}

	if (!data.has("editor_panel") && !data.has("editor_panel_subject")) {
		// Something else the editor drags about: a node, a file, a colour.
		return drop;
	}
	const String named = data.get("editor_panel", String());
	if (data.has("editor_panel_subject")) {
		drop.subject = data.get("editor_panel_subject", Variant());
		drop.type = named.is_empty() ? _type_for_subject(drop.subject) : StringName(named);
	} else {
		// A kind of panel and nothing to point it at - one of the buttons in a
		// pane's header. Where it lands decides what it shows.
		drop.type = StringName(named);
		drop.subject = _subject_for_type(drop.type);
	}
	if (!EditorPanelRegistry::has_type(drop.type)) {
		// Nothing registered can show it, so there is nothing to make.
		drop.type = StringName();
	}
	return drop;
}

Rect2 EditorPane::get_body_rect() const {
	Rect2 body(Point2(), get_size());
	if (header_panel && header_panel->is_visible()) {
		// Where the header actually ends, rather than its height plus whatever
		// the separation happens to be.
		const real_t taken = header_panel->get_rect().get_end().y;
		body.position.y = taken;
		body.size.y = MAX(0.0, get_size().y - taken);
	}
	return body;
}

bool EditorPane::is_point_on_header(const Point2 &p_point) const {
	return header_panel && header_panel->is_visible() && header_panel->get_rect().has_point(p_point);
}

EditorPane::DropZone EditorPane::get_drop_zone_at(const Point2 &p_point, DropZone p_current) const {
	// Over the tabs is always "join these", whatever part of the bar it is.
	if (is_point_on_header(p_point)) {
		return DROP_INTO;
	}

	const Rect2 body = get_body_rect();
	if (body.size.x <= 0 || body.size.y <= 0) {
		return DROP_INTO;
	}
	const Point2 at = p_point - body.position;

	// Nearly a third of each side, up to a band wide enough to aim at without
	// care, so that a large pane does not become mostly edge and a small one is
	// still worth aiming at.
	const real_t band_x = MIN(body.size.x * 0.3, 180.0 * EDSCALE);
	const real_t band_y = MIN(body.size.y * 0.3, 180.0 * EDSCALE);

	// Whichever edge is nearest, if any is near enough.
	const real_t left = at.x;
	const real_t right = body.size.x - at.x;
	const real_t top = at.y;
	const real_t bottom = body.size.y - at.y;

	const real_t best = MIN(MIN(left, right), MIN(top, bottom));
	DropZone nearest = DROP_BOTTOM;
	real_t band = band_y;
	if (best == left) {
		nearest = DROP_LEFT;
		band = band_x;
	} else if (best == right) {
		nearest = DROP_RIGHT;
		band = band_x;
	} else if (best == top) {
		nearest = DROP_TOP;
	}

	// Staying put is favored by a margin, so a hand that is not perfectly still
	// on a boundary does not make the hint jump back and forth under it.
	const real_t margin = 16 * EDSCALE;
	switch (p_current) {
		case DROP_LEFT:
		case DROP_RIGHT:
		case DROP_TOP:
		case DROP_BOTTOM: {
			real_t distance = bottom;
			real_t current_band = band_y;
			if (p_current == DROP_LEFT) {
				distance = left;
				current_band = band_x;
			} else if (p_current == DROP_RIGHT) {
				distance = right;
				current_band = band_x;
			} else if (p_current == DROP_TOP) {
				distance = top;
			}
			if (distance < current_band + margin && distance <= best + margin) {
				return p_current;
			}
		} break;
		case DROP_INTO: {
			// Out of the middle only once well inside an edge's band.
			return best < MAX(band - margin, band * 0.5) ? nearest : DROP_INTO;
		}
		default: {
		} break;
	}
	return best < band ? nearest : DROP_INTO;
}

Rect2 EditorPane::get_compass_target_rect(DropZone p_zone) const {
	const Rect2 body = get_body_rect();
	const real_t size = 34 * EDSCALE;
	const real_t gap = 6 * EDSCALE;
	const real_t room = 3 * size + 2 * gap + 32 * EDSCALE;
	if (body.size.x < room || body.size.y < room) {
		return Rect2();
	}
	const Point2 center = body.get_center();
	Point2 offset;
	switch (p_zone) {
		case DROP_INTO:
			break;
		case DROP_LEFT:
			offset.x = -(size + gap);
			break;
		case DROP_RIGHT:
			offset.x = size + gap;
			break;
		case DROP_TOP:
			offset.y = -(size + gap);
			break;
		case DROP_BOTTOM:
			offset.y = size + gap;
			break;
		default:
			return Rect2();
	}
	return Rect2(center + offset - Vector2(size, size) * 0.5, Vector2(size, size));
}

EditorPane::DropZone EditorPane::get_compass_zone_at(const Point2 &p_point) const {
	const DropZone zones[] = { DROP_INTO, DROP_LEFT, DROP_RIGHT, DROP_TOP, DROP_BOTTOM };
	for (const DropZone zone : zones) {
		const Rect2 rect = get_compass_target_rect(zone);
		if (rect.has_area() && rect.has_point(p_point)) {
			return zone;
		}
	}
	return DROP_NONE;
}

bool EditorPane::is_panel_drag(const Variant &p_data) {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	const Dictionary data = p_data;
	return String(data.get("type", "")) == "editor_pane_panel" || data.has("editor_panel") || data.has("editor_panel_subject");
}

bool EditorPane::can_accept_drop(const Point2 &p_point, const Variant &p_data) const {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return false;
	}
	// Putting a pane's panel back into the pane it is already in changes
	// nothing - unless it is being put somewhere else in the order.
	DropZone zone = get_compass_zone_at(p_point);
	if (zone == DROP_NONE) {
		zone = get_drop_zone_at(p_point);
	}
	if (drop.source == this && zone == DROP_INTO && !is_point_on_header(p_point)) {
		return false;
	}
	return true;
}

bool EditorPane::accept_drop(const Point2 &p_point, const Variant &p_data, DropZone p_zone) {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return false;
	}

	if (is_point_on_header(p_point)) {
		// Where along the bar it was let go, so a tab can be put in order
		// rather than only appended.
		const Point2 in_bar = p_point - header_panel->get_position() - header->get_position() - tab_bar->get_position();
		return _accept_drop(drop, DROP_INTO, _tab_insert_index_at(in_bar));
	}
	return _accept_drop(drop, p_zone != DROP_NONE ? p_zone : get_drop_zone_at(p_point), -1);
}

int EditorPane::_tab_insert_index_at(const Point2 &p_in_bar) const {
	// The same sum TabBar::_draw_tab_drop() does to place its mark: before the
	// nearest tab, or after it once past its middle. Taking the tab under the
	// pointer instead put a tab before one the mark said it would follow.
	const int closest = tab_bar->get_closest_tab_idx_to_point(p_in_bar);
	if (closest < 0) {
		return panels.size();
	}
	const Rect2 rect = tab_bar->get_tab_rect(closest);
	return p_in_bar.x > rect.get_center().x ? closest + 1 : closest;
}

Variant EditorPane::_tab_get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	const int index = tab_bar->get_tab_idx_at_point(p_point);
	if (index < 0 || index >= panels.size()) {
		return Variant();
	}

	Dictionary data;
	data["type"] = "editor_pane_panel";
	data["pane"] = (int64_t)get_instance_id();
	data["index"] = index;

	set_drag_preview(_make_drag_preview(index));
	return data;
}

Control *EditorPane::_make_drag_preview(int p_index) const {
	// What is being carried, recognizably: its tab, and a glimpse of what it
	// shows as it was when picked up.
	const PanelEntry &entry = panels[p_index];

	// Kept clear of the pointer, which would otherwise sit on the title.
	Control *root = memnew(Control);
	root->set_mouse_filter(MOUSE_FILTER_IGNORE);
	PanelContainer *card = memnew(PanelContainer);
	card->set_theme_type_variation("TooltipPanel");
	card->set_position(Vector2(12, 12) * EDSCALE);
	card->set_modulate(Color(1, 1, 1, 0.9));
	root->add_child(card);

	VBoxContainer *box = memnew(VBoxContainer);
	card->add_child(box);
	HBoxContainer *title_row = memnew(HBoxContainer);
	box->add_child(title_row);
	const Ref<Texture2D> icon = _icon_of(entry.type);
	if (icon.is_valid()) {
		TextureRect *icon_rect = memnew(TextureRect);
		icon_rect->set_texture(icon);
		icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		title_row->add_child(icon_rect);
	}
	Label *label = memnew(Label);
	label->set_text(_title_of(entry));
	title_row->add_child(label);

	// Only a panel on screen can be looked at, and only a renderer that draws
	// has anything to show.
	Viewport *viewport = get_viewport();
	if (entry.control && entry.control->is_visible_in_tree() && viewport && viewport->get_texture().is_valid()) {
		const Ref<Image> frame = viewport->get_texture()->get_image();
		if (frame.is_valid() && !frame->is_empty()) {
			const Rect2i region = Rect2i(viewport->get_final_transform().xform(entry.control->get_global_rect())).intersection(Rect2i(Point2i(), frame->get_size()));
			if (region.size.x > 8 && region.size.y > 8) {
				Ref<Image> glimpse = frame->get_region(region);
				const real_t width = 220 * EDSCALE;
				const real_t scale = MIN((real_t)1.0, width / region.size.x);
				glimpse->resize(MAX(1, int(region.size.x * scale)), MAX(1, int(region.size.y * scale)), Image::INTERPOLATE_BILINEAR);
				TextureRect *picture = memnew(TextureRect);
				picture->set_texture(ImageTexture::create_from_image(glimpse));
				box->add_child(picture);
			}
		}
	}
	return root;
}

bool EditorPane::open_resource(const String &p_path) {
	const StringName type = EditorPanelRegistry::find_type_for_resource(p_path);
	if (type == StringName()) {
		return false;
	}

	const int at = show_panel_of_type(type);
	if (at < 0) {
		return false;
	}
	// A type that keeps its own tabs opens the file in them; anything else is
	// simply pointed at it.
	if (!EditorPanelRegistry::open_resource(type, get_panel_at(at), p_path)) {
		set_current_panel(at);
		set_panel_subject(p_path);
	}
	return true;
}

String EditorPane::first_openable_file(const Variant &p_data) {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return String();
	}
	const Dictionary data = p_data;
	const String kind = data.get("type", "");
	if (kind != "files" && kind != "files_and_dirs") {
		return String();
	}
	const Vector<String> files = data.get("files", Vector<String>());
	for (const String &file : files) {
		if (EditorPanelRegistry::find_type_for_resource(file) != StringName()) {
			return file;
		}
	}
	return String();
}

bool EditorPane::_tab_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	// A file let go on the tabs means "show it here". Only on the tabs: over
	// the body a file still means whatever the panel wants of it, so dropping a
	// scene on a 3D view still puts it in the scene.
	if (!first_openable_file(p_data).is_empty()) {
		return true;
	}

	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return false;
	}
	// Dropping a tab back where it came from changes nothing.
	return !(drop.source == this && tab_bar->get_tab_idx_at_point(p_point) == drop.source_index);
}

void EditorPane::_tab_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	const String file = first_openable_file(p_data);
	if (!file.is_empty()) {
		open_resource(file);
		return;
	}

	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return;
	}
	// Where along the bar it was let go, so a tab can be put in order rather
	// than only appended.
	_accept_drop(drop, DROP_INTO, _tab_insert_index_at(p_point));
}

bool EditorPane::_accept_drop(const PanelDrop &p_drop, DropZone p_zone, int p_tab_index) {
	if (!p_drop.is_valid()) {
		return false;
	}
	EditorPaneTree *tree = _get_pane_tree();

	if (p_zone == DROP_INTO) {
		if (!p_drop.source) {
			return add_panel(p_drop.type, p_drop.subject) >= 0;
		}
		int at = p_tab_index;
		if (p_drop.source == this && at > p_drop.source_index) {
			at--;
		}
		const bool moved = p_drop.source->transfer_panel_to(this, p_drop.source_index, at);
		if (tree) {
			tree->drop_empty_panes();
		}
		return moved;
	}

	if (!tree) {
		return false;
	}
	const bool vertical = p_zone == DROP_TOP || p_zone == DROP_BOTTOM;
	const bool before = p_zone == DROP_LEFT || p_zone == DROP_TOP;
	if (!p_drop.source) {
		return tree->split_with_new_panel(this, vertical, before, p_drop.type, p_drop.subject) != nullptr;
	}
	return tree->split_with_panel(this, vertical, before, p_drop.source, p_drop.source_index) != nullptr;
}

void EditorPane::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_theme();
			// A pane following the current scene changes color with it, and
			// one pointed at a scene stops showing any when it is the only one.
			EditorNode::get_singleton()->connect("scene_changed", callable_mp(this, &EditorPane::_scene_changed));
		} break;

		case NOTIFICATION_EXIT_TREE: {
			EditorNode *editor = EditorNode::get_singleton();
			if (editor && editor->is_connected("scene_changed", callable_mp(this, &EditorPane::_scene_changed))) {
				editor->disconnect("scene_changed", callable_mp(this, &EditorPane::_scene_changed));
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;

		case NOTIFICATION_PREDELETE: {
			// A Node kills its children as it goes, and it does that from this
			// very notification - so anything lent has to leave before the base
			// class gets its turn. Being told in reverse is what makes that work.
			_return_everything_lent();
		} break;

	}
}

bool EditorPane::transfer_panel_to(EditorPane *p_target, int p_index, int p_target_index) {
	ERR_FAIL_NULL_V(p_target, false);
	ERR_FAIL_INDEX_V(p_index, panels.size(), false);
	if (p_target == this && (p_target_index == p_index || p_target_index < 0)) {
		return false;
	}

	PanelEntry entry = panels[p_index];
	panels.remove_at(p_index);
	if (entry.control && entry.control->get_parent() == this) {
		remove_child(entry.control);
	}
	if (panels.is_empty()) {
		current = -1;
	} else {
		current = CLAMP(current >= p_index ? current - 1 : current, 0, panels.size() - 1);
	}
	_show_only_current();
	_update_tabs();

	const int at = (p_target_index < 0 || p_target_index > p_target->panels.size()) ? p_target->panels.size() : p_target_index;
	if (entry.control) {
		p_target->add_child(entry.control);
	}
	p_target->panels.insert(at, entry);
	p_target->current = at;
	p_target->_show_only_current();
	p_target->_update_tabs();

	emit_signal(SNAME("panels_changed"));
	if (p_target != this) {
		p_target->emit_signal(SNAME("panels_changed"));
	}
	return true;
}

void EditorPane::set_closable(bool p_closable) {
	close_button->set_visible(p_closable);
}

bool EditorPane::is_header_visible() const {
	return header_panel && header_panel->is_visible();
}

EditorPane *EditorPaneDropHint::_pane_at(const Point2 &p_point) const {
	if (!tree) {
		return nullptr;
	}
	const Point2 global = get_global_transform().xform(p_point);
	for (EditorPane *pane : tree->get_panes()) {
		if (pane->is_visible_in_tree() && pane->get_global_rect().has_point(global)) {
			return pane;
		}
	}
	return nullptr;
}

void EditorPaneDropHint::_forget() {
	if (target || zone != EditorPane::DROP_NONE || at_edge) {
		target = nullptr;
		zone = EditorPane::DROP_NONE;
		on_header = false;
		at_edge = false;
		_aim();
	}
}

void EditorPaneDropHint::track_external(const Point2 &p_screen_position, const Variant &p_data) {
	if (!is_visible()) {
		move_to_front();
		show();
	}
	can_drop_data(p_screen_position - get_screen_position(), p_data);
}

bool EditorPaneDropHint::drop_external(const Point2 &p_screen_position, const Variant &p_data) {
	const Point2 local = p_screen_position - get_screen_position();
	const bool taken = can_drop_data(local, p_data);
	if (taken) {
		drop_data(local, p_data);
	}
	end_external();
	return taken;
}

void EditorPaneDropHint::end_external() {
	_forget();
	shown_alpha = 0.0;
	set_process_internal(false);
	hide();
}

void EditorPaneDropHint::_draw_target(const Rect2 &p_rect, EditorPane::DropZone p_zone, bool p_edge, bool p_hot) {
	// A little picture of a pane with the part the panel would take filled in:
	// read at a glance, whatever the theme.
	target_box->set_border_color(accent * Color(1, 1, 1, (p_hot ? 1.0 : 0.6) * shown_alpha));
	target_box->set_bg_color(Color(0, 0, 0, (p_hot ? 0.75 : 0.55) * shown_alpha));
	draw_style_box(target_box, p_rect);

	const Rect2 inner = p_rect.grow(-7 * EDSCALE);
	const Color line = accent * Color(1, 1, 1, 0.9 * shown_alpha);
	draw_rect(inner, line, false, Math::round(1 * EDSCALE));
	// A side of one pane is half of it; a side of everything, a narrower strip
	// against the edge.
	const real_t share = p_edge ? 0.34 : 0.5;
	Rect2 filled = inner;
	switch (p_zone) {
		case EditorPane::DROP_LEFT:
			filled.size.x *= share;
			break;
		case EditorPane::DROP_RIGHT:
			filled.position.x += filled.size.x * (1.0 - share);
			filled.size.x *= share;
			break;
		case EditorPane::DROP_TOP:
			filled.size.y *= share;
			break;
		case EditorPane::DROP_BOTTOM:
			filled.position.y += filled.size.y * (1.0 - share);
			filled.size.y *= share;
			break;
		default:
			break;
	}
	draw_rect(filled, accent * Color(1, 1, 1, (p_hot ? 0.85 : 0.45) * shown_alpha));
}

void EditorPaneDropHint::_aim() {
	if (at_edge && tree) {
		// A whole side of the arrangement: the new pane takes a third of it.
		const Rect2 all = Rect2(Point2(), get_size());
		wanted_outline = all;
		Rect2 side = all;
		switch (zone) {
			case EditorPane::DROP_LEFT:
				side.size.x *= 0.3;
				break;
			case EditorPane::DROP_RIGHT:
				side.position.x += side.size.x * 0.7;
				side.size.x *= 0.3;
				break;
			case EditorPane::DROP_TOP:
				side.size.y *= 0.3;
				break;
			default:
				side.position.y += side.size.y * 0.7;
				side.size.y *= 0.3;
				break;
		}
		wanted_landing = side;
		wanted_alpha = 1.0;
		if (shown_alpha < 0.05) {
			shown_landing = wanted_landing;
			shown_outline = wanted_outline;
		}
		set_process_internal(true);
		queue_redraw();
		return;
	}
	if (!target || zone == EditorPane::DROP_NONE) {
		// Fading out where it was, rather than vanishing.
		wanted_alpha = 0.0;
		set_process_internal(true);
		return;
	}

	const Transform2D to_here = get_global_transform().affine_inverse() * target->get_global_transform();
	wanted_outline = to_here.xform(Rect2(Point2(), target->get_size()));
	Rect2 landing_rect = to_here.xform(target->get_body_rect());
	switch (zone) {
		case EditorPane::DROP_LEFT:
			landing_rect.size.x *= 0.5;
			break;
		case EditorPane::DROP_RIGHT:
			landing_rect.position.x += landing_rect.size.x * 0.5;
			landing_rect.size.x *= 0.5;
			break;
		case EditorPane::DROP_TOP:
			landing_rect.size.y *= 0.5;
			break;
		case EditorPane::DROP_BOTTOM:
			landing_rect.position.y += landing_rect.size.y * 0.5;
			landing_rect.size.y *= 0.5;
			break;
		default:
			// Joining this pane is about the whole of it, tabs included.
			landing_rect = wanted_outline;
			break;
	}
	wanted_landing = landing_rect;
	wanted_alpha = 1.0;
	if (shown_alpha < 0.05) {
		// Appearing: in place, fading in, rather than flying in from wherever
		// it was last.
		shown_landing = wanted_landing;
		shown_outline = wanted_outline;
	}
	set_process_internal(true);
	queue_redraw();
}

bool EditorPaneDropHint::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	// A whole side of the arrangement first: its targets sit over the panes.
	if (tree && EditorPane::is_panel_drag(p_data)) {
		const Point2 in_tree = tree->get_global_transform().affine_inverse().xform(get_global_transform().xform(p_point));
		const EditorPane::DropZone edge = tree->get_edge_target_at(in_tree);
		if (edge != EditorPane::DROP_NONE) {
			if (!at_edge || zone != edge) {
				at_edge = true;
				target = nullptr;
				zone = edge;
				on_header = false;
				const_cast<EditorPaneDropHint *>(this)->_aim();
			}
			return true;
		}
	}
	if (at_edge) {
		at_edge = false;
		target = nullptr;
		zone = EditorPane::DROP_NONE;
	}

	EditorPane *pane = _pane_at(p_point);
	if (!pane) {
		const_cast<EditorPaneDropHint *>(this)->_forget();
		return false;
	}

	const Point2 in_pane = pane->get_global_transform().affine_inverse().xform(get_global_transform().xform(p_point));
	if (!pane->can_accept_drop(in_pane, p_data)) {
		const_cast<EditorPaneDropHint *>(this)->_forget();
		return false;
	}

	// A target on the compass says exactly where; anywhere else the nearest
	// edge does, as it always has.
	EditorPane::DropZone now = pane->get_compass_zone_at(in_pane);
	if (now == EditorPane::DROP_NONE) {
		now = pane->get_drop_zone_at(in_pane, pane == target ? zone : EditorPane::DROP_NONE);
	}
	const bool header_now = pane->is_point_on_header(in_pane);
	if (pane != target || now != zone || header_now != on_header) {
		target = pane;
		zone = now;
		on_header = header_now;
		const_cast<EditorPaneDropHint *>(this)->_aim();
	} else if (header_now) {
		// The mark between the tabs follows the pointer rather than sitting in
		// one place.
		const_cast<EditorPaneDropHint *>(this)->queue_redraw();
	}
	return true;
}

void EditorPaneDropHint::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (at_edge && tree) {
		// A whole side: a new pane beside everything there is, and the panel
		// goes into it.
		const EditorPane::DropZone edge = zone;
		_forget();
		const bool vertical = edge == EditorPane::DROP_TOP || edge == EditorPane::DROP_BOTTOM;
		const bool before = edge == EditorPane::DROP_LEFT || edge == EditorPane::DROP_TOP;
		EditorPane *fresh = tree->split_root(vertical, before, 0.3);
		if (fresh) {
			fresh->accept_drop(Point2(), p_data, EditorPane::DROP_INTO);
			tree->drop_empty_panes();
		}
		return;
	}

	EditorPane *pane = _pane_at(p_point);
	if (!pane) {
		return;
	}
	const Point2 in_pane = pane->get_global_transform().affine_inverse().xform(get_global_transform().xform(p_point));
	// Where it was shown to be going, not where a last twitch might put it.
	const EditorPane::DropZone shown = pane == target ? zone : EditorPane::DROP_NONE;
	_forget();
	pane->accept_drop(in_pane, p_data, shown);
}

void EditorPaneDropHint::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			const int radius = EDSCALE * (int)EDITOR_GET("interface/theme/corner_radius");
			landing->set_corner_radius_all(radius);
			outline->set_corner_radius_all(radius);
		} break;

		case NOTIFICATION_DRAG_BEGIN: {
			// Only for a drag some pane could take. Anything else - a node onto
			// a 3D view, a file onto the FileSystem - has to reach what it was
			// aimed at, so this stays out of the way.
			if (!get_viewport() || !EditorPane::is_panel_drag(get_viewport()->gui_get_drag_data())) {
				break;
			}
			// Above whatever the panes are showing, however the arrangement has
			// been rebuilt since.
			move_to_front();
			show();
			// And followed into other windows, which the engine does not do.
			EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
			if (main_screen) {
				main_screen->begin_panel_drag(get_viewport(), get_viewport()->gui_get_drag_data());
			}
		} break;

		case NOTIFICATION_DRAG_END: {
			_forget();
			shown_alpha = 0.0;
			set_process_internal(false);
			hide();
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			// The same share of what is left every second, however many frames
			// that is, so it feels the same at any framerate. Quick: this is to
			// be followed, not watched.
			const real_t t = CLAMP(1.0 - Math::exp(-22.0 * get_process_delta_time()), 0.0, 1.0);
			shown_landing = Rect2(shown_landing.position.lerp(wanted_landing.position, t), shown_landing.size.lerp(wanted_landing.size, t));
			shown_outline = Rect2(shown_outline.position.lerp(wanted_outline.position, t), shown_outline.size.lerp(wanted_outline.size, t));
			shown_alpha = Math::lerp(shown_alpha, wanted_alpha, (real_t)CLAMP(t * (real_t)1.5, (real_t)0.0, (real_t)1.0));
			const bool arrived = shown_landing.position.distance_to(wanted_landing.position) < 0.5 && shown_landing.size.distance_to(wanted_landing.size) < 0.5 && shown_outline.position.distance_to(wanted_outline.position) < 0.5 && Math::abs(shown_alpha - wanted_alpha) < 0.01;
			if (arrived) {
				shown_landing = wanted_landing;
				shown_outline = wanted_outline;
				shown_alpha = wanted_alpha;
				set_process_internal(false);
			}
			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			if (shown_alpha <= 0.0) {
				break;
			}

			// The pane being aimed at, faintly, so it is clear which one is
			// being talked about even before the landing place is read.
			outline->set_border_color(accent * Color(1, 1, 1, 0.35 * shown_alpha));
			draw_style_box(outline, shown_outline.grow(-1 * EDSCALE));

			// Where the panel would end up.
			landing->set_bg_color(accent * Color(1, 1, 1, 0.18 * shown_alpha));
			landing->set_border_color(accent * Color(1, 1, 1, shown_alpha));
			draw_style_box(landing, shown_landing.grow(-2 * EDSCALE));

			// The targets: a whole side of the arrangement, and the compass in
			// the pane being aimed at, each lit when it is the one aimed at.
			if (tree && tree->get_panes().size() > 1) {
				const EditorPane::DropZone edges[] = { EditorPane::DROP_LEFT, EditorPane::DROP_RIGHT, EditorPane::DROP_TOP, EditorPane::DROP_BOTTOM };
				const Transform2D tree_to_here = get_global_transform().affine_inverse() * tree->get_global_transform();
				for (const EditorPane::DropZone edge : edges) {
					const Rect2 rect = tree->get_edge_target_rect(edge);
					if (rect.has_area()) {
						_draw_target(tree_to_here.xform(rect), edge, true, at_edge && zone == edge);
					}
				}
			}
			if (target && !on_header && !at_edge) {
				const Transform2D pane_to_here = get_global_transform().affine_inverse() * target->get_global_transform();
				const EditorPane::DropZone zones[] = { EditorPane::DROP_INTO, EditorPane::DROP_LEFT, EditorPane::DROP_RIGHT, EditorPane::DROP_TOP, EditorPane::DROP_BOTTOM };
				for (const EditorPane::DropZone target_zone : zones) {
					const Rect2 rect = target->get_compass_target_rect(target_zone);
					if (rect.has_area()) {
						_draw_target(pane_to_here.xform(rect), target_zone, false, zone == target_zone);
					}
				}
			}

			// Over the tabs, the bar says where between them it would go.
			if (on_header && target) {
				const Transform2D to_here = get_global_transform().affine_inverse() * target->get_global_transform();
				TabBar *bar = target->get_tab_bar();
				draw_set_transform_matrix(to_here * Transform2D(0, target->get_tab_bar()->get_global_position() - target->get_global_position()));
				bar->_draw_tab_drop(get_canvas_item());
				draw_set_transform_matrix(Transform2D());
			}
		} break;
	}
}

EditorPaneDropHint::EditorPaneDropHint() {
	hide();
	// It is there to be dropped on, and to be looked at - never to be clicked
	// through to, which is the whole point of it.
	set_mouse_filter(MOUSE_FILTER_STOP);

	landing.instantiate();
	landing->set_border_width_all(Math::round(2 * EDSCALE));
	outline.instantiate();
	outline->set_bg_color(Color(0, 0, 0, 0));
	outline->set_draw_center(false);
	outline->set_border_width_all(Math::round(1 * EDSCALE));

	target_box.instantiate();
	target_box->set_border_width_all(Math::round(1 * EDSCALE));
	target_box->set_corner_radius_all(Math::round(4 * EDSCALE));
}

EditorPane::EditorPane() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);
	_build_header();
}
