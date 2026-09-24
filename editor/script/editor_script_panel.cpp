/**************************************************************************/
/*  editor_script_panel.cpp                                               */
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

#include "editor_script_panel.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/gui/code_editor.h"
#include "editor/gui/editor_pane.h"
#include "editor/script/script_editor_base.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/main/viewport.h"

// ------------------------------------------------------------ the stand-in

Control *ScriptEditorStandIn::get_editor() const {
	return ObjectDB::get_instance<Control>(editor);
}

Control *ScriptEditorStandIn::get_panel() const {
	return ObjectDB::get_instance<Control>(panel);
}

void ScriptEditorStandIn::_show_pressed() {
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	Control *shown_in = get_panel();
	if (main_screen && shown_in) {
		main_screen->reveal_panel(shown_in);
	}
}

ScriptEditorStandIn::ScriptEditorStandIn(Control *p_editor, Control *p_panel) {
	editor = p_editor ? p_editor->get_instance_id() : ObjectID();
	panel = p_panel ? p_panel->get_instance_id() : ObjectID();
	set_alignment(ALIGNMENT_CENTER);
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);

	message = memnew(Label);
	message->set_text(TTRC("This script is open in a panel of its own."));
	message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	add_child(message);

	Button *show = memnew(Button);
	show->set_text(TTRC("Show It"));
	show->set_h_size_flags(SIZE_SHRINK_CENTER);
	show->connect(SceneStringName(pressed), callable_mp(this, &ScriptEditorStandIn::_show_pressed));
	add_child(show);
}

// ------------------------------------------------------------ the panel

Control *EditorScriptPanel::get_editor() const {
	return ObjectDB::get_instance<Control>(editor);
}

void EditorScriptPanel::_set_note(const String &p_text) {
	note->set_text(p_text);
	note->set_visible(!p_text.is_empty());
}

void EditorScriptPanel::set_script_path(const String &p_path) {
	if (p_path == path && get_editor()) {
		return;
	}
	// Pointed at another script: the one it had goes back first.
	_give_back(false);
	path = p_path;
	_set_note(String());

	ScriptEditor *script_editor = ScriptEditor::get_singleton();
	const Ref<Resource> resource = path.is_empty() ? Ref<Resource>() : ResourceLoader::load(path);
	if (!script_editor || resource.is_null()) {
		_set_note(vformat(TTR("%s could not be opened."), path.get_file()));
		return;
	}
	Control *borrowed = script_editor->lend_editor(resource, this);
	if (!borrowed) {
		_set_note(vformat(TTR("%s is shown in another panel."), path.get_file()));
		return;
	}
	editor = borrowed->get_instance_id();
	borrowed->set_v_size_flags(SIZE_EXPAND_FILL);
	borrowed->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(borrowed);
	// The find bar goes under the code, as in the script editor.
	move_child(find_bar, -1);

	if (TextEditorBase *text_editor = Object::cast_to<TextEditorBase>(borrowed)) {
		// Its menus come along, answering their shortcuts while the keyboard is
		// anywhere in this panel.
		Control *menu = text_editor->get_edit_menu();
		if (menu) {
			if (menu->get_parent()) {
				menu->get_parent()->remove_child(menu);
			}
			menu_bar->add_child(menu);
			menu->show();
			for (int i = 0; i < menu->get_child_count(); i++) {
				Control *item = Object::cast_to<Control>(menu->get_child(i));
				if (item) {
					item->set_shortcut_context(this);
				}
			}
		}
		text_editor->set_find_replace_bar(find_bar);
		text_editor->enable_editor();
	}
	borrowed->connect("name_changed", callable_mp(this, &EditorScriptPanel::_editor_name_changed));
	_editor_name_changed();
}

void EditorScriptPanel::_give_back(bool p_close) {
	Control *borrowed = get_editor();
	editor = ObjectID();
	if (!borrowed) {
		return;
	}
	if (borrowed->is_connected("name_changed", callable_mp(this, &EditorScriptPanel::_editor_name_changed))) {
		borrowed->disconnect("name_changed", callable_mp(this, &EditorScriptPanel::_editor_name_changed));
	}

	ScriptEditor *script_editor = ScriptEditor::get_singleton();
	const EditorNode *editor_node = EditorNode::get_singleton();
	if (!script_editor || !editor_node || !editor_node->is_inside_tree()) {
		// The editor is closing, the script editor with it, and in no order
		// anyone can count on - or the script editor is gone already: nobody to
		// give it back to. It goes with this panel, and so does its menu, which
		// is in this panel's bar.
		if (borrowed->get_parent() == this) {
			remove_child(borrowed);
		}
		memdelete(borrowed);
		return;
	}

	if (TextEditorBase *text_editor = Object::cast_to<TextEditorBase>(borrowed)) {
		Control *menu = text_editor->get_edit_menu();
		if (menu && menu->get_parent() == menu_bar) {
			menu_bar->remove_child(menu);
		}
	}
	if (borrowed->get_parent() == this) {
		remove_child(borrowed);
	}
	script_editor->take_back_editor(borrowed, p_close);
}

