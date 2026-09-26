/**************************************************************************/
/*  editor_crash_report.cpp                                               */
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

#include "editor_crash_report.h"

#include "scene/main/scene_tree.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_scene_recovery.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/text_edit.h"
#include "servers/display/display_server.h"

String EditorCrashReport::get_logs_dir() {
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("editor/logs");
}

String EditorCrashReport::_marker_path() {
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("editor/session_running");
}

String EditorCrashReport::_find_previous_log() {
	// The log being written now is editor.log; the ones before it were renamed
	// with the time they were set aside, which sorts them oldest first.
	Ref<DirAccess> dir = DirAccess::open(get_logs_dir());
	if (dir.is_null()) {
		return String();
	}
	String newest;
	dir->list_dir_begin();
	for (String file = dir->get_next(); !file.is_empty(); file = dir->get_next()) {
		if (!dir->current_is_dir() && file.begins_with("editor") && file.get_extension() == "log" && file != "editor.log" && file > newest) {
			newest = file;
		}
	}
	dir->list_dir_end();
	return newest.is_empty() ? String() : get_logs_dir().path_join(newest);
}

String EditorCrashReport::_excerpt_of(const String &p_log) {
	const Vector<String> lines = p_log.split("\n");
	// The crash handler's report, from its banner to the end of the last
	// backtrace, when there is one.
	int start = -1;
	for (int i = 0; i < lines.size(); i++) {
		if (lines[i].contains("Program crashed")) {
			start = MAX(0, i - 1);
			break;
		}
	}
	if (start >= 0) {
		backtrace_found = true;
		int end = lines.size() - 1;
		for (int i = lines.size() - 1; i > start; i--) {
			if (lines[i].contains("END OF") && lines[i].contains("BACKTRACE")) {
				end = MIN(lines.size() - 1, i + 1);
				break;
			}
		}
		// A few lines from before it too: what was being done at the time.
		start = MAX(0, start - 8);
		return String("\n").join(lines.slice(start, end + 1)).strip_edges();
	}
	// The stall watchdog's report (see crash_handler_windows_seh.cpp), when it
	// is how the log ends: frozen, and stopped. One earlier on is only
	// something that took long and then went on.
	int stall = -1;
	for (int i = lines.size() - 1; i >= 0; i--) {
		if (lines[i].contains("has not gone round its main loop")) {
			stall = i;
			break;
		}
	}
	if (stall >= 0) {
		int end = -1;
		for (int i = stall; i < lines.size(); i++) {
			if (lines[i].contains("END OF THE STALLED MAIN THREAD")) {
				end = i;
				break;
			}
		}
		int after = 0;
		for (int i = end + 1; end >= 0 && i < lines.size(); i++) {
			const String rest = lines[i].strip_edges();
			after += !rest.is_empty() && !rest.begins_with("====");
		}
		if (end >= 0 && after < 3) {
			stall_found = true;
			return String("\n").join(lines.slice(MAX(0, stall - 8), end + 1)).strip_edges();
		}
	}
	// Otherwise the end of it: it stopped there, for whatever reason.
	return String("\n").join(lines.slice(MAX(0, lines.size() - 40))).strip_edges();
}

void EditorCrashReport::begin_session() {
	// A run with no window - an export, a script, some tool checking the
	// project - is not a session anyone sits in front of. It neither marks
	// the project nor reads the mark: one stopped half way must not greet the
	// next person to open the project with a crash report, and one starting
	// must not use up the report of a session that did crash. The self-test
	// runs without a window and checks all of this, so it is the exception.
	if (DisplayServer::get_singleton()->get_name() == "headless" && !OS::get_singleton()->has_environment("LATTICE_SELFTEST")) {
		return;
	}
	tracking = true;
	const String marker = _marker_path();
	if (FileAccess::exists(marker)) {
		const int pid = FileAccess::get_file_as_string(marker).strip_edges().to_int();
		if (pid > 0 && pid != OS::get_singleton()->get_process_id() && OS::get_singleton()->process_exists(pid)) {
			// Another editor has this project open and is still running: not a
			// crash, and not this session's mark to take over or clear.
			return;
		}
		previous_session_crashed = true;
		previous_log = _find_previous_log();
		if (!previous_log.is_empty()) {
			excerpt = _excerpt_of(FileAccess::get_file_as_string(previous_log));
		}
	}

	DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(marker.get_base_dir()));
	Ref<FileAccess> file = FileAccess::open(marker, FileAccess::WRITE);
	if (file.is_valid()) {
		file->store_string(itos(OS::get_singleton()->get_process_id()));
		owns_marker = true;
	}
}

