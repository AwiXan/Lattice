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
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_pane_tree.h"
#include "editor/gui/editor_panel_button.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "scene/gui/tab_bar.h"

void EditorPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::BOOL, "vertical")));
	ADD_SIGNAL(MethodInfo("close_requested"));
	ADD_SIGNAL(MethodInfo("panels_changed"));
}

void EditorPane::_build_header() {
	header = memnew(HFlowContainer);
	add_child(header);

	tab_bar = memnew(TabBar);
	tab_bar->set_h_size_flags(SIZE_EXPAND_FILL);
	// The bar offers arrows rather than demanding room for every tab, which is
	// what lets a pane be narrower than its tabs laid end to end.
	tab_bar->set_clip_tabs(true);
	tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ACTIVE_ONLY);
	tab_bar->connect(SNAME("tab_selected"), callable_mp(this, &EditorPane::_tab_selected));
	tab_bar->connect(SNAME("tab_close_pressed"), callable_mp(this, &EditorPane::_tab_close_pressed));
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

	header->add_child(memnew(VSeparator));

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
	split_right_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2Alt")));
	split_down_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2")));
	close_button->set_button_icon(base->get_editor_theme_icon(SNAME("Close")));
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

	// What a pane offers, split in two. A view of a scene - 2D, 3D, the tree,
	// the inspector - is something to arrange around what is being edited, so
	// it gets a button that can also be dragged somewhere. Everywhere else the
	// editor can take you is a place to go rather than a thing to arrange, so
	// they share one menu between them and take up a button's worth of room.
	//
	// A panel showing a resource is in neither: it is opened by dragging that
	// resource here. Nor is a dock: its own tab is how it is moved.
	Vector<StringName> wanted;
	Vector<StringName> elsewhere;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		if (!type || !type->offered || type->binding == EditorPanelRegistry::BINDING_RESOURCE) {
			continue;
		}
		if (type->binding == EditorPanelRegistry::BINDING_DOCUMENT) {
			wanted.push_back(id);
		} else {
			elsewhere.push_back(id);
		}
	}

	if (more_button) {
		PopupMenu *popup = more_button->get_popup();
		popup->clear();
		for (const StringName &id : elsewhere) {
			const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
			popup->add_icon_item(_icon_of(id), type->title.is_empty() ? String(id) : type->title);
		}
		more_types = elsewhere;
		more_button->set_visible(!elsewhere.is_empty());
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
	if (p_index < 0 || p_index >= more_types.size()) {
		return;
	}
	_palette_pressed(more_types[p_index]);
}

void EditorPane::_palette_pressed(const StringName &p_type) {
	// Pressing is the same as dropping it here, so it goes the same way and
	// gets the same answer about what to show.
	add_panel(p_type, _subject_for_type(p_type));
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
	if (subject_button->is_visible()) {
		_update_subject_list();
	}
	_update_header_visibility();
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

void EditorPane::_tab_close_pressed(int p_index) {
	close_panel(p_index);
}

void EditorPane::_subject_selected(int p_index) {
	set_panel_subject(subject_button->get_item_metadata(p_index));
}

void EditorPane::_split_pressed(bool p_vertical) {
	emit_signal(SNAME("split_requested"), p_vertical);
}

void EditorPane::_close_pressed() {
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
	entry.type = p_type;
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
	for (int i = 0; i < panels.size(); i++) {
		if (panels[i].type == p_type) {
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
	if (header && header->is_visible()) {
		// Where the header actually ends, rather than its height plus whatever
		// the separation happens to be.
		const real_t taken = header->get_rect().get_end().y;
		body.position.y = taken;
		body.size.y = MAX(0.0, get_size().y - taken);
	}
	return body;
}

bool EditorPane::is_point_on_header(const Point2 &p_point) const {
	return header && header->is_visible() && header->get_rect().has_point(p_point);
}

EditorPane::DropZone EditorPane::get_drop_zone_at(const Point2 &p_point) const {
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
	if (best == left && left < band_x) {
		return DROP_LEFT;
	}
	if (best == right && right < band_x) {
		return DROP_RIGHT;
	}
	if (best == top && top < band_y) {
		return DROP_TOP;
	}
	if (best == bottom && bottom < band_y) {
		return DROP_BOTTOM;
	}
	return DROP_INTO;
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
	if (drop.source == this && get_drop_zone_at(p_point) == DROP_INTO && !is_point_on_header(p_point)) {
		return false;
	}
	return true;
}

bool EditorPane::accept_drop(const Point2 &p_point, const Variant &p_data) {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return false;
	}

	if (is_point_on_header(p_point)) {
		// Where along the bar it was let go, so a tab can be put in order
		// rather than only appended.
		const Point2 in_bar = p_point - header->get_position() - tab_bar->get_position();
		int at = tab_bar->get_tab_idx_at_point(in_bar);
		if (at < 0) {
			at = panels.size();
		}
		return _accept_drop(drop, DROP_INTO, at);
	}
	return _accept_drop(drop, get_drop_zone_at(p_point), -1);
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

	// Something to see while it is in the air.
	Panel *preview = memnew(Panel);
	Label *label = memnew(Label);
	label->set_text(_title_of(panels[index]));
	preview->add_child(label);
	label->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_drag_preview(preview);

	return data;
}

bool EditorPane::_tab_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return false;
	}
	// Dropping a tab back where it came from changes nothing.
	return !(drop.source == this && tab_bar->get_tab_idx_at_point(p_point) == drop.source_index);
}

