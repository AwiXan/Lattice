/**************************************************************************/
/*  editor_main_screen.cpp                                                */
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

#include "editor_main_screen.h"

#include "core/io/config_file.h"
#include "core/object/callable_mp.h"
#include "editor/editor_document_view.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/split_container.h"

void EditorMainScreen::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_accessibility_region(true);
			if (EDITOR_3D < buttons.size() && buttons[EDITOR_3D]->is_visible()) {
				// If the 3D editor is enabled, use this as the default.
				select(EDITOR_3D);
				return;
			}

			// Switch to the first main screen plugin that is enabled. Usually this is
			// 2D, but may be subsequent ones if 2D is disabled in the feature profile.
			for (int i = 0; i < buttons.size(); i++) {
				Button *editor_button = buttons[i];
				if (editor_button->is_visible()) {
					select(i);
					return;
				}
			}

			select(-1);
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			for (int i = 0; i < buttons.size(); i++) {
				Button *tb = buttons[i];
				EditorPlugin *p_editor = editor_table[i];
				Ref<Texture2D> icon = p_editor->get_plugin_icon();

				if (icon.is_valid()) {
					tb->set_button_icon(icon);
				} else if (has_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons))) {
					tb->set_button_icon(get_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons)));
				}
			}
		} break;
	}
}

void EditorMainScreen::set_button_container(HBoxContainer *p_button_hb) {
	button_hb = p_button_hb;
}

void EditorMainScreen::save_layout_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const {
	int selected_main_editor_idx = -1;
	for (int i = 0; i < buttons.size(); i++) {
		if (buttons[i]->is_pressed()) {
			selected_main_editor_idx = i;
			break;
		}
	}
	if (selected_main_editor_idx != -1) {
		p_config_file->set_value(p_section, "selected_main_editor_idx", selected_main_editor_idx);
	} else {
		p_config_file->set_value(p_section, "selected_main_editor_idx", Variant());
	}
}

void EditorMainScreen::load_layout_from_config(Ref<ConfigFile> p_config_file, const String &p_section) {
	int selected_main_editor_idx = p_config_file->get_value(p_section, "selected_main_editor_idx", -1);
	if (selected_main_editor_idx >= 0 && selected_main_editor_idx < buttons.size()) {
		callable_mp(this, &EditorMainScreen::select).call_deferred(selected_main_editor_idx);
	}
}

void EditorMainScreen::set_button_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, buttons.size());
	buttons[p_index]->set_visible(p_enabled);
	if (!p_enabled && buttons[p_index]->is_pressed()) {
		select(EDITOR_2D);
	}
}

bool EditorMainScreen::is_button_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);
	return buttons[p_index]->is_visible();
}

int EditorMainScreen::_get_current_main_editor() const {
	for (int i = 0; i < editor_table.size(); i++) {
		if (editor_table[i] == selected_plugin) {
			return i;
		}
	}

	return 0;
}

void EditorMainScreen::select_next() {
	int editor = _get_current_main_editor();

	do {
		if (editor == editor_table.size() - 1) {
			editor = 0;
		} else {
			editor++;
		}
	} while (!buttons[editor]->is_visible());

	select(editor);
}

void EditorMainScreen::select_prev() {
	int editor = _get_current_main_editor();

	do {
		if (editor == 0) {
			editor = editor_table.size() - 1;
		} else {
			editor--;
		}
	} while (!buttons[editor]->is_visible());

	select(editor);
}

void EditorMainScreen::select_by_name(const String &p_name) {
	ERR_FAIL_COND(p_name.is_empty());

	for (int i = 0; i < buttons.size(); i++) {
		if (buttons[i]->get_text() == p_name) {
			select(i);
			return;
		}
	}

	ERR_FAIL_MSG("The editor name '" + p_name + "' was not found.");
}