void EditorCrashReport::end_session() {
	if (!owns_marker) {
		return;
	}
	owns_marker = false;
	DirAccess::remove_absolute(ProjectSettings::get_singleton()->globalize_path(_marker_path()));
}

void EditorCrashReport::popup_if_needed() {
	if (!previous_session_crashed) {
		return;
	}
	if (backtrace_found) {
		set_title(TTR("The editor crashed last time"));
		message->set_text(TTR("The last session on this project crashed. This is what it reported:"));
	} else if (stall_found) {
		set_title(TTR("The editor froze last time"));
		message->set_text(TTR("The last session on this project stopped responding and was closed. This is where it was stuck:"));
	} else {
		set_title(TTR("The editor did not close properly last time"));
		message->set_text(TTR("The last session on this project ended without closing - it crashed, or was stopped. This is how its log ends:"));
	}
	text->set_text(excerpt.is_empty() ? TTR("(Its log could not be found.)") : excerpt);
	const int recoverable = EditorSceneRecovery::get_previous_count();
	restore_button->set_visible(recoverable > 0);
	restore_button->set_text(vformat(TTRN("Restore %d Unsaved Scene", "Restore %d Unsaved Scenes", recoverable), recoverable));
	// Never taller than three quarters of the editor's window: the log's own
	// box gives way, and scrolls.
	const real_t max_height = get_tree()->get_root()->get_size().height * 0.75;
	text->set_custom_minimum_size(Size2(0, MIN(240 * EDSCALE, max_height * 0.4)));
	popup_centered_clamped(Size2(760, 420) * EDSCALE, 0.75);
	if (get_size().height > max_height) {
		set_size(Size2i(get_size().width, max_height));
		move_to_center();
	}
}

void EditorCrashReport::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		// A backtrace reads in columns.
		text->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("source"), EditorStringName(EditorFonts)));
		text->add_theme_font_size_override(SceneStringName(font_size), get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts)));
	}
}

void EditorCrashReport::_restore_pressed() {
	EditorSceneRecovery *recovery = EditorSceneRecovery::get_singleton();
	if (recovery) {
		recovery->restore_previous();
	}
	hide();
}

void EditorCrashReport::_copy_pressed() {
	DisplayServer::get_singleton()->clipboard_set(text->get_text());
}

void EditorCrashReport::_open_folder_pressed() {
	OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(previous_log.is_empty() ? get_logs_dir() : previous_log), true);
}

EditorCrashReport::EditorCrashReport() {
	set_ok_button_text(TTRC("Close"));

	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);

	message = memnew(Label);
	message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	// A wrapping label with no width of its own is measured a letter wide,
	// every word a line of its own - which made this dialog taller than the
	// screen.
	message->set_custom_minimum_size(Size2(600, 0) * EDSCALE);
	box->add_child(message);

	text = memnew(TextEdit);
	text->set_editable(false);
	text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	text->set_custom_minimum_size(Size2(0, 240) * EDSCALE);
	box->add_child(text);

	Button *copy = add_button(TTRC("Copy"), false, "copy");
	copy->connect(SceneStringName(pressed), callable_mp(this, &EditorCrashReport::_copy_pressed));
	Button *folder = add_button(TTRC("Show Log"), false, "folder");
	folder->connect(SceneStringName(pressed), callable_mp(this, &EditorCrashReport::_open_folder_pressed));
	// Offered when the session that crashed had scenes with changes not saved.
	restore_button = add_button(TTRC("Restore Unsaved Scenes"), true, "restore");
	restore_button->connect(SceneStringName(pressed), callable_mp(this, &EditorCrashReport::_restore_pressed));
	restore_button->hide();
}
