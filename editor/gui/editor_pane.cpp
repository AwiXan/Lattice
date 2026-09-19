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
#include "scene/gui/separator.h"
#include "scene/gui/tab_bar.h"

void EditorPane::_bind_methods() {
	ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::BOOL, "vertical")));
	ADD_SIGNAL(MethodInfo("close_requested"));
	ADD_SIGNAL(MethodInfo("panels_changed"));
}

void EditorPane::_build_header() {
	header = memnew(HBoxContainer);
	add_child(header);

	tab_bar = memnew(TabBar);
	tab_bar->set_h_size_flags(SIZE_EXPAND_FILL);
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
	subject_button->hide();
	header->add_child(subject_button);

	header->add_child(memnew(VSeparator));

	palette = memnew(HBoxContainer);
	palette->add_theme_constant_override("separation", 0);
	header->add_child(palette);

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
	split_right_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2")));
	split_down_button->set_button_icon(base->get_editor_theme_icon(SNAME("Panels2Alt")));
	close_button->set_button_icon(base->get_editor_theme_icon(SNAME("Close")));
	_update_palette();
}

void EditorPane::_update_palette() {
	Control *base = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_gui_base() : nullptr;
	if (!base) {
		return;
	}

	// Everything the editor can make another of. A panel showing a resource is
	// not one: it is opened by dragging that resource here. Nor is a dock: there
	// is one of each, it is already somewhere, and its own tab is how it is
	// moved - a button offering a second FileSystem would be offering a thing
	// that cannot exist.
	Vector<StringName> wanted;
	for (const StringName &id : EditorPanelRegistry::get_type_ids()) {
		const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(id);
		if (type && !type->lent && type->binding != EditorPanelRegistry::BINDING_RESOURCE) {
			wanted.push_back(id);
		}
	}
	if (wanted == palette_types && palette->get_child_count() > 0) {
		// Nothing new registered; only the icons need saying again.
		for (int i = 0; i < palette->get_child_count(); i++) {
			EditorPanelButton *button = Object::cast_to<EditorPanelButton>(palette->get_child(i));
			const EditorPanelRegistry::PanelType *type = button ? EditorPanelRegistry::get_type(button->get_panel_type()) : nullptr;
			if (type && type->icon != StringName()) {
				button->set_button_icon(base->get_editor_theme_icon(type->icon));
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
		if (type->icon != StringName()) {
			button->set_button_icon(base->get_editor_theme_icon(type->icon));
		} else {
			button->set_text(title);
		}
		button->set_tooltip_text(vformat(TTR("Add a %s panel here, or drag it onto a pane to put one there."), title));
		button->connect(SceneStringName(pressed), callable_mp(this, &EditorPane::_palette_pressed).bind(id));
		palette->add_child(button);
	}
}

void EditorPane::_palette_pressed(const StringName &p_type) {
	// Pressing is the same as dropping it here, so it goes the same way and
	// gets the same answer about what to show.
	add_panel(p_type, _subject_for_type(p_type));
}

String EditorPane::_title_of(const PanelEntry &p_entry) const {
	if (p_entry.adopted) {
		return p_entry.title;
	}
	const EditorPanelRegistry::PanelType *type = EditorPanelRegistry::get_type(p_entry.type);
	return type && !type->title.is_empty() ? type->title : String(p_entry.type);
}

void EditorPane::_update_tabs() {
	rebuilding_tabs = true;
	tab_bar->clear_tabs();
	for (const PanelEntry &entry : panels) {
		tab_bar->add_tab(_title_of(entry));
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
	// A pane holding more than one panel always needs its tabs.
	if (panels.size() > 1) {
		header->show();
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
	if (panels[p_index].adopted) {
		// This is the editor's main screen, which has to be somewhere.
		return;
	}

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

bool EditorPane::is_panel_adopted_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return false;
	}
	return panels[p_index].adopted;
}

String EditorPane::get_panel_title_at(int p_index) const {
	if (p_index < 0 || p_index >= panels.size()) {
		return String();
	}
	return _title_of(panels[p_index]);
}

void EditorPane::set_panel_type(const StringName &p_type, const Variant &p_subject) {
	// Everything but the editor's main screen, which cannot be dropped.
	for (int i = panels.size() - 1; i >= 0; i--) {
		if (!panels[i].adopted) {
			_let_go_of(panels[i]);
			panels.remove_at(i);
		}
	}
	current = panels.is_empty() ? -1 : 0;
	add_panel(p_type, p_subject);
}

void EditorPane::set_panel_subject(const Variant &p_subject) {
	if (current < 0 || current >= panels.size()) {
		return;
	}
	panels.write[current].subject = p_subject;
	EditorPanelRegistry::bind_panel(panels[current].type, panels[current].control, p_subject);
}

void EditorPane::adopt_panel(Control *p_panel, const String &p_title) {
	ERR_FAIL_NULL(p_panel);
	ERR_FAIL_COND_MSG(is_adopting(), "This pane already shows the editor's main screen.");

	PanelEntry entry;
	entry.control = p_panel;
	entry.adopted = true;
	entry.title = p_title;
	p_panel->set_v_size_flags(SIZE_EXPAND_FILL);
	p_panel->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(p_panel);

	panels.insert(0, entry);
	current = 0;
	_show_only_current();
	_update_tabs();
	emit_signal(SNAME("panels_changed"));
}

bool EditorPane::is_adopting() const {
	for (const PanelEntry &entry : panels) {
		if (entry.adopted) {
			return true;
		}
	}
	return false;
}

String EditorPane::get_adopted_title() const {
	for (const PanelEntry &entry : panels) {
		if (entry.adopted) {
			return entry.title;
		}
	}
	return String();
}

Control *EditorPane::release_adopted_panel() {
	for (int i = 0; i < panels.size(); i++) {
		if (!panels[i].adopted) {
			continue;
		}
		Control *released = panels[i].control;
		if (released && released->get_parent() == this) {
			remove_child(released);
		}
		panels.remove_at(i);
		current = panels.is_empty() ? -1 : 0;
		_show_only_current();
		_update_tabs();
		return released;
	}
	return nullptr;
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
	// editor has one FileSystem, and a pane closing is not a reason to lose it.
	for (const PanelEntry &entry : panels) {
		if (entry.adopted || !entry.control) {
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

EditorPane::DropZone EditorPane::get_drop_zone_at(const Point2 &p_point) const {
	const Size2 size = get_size();
	if (size.x <= 0 || size.y <= 0) {
		return DROP_INTO;
	}
	// A quarter of each side, and no more than a comfortable band, so that a
	// large pane does not become mostly edge.
	const real_t band_x = MIN(size.x * 0.25, 120.0);
	const real_t band_y = MIN(size.y * 0.25, 120.0);

	// Whichever edge is nearest, if any is near enough.
	const real_t left = p_point.x;
	const real_t right = size.x - p_point.x;
	const real_t top = p_point.y;
	const real_t bottom = size.y - p_point.y;

	real_t best = MIN(MIN(left, right), MIN(top, bottom));
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

Variant EditorPane::_tab_get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	const int index = tab_bar->get_tab_idx_at_point(p_point);
	if (index < 0 || index >= panels.size() || panels[index].adopted) {
		// The editor's main screen stays where it is.
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

bool EditorPane::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	const PanelDrop drop = _read_drop(p_data);
	if (!drop.is_valid()) {
		drop_zone = DROP_NONE;
		return false;
	}
	const DropZone zone = get_drop_zone_at(p_point);
	// Dropping a pane's only panel back into the same pane changes nothing.
	if (drop.source == this && zone == DROP_INTO) {
		drop_zone = DROP_NONE;
		return false;
	}
	if (zone != drop_zone) {
		drop_zone = zone;
		const_cast<EditorPane *>(this)->queue_redraw();
	}
	return true;
}

void EditorPane::drop_data(const Point2 &p_point, const Variant &p_data) {
	const PanelDrop drop = _read_drop(p_data);
	const DropZone zone = get_drop_zone_at(p_point);
	drop_zone = DROP_NONE;
	queue_redraw();
	_accept_drop(drop, zone, -1);
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

		case NOTIFICATION_DRAG_END: {
			if (drop_zone != DROP_NONE) {
				drop_zone = DROP_NONE;
				queue_redraw();
			}
		} break;

		case NOTIFICATION_DRAW: {
			if (drop_zone == DROP_NONE) {
				break;
			}
			const Size2 size = get_size();
			Rect2 hint(Vector2(), size);
			switch (drop_zone) {
				case DROP_LEFT:
					hint.size.x *= 0.5;
					break;
				case DROP_RIGHT:
					hint.position.x = size.x * 0.5;
					hint.size.x *= 0.5;
					break;
				case DROP_TOP:
					hint.size.y *= 0.5;
					break;
				case DROP_BOTTOM:
					hint.position.y = size.y * 0.5;
					hint.size.y *= 0.5;
					break;
				default:
					break;
			}
			// From the editor's own base: a colour looked up here would be wrong
			// for a pane that is between parents.
			Color accent = EditorNode::get_singleton()->get_gui_base()->get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			accent.a = 0.25;
			draw_rect(hint, accent);
			accent.a = 0.8;
			draw_rect(hint, accent, false, 2.0);
		} break;
	}
}

bool EditorPane::transfer_panel_to(EditorPane *p_target, int p_index, int p_target_index) {
	ERR_FAIL_NULL_V(p_target, false);
	ERR_FAIL_INDEX_V(p_index, panels.size(), false);
	if (p_target == this && (p_target_index == p_index || p_target_index < 0)) {
		return false;
	}
	// The editor's main screen stays where it is: too much reaches it by name
	// for it to wander.
	if (panels[p_index].adopted) {
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

void EditorPane::set_header_visible(bool p_visible) {
	// A pane holding more than one panel always needs its tabs, however few
	// panes there are.
	header->set_visible(p_visible || panels.size() > 1);
}

EditorPane::EditorPane() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);
	_build_header();
}