void EditorMainScreen::select(int p_index) {
	if (EditorNode::get_singleton()->is_changing_scene()) {
		return;
	}

	ERR_FAIL_INDEX(p_index, editor_table.size());

	if (!buttons[p_index]->is_visible()) { // Button hidden, no editor.
		return;
	}

	for (int i = 0; i < buttons.size(); i++) {
		buttons[i]->set_pressed_no_signal(i == p_index);
	}

	EditorPlugin *new_editor = editor_table[p_index];
	ERR_FAIL_NULL(new_editor);

	if (selected_plugin == new_editor) {
		return;
	}

	if (selected_plugin) {
		selected_plugin->make_visible(false);
	}

	selected_plugin = new_editor;
	selected_plugin->make_visible(true);
	selected_plugin->selected_notify();
	set_accessibility_name(selected_plugin->get_plugin_name());

	EditorData &editor_data = EditorNode::get_editor_data();
	int plugin_count = editor_data.get_editor_plugin_count();
	for (int i = 0; i < plugin_count; i++) {
		editor_data.get_editor_plugin(i)->notify_main_screen_changed(selected_plugin->get_plugin_name());
	}

	EditorNode::get_singleton()->update_distraction_free_mode();
}

int EditorMainScreen::get_selected_index() const {
	for (int i = 0; i < editor_table.size(); i++) {
		if (selected_plugin == editor_table[i]) {
			return i;
		}
	}
	return -1;
}

int EditorMainScreen::get_plugin_index(EditorPlugin *p_editor) const {
	int screen = -1;
	for (int i = 0; i < editor_table.size(); i++) {
		if (p_editor == editor_table[i]) {
			screen = i;
			break;
		}
	}
	return screen;
}

EditorPlugin *EditorMainScreen::get_selected_plugin() const {
	return selected_plugin;
}

EditorPlugin *EditorMainScreen::get_plugin_by_name(const String &p_plugin_name) const {
	ERR_FAIL_COND_V(!main_editor_plugins.has(p_plugin_name), nullptr);
	return main_editor_plugins[p_plugin_name];
}

bool EditorMainScreen::can_auto_switch_screens() const {
	if (selected_plugin == nullptr) {
		return true;
	}
	// Only allow auto-switching if the selected button is to the left of the Script button.
	for (int i = 0; i < button_hb->get_child_count(); i++) {
		Button *button = Object::cast_to<Button>(button_hb->get_child(i));
		if (button->get_text() == "Script") {
			// Selected button is at or after the Script button.
			return false;
		}
		if (button->get_text() == selected_plugin->get_plugin_name()) {
			// Selected button is before the Script button.
			return true;
		}
	}
	return false;
}

VBoxContainer *EditorMainScreen::get_control() const {
	return main_screen_vbox;
}

VBoxContainer *EditorMainScreen::get_secondary_control() const {
	return secondary_screen_vbox;
}

void EditorMainScreen::view_activated(EditorDocumentView *p_view) {
	active_view = p_view;
	if (!p_view || changing_context) {
		return;
	}

	const int document_id = p_view->get_bound_document();
	if (document_id < 0) {
		// This view follows the current document already; there is nothing to
		// bring the rest of the editor to.
		return;
	}

	EditorData &editor_data = EditorNode::get_editor_data();
	const int idx = editor_data.get_scene_index_by_history_id(document_id);
	// A closed document leaves the view following the current one, and there is
	// nothing to switch to.
	if (idx < 0 || idx == editor_data.get_edited_scene()) {
		return;
	}

	changing_context = true;
	EditorNode::get_singleton()->set_current_scene_index(idx);
	changing_context = false;
}

void EditorMainScreen::current_document_changed() {
	if (changing_context || !active_view) {
		return;
	}
	// A view that follows the current document needs no telling.
	if (active_view->get_bound_document() < 0) {
		return;
	}
	// The pane being worked in is what the tab bar acts on: picking a scene up
	// there shows it in that pane, and leaves the other panes where they are.
	EditorData &editor_data = EditorNode::get_editor_data();
	if (editor_data.get_edited_scene_count() > 0) {
		active_view->bind_document(editor_data.get_scene_history_id(editor_data.get_edited_scene()));
	}
	// The header names the document its pane shows, so it has to be told when
	// something other than the header itself changed which one that is.
	_update_secondary_document_list();
}