void EditorScriptPanel::_editor_name_changed() {
	refresh_title();
}

void EditorScriptPanel::refresh_title() {
	// The tab says which script, and whether it has changes not saved.
	EditorPane *pane = Object::cast_to<EditorPane>(get_parent());
	if (pane) {
		pane->refresh_titles();
	}
}

String EditorScriptPanel::get_title() const {
	ScriptEditorBase *script_editor = Object::cast_to<ScriptEditorBase>(get_editor());
	if (!script_editor) {
		return path.is_empty() ? String() : path.get_file();
	}
	String title = script_editor->get_name();
	if (script_editor->is_unsaved()) {
		title += "(*)";
	}
	return title;
}

void EditorScriptPanel::_focus_changed(Control *p_control) {
	// Working here makes this the script the rest of the script editor acts
	// on: its File and Debug menus, saving with Ctrl+S.
	Control *borrowed = get_editor();
	if (!borrowed || !p_control || !is_ancestor_of(p_control)) {
		return;
	}
	ScriptEditor *script_editor = ScriptEditor::get_singleton();
	if (script_editor) {
		script_editor->activate_lent_editor(borrowed);
	}
	// And the pane it is in is where the next script opens.
	EditorPane *pane = Object::cast_to<EditorPane>(get_parent());
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	if (pane && main_screen) {
		for (int i = 0; i < pane->get_panel_count(); i++) {
			if (pane->get_panel_at(i) == this) {
				main_screen->note_panel_touched(pane, i);
				break;
			}
		}
	}
}

void EditorScriptPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			get_viewport()->connect(SNAME("gui_focus_changed"), callable_mp(this, &EditorScriptPanel::_focus_changed));
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (get_viewport()->is_connected(SNAME("gui_focus_changed"), callable_mp(this, &EditorScriptPanel::_focus_changed))) {
				get_viewport()->disconnect(SNAME("gui_focus_changed"), callable_mp(this, &EditorScriptPanel::_focus_changed));
			}
		} break;

		case NOTIFICATION_PREDELETE: {
			// Closed by hand, the script is closed; otherwise it only goes back.
			_give_back(closing_by_user);
		} break;
	}
}

void EditorScriptPanel::shortcut_input(const Ref<InputEvent> &p_event) {
	// The script editor's File menu answers these only with the keyboard
	// inside it; here, with the keyboard inside this panel, they are this
	// script's.
	if (!p_event->is_pressed() || p_event->is_echo()) {
		return;
	}
	const Control *focus = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
	Control *borrowed = get_editor();
	ScriptEditor *script_editor = ScriptEditor::get_singleton();
	if (!focus || !is_ancestor_of(focus) || !borrowed || !script_editor) {
		return;
	}
	if (ED_IS_SHORTCUT("script_editor/save", p_event)) {
		script_editor->activate_lent_editor(borrowed);
		script_editor->save_current_script();
		accept_event();
	} else if (ED_IS_SHORTCUT("script_editor/close_file", p_event)) {
		// As its tab's close button: closing the script, asking if it has
		// changes not saved.
		EditorPane *pane = Object::cast_to<EditorPane>(get_parent());
		EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
		if (pane && main_screen) {
			for (int i = 0; i < pane->get_panel_count(); i++) {
				if (pane->get_panel_at(i) == this) {
					main_screen->note_panel_closing(pane, i);
					pane->close_panel(i);
					break;
				}
			}
		}
		accept_event();
	}
}

Control *EditorScriptPanel::create_panel() {
	return memnew(EditorScriptPanel);
}

void EditorScriptPanel::bind_panel(Control *p_panel, const Variant &p_subject) {
	EditorScriptPanel *panel = Object::cast_to<EditorScriptPanel>(p_panel);
	if (panel) {
		panel->set_script_path(p_subject);
	}
}

int EditorScriptPanel::rank_resource(const String &p_path, const StringName &p_class) {
	// One more than the script editor gives what it edits, while scripts get
	// panels of their own.
	if (!ScriptEditor::opens_scripts_in_panels() || p_class == StringName()) {
		return 0;
	}
	return (ClassDB::is_parent_class(p_class, "Script") || ClassDB::is_parent_class(p_class, "JSON")) ? 11 : 0;
}

String EditorScriptPanel::title_of(Control *p_panel) {
	EditorScriptPanel *panel = Object::cast_to<EditorScriptPanel>(p_panel);
	return panel ? panel->get_title() : String();
}

void EditorScriptPanel::closed_by_user(Control *p_panel) {
	EditorScriptPanel *panel = Object::cast_to<EditorScriptPanel>(p_panel);
	if (panel) {
		panel->closing_by_user = true;
	}
}

EditorScriptPanel::EditorScriptPanel() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);

	menu_bar = memnew(HBoxContainer);
	add_child(menu_bar);

	note = memnew(Label);
	note->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	note->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	note->set_v_size_flags(SIZE_EXPAND_FILL);
	note->hide();
	add_child(note);

	find_bar = memnew(FindReplaceBar);
	find_bar->hide();
	add_child(find_bar);

	set_process_shortcut_input(true);
}