void EditorPane::_tab_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		return;
	}
	// Where along the bar it was let go, so a tab can be put in order rather
	// than only appended.
	int at = tab_bar->get_tab_idx_at_point(p_point);
	if (at < 0) {
		at = panels.size();
	}
	_accept_drop(drop, DROP_INTO, at);
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
		const bool moved = p_drop.source->transfer_panel_to(this, p_drop.source_index, p_tab_index);
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
		case NOTIFICATION_ENTER_TREE:
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
	return header && header->is_visible();
}

void EditorPane::set_header_visible(bool p_visible) {
	header_wanted = p_visible;
	_update_header_visibility();
}

void EditorPane::_update_header_visibility() {
	// A pane holding more than one panel needs its tabs, and one holding
	// nothing needs the row that offers something to put in it - otherwise an
	// empty pane is a dead end with no way out of it.
	header->set_visible(header_wanted || panels.size() != 1);
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
	if (target || zone != EditorPane::DROP_NONE) {
		target = nullptr;
		zone = EditorPane::DROP_NONE;
		on_header = false;
		queue_redraw();
	}
}

bool EditorPaneDropHint::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
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

	const EditorPane::DropZone now = pane->get_drop_zone_at(in_pane);
	const bool header_now = pane->is_point_on_header(in_pane);
	// Redrawn on every move while over the tabs, because the mark follows the
	// pointer between them rather than sitting in one place.
	if (pane != target || now != zone || header_now != on_header || header_now) {
		target = pane;
		zone = now;
		on_header = header_now;
		const_cast<EditorPaneDropHint *>(this)->queue_redraw();
	}
	return true;
}

void EditorPaneDropHint::drop_data(const Point2 &p_point, const Variant &p_data) {
	EditorPane *pane = _pane_at(p_point);
	if (!pane) {
		return;
	}
	const Point2 in_pane = pane->get_global_transform().affine_inverse().xform(get_global_transform().xform(p_point));
	_forget();
	pane->accept_drop(in_pane, p_data);
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
		} break;

		case NOTIFICATION_DRAG_END: {
			_forget();
			hide();
		} break;

		case NOTIFICATION_DRAW: {
			if (!target || zone == EditorPane::DROP_NONE) {
				break;
			}

			const Transform2D to_here = get_global_transform().affine_inverse() * target->get_global_transform();
			const Rect2 pane_rect = to_here.xform(Rect2(Point2(), target->get_size()));

			// The pane being aimed at, faintly, so it is clear which one is
			// being talked about even before the landing place is read.
			outline->set_border_color(accent * Color(1, 1, 1, 0.35));
			draw_style_box(outline, pane_rect.grow(-1 * EDSCALE));

			// Where the panel would end up.
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
					landing_rect = pane_rect;
					break;
			}

			landing->set_bg_color(accent * Color(1, 1, 1, 0.18));
			landing->set_border_color(accent);
			draw_style_box(landing, landing_rect.grow(-2 * EDSCALE));

			// Over the tabs, the bar says where between them it would go.
			if (on_header) {
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
}

EditorPane::EditorPane() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);
	_build_header();
}