bool EditorMainScreen::can_split_view() const {
	return selected_plugin != nullptr;
}

int EditorMainScreen::_pick_document_for_second_pane() const {
	// The scene being worked on, so that splitting gives two views of it and
	// nothing appears to have gone wrong. Showing a different document is what
	// the header's picker is for, and it is the more interesting half of the
	// feature, but it is not what someone asking for a split expects to get.
	//
	// Returned as a history id: the pane keeps pointing at that document however
	// its tab moves, and follows the current one again once it is closed.
	EditorData &editor_data = EditorNode::get_editor_data();
	const int count = editor_data.get_edited_scene_count();
	if (count < 1) {
		return -1;
	}
	return editor_data.get_scene_history_id(editor_data.get_edited_scene());
}

void EditorMainScreen::_build_secondary_header() {
	if (secondary_header) {
		return;
	}

	secondary_header = memnew(HBoxContainer);
	secondary_screen_vbox->add_child(secondary_header);

	secondary_document = memnew(OptionButton);
	secondary_document->set_tooltip_text(TTRC("The scene this pane is showing. It does not have to be the one the tab bar has selected."));
	secondary_document->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	secondary_document->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	// Filled when it is opened rather than kept in step with the tab bar, so
	// that opening and closing scenes needs no notification to reach here.
	secondary_document->get_popup()->connect(SNAME("about_to_popup"), callable_mp(this, &EditorMainScreen::_update_secondary_document_list));
	secondary_document->connect(SceneStringName(item_selected), callable_mp(this, &EditorMainScreen::_secondary_document_selected));
	secondary_header->add_child(secondary_document);

	Button *close_button = memnew(Button);
	close_button->set_flat(true);
	close_button->set_tooltip_text(TTRC("Close this pane."));
	close_button->set_text(TTRC("Close"));
	close_button->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::set_split_view_enabled).bind(false));
	secondary_header->add_child(close_button);
}

void EditorMainScreen::_update_secondary_document_list() {
	if (!secondary_document) {
		return;
	}

	const int bound = secondary_view ? secondary_view->get_bound_document() : -1;
	secondary_document->clear();

	EditorData &editor_data = EditorNode::get_editor_data();
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		const int history_id = editor_data.get_scene_history_id(i);
		String title = editor_data.get_scene_title(i);
		if (title.is_empty()) {
			title = TTR("[unsaved]");
		}
		secondary_document->add_item(title, history_id);
		if (history_id == bound) {
			secondary_document->select(secondary_document->get_item_count() - 1);
		}
	}
}

void EditorMainScreen::_secondary_document_selected(int p_index) {
	if (!secondary_view) {
		return;
	}
	secondary_view->bind_document(secondary_document->get_item_id(p_index));
}

void EditorMainScreen::set_split_view_enabled(bool p_enabled) {
	if (p_enabled == is_split_view_enabled()) {
		return;
	}

	if (!p_enabled) {
		memdelete(secondary_view);
		secondary_view = nullptr;
		secondary_screen_vbox->hide();
		// One pane follows the current document again, as it did before there
		// was anything to hold still for.
		if (pinned_primary_view) {
			pinned_primary_view->bind_document(-1);
			pinned_primary_view = nullptr;
		}
		active_view = nullptr;
		EditorNode::get_singleton()->update_split_view_menu_item();
		return;
	}

	ERR_FAIL_NULL_MSG(selected_plugin, "No main screen editor is selected, so there is nothing to show in a second pane.");

	Control *view = selected_plugin->create_main_screen_view();
	if (!view) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("The %s editor cannot be opened a second time yet."), selected_plugin->get_plugin_name()));
		return;
	}

	secondary_view = Object::cast_to<EditorDocumentView>(view);
	if (!secondary_view) {
		memdelete(view);
		ERR_FAIL_MSG(vformat("The %s editor returned a second view that is not an EditorDocumentView.", selected_plugin->get_plugin_name()));
	}

	_build_secondary_header();
	secondary_screen_vbox->add_child(secondary_view);
	secondary_screen_vbox->show();

	if (secondary_view->supports_document_binding()) {
		const int document_id = _pick_document_for_second_pane();
		// The first pane stops following the current document and holds the one
		// it is showing, or pointing the second pane elsewhere - which makes
		// that document current - would drag the first pane along with it and
		// leave both panes showing the same scene.
		EditorDocumentView *primary_view = selected_plugin->get_main_screen_view();
		if (primary_view && primary_view->supports_document_binding()) {
			primary_view->bind_document(document_id);
			pinned_primary_view = primary_view;
			// Something has to be what the tab bar acts on, and until the user
			// works in a pane it is the one that was there first.
			active_view = primary_view;
		}
		secondary_view->bind_document(document_id);
		secondary_header->show();
	} else {
		// Nothing to choose: this view always shows the current scene.
		secondary_header->hide();
	}
	_update_secondary_document_list();
	EditorNode::get_singleton()->update_split_view_menu_item();
}

void EditorMainScreen::add_main_plugin(EditorPlugin *p_editor) {
	Button *tb = memnew(Button);
	tb->set_toggle_mode(true);
	tb->set_theme_type_variation("MainScreenButton");
	tb->set_name(p_editor->get_plugin_name());
	tb->set_text(p_editor->get_plugin_name());

	Ref<Shortcut> shortcut = EditorSettings::get_singleton()->get_shortcut("editor/editor_" + p_editor->get_plugin_name().to_lower());
	if (shortcut.is_valid()) {
		tb->set_shortcut(shortcut);
	}

	Ref<Texture2D> icon = p_editor->get_plugin_icon();
	if (icon.is_null() && has_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons))) {
		icon = get_editor_theme_icon(p_editor->get_plugin_name());
	}
	if (icon.is_valid()) {
		tb->set_button_icon(icon);
		// Make sure the control is updated if the icon is reimported.
		icon->connect_changed(callable_mp((Control *)tb, &Control::update_minimum_size));
	}

	tb->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select).bind(buttons.size()));

	buttons.push_back(tb);
	button_hb->add_child(tb);
	editor_table.push_back(p_editor);
	main_editor_plugins.insert(p_editor->get_plugin_name(), p_editor);
}

void EditorMainScreen::remove_main_plugin(EditorPlugin *p_editor) {
	// Remove the main editor button and update the bindings of
	// all buttons behind it to point to the correct main window.
	for (int i = buttons.size() - 1; i >= 0; i--) {
		if (p_editor->get_plugin_name() == buttons[i]->get_text()) {
			if (buttons[i]->is_pressed()) {
				select(EDITOR_SCRIPT);
			}

			memdelete(buttons[i]);
			buttons.remove_at(i);

			break;
		} else {
			buttons[i]->disconnect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select));
			buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select).bind(i - 1));
		}
	}

	if (selected_plugin == p_editor) {
		selected_plugin = nullptr;
	}

	editor_table.erase(p_editor);
	main_editor_plugins.erase(p_editor->get_plugin_name());
}

EditorMainScreen::EditorMainScreen() {
	pane_split = memnew(HSplitContainer);
	pane_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(pane_split);

	main_screen_vbox = memnew(VBoxContainer);
	main_screen_vbox->set_name("MainScreen");
	main_screen_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->add_theme_constant_override("separation", 0);
	pane_split->add_child(main_screen_vbox);

	// Stays hidden, and with it the split dragger, until split view is enabled.
	secondary_screen_vbox = memnew(VBoxContainer);
	secondary_screen_vbox->set_name("SecondaryMainScreen");
	secondary_screen_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	secondary_screen_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	secondary_screen_vbox->add_theme_constant_override("separation", 0);
	secondary_screen_vbox->hide();
	pane_split->add_child(secondary_screen_vbox);
}
