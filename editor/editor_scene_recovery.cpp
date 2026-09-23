/**************************************************************************/
/*  editor_scene_recovery.cpp                                             */
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

#include "editor_scene_recovery.h"

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/timer.h"
#include "scene/resources/packed_scene.h"

String EditorSceneRecovery::_dir(const String &p_which) {
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("editor/recovery").path_join(p_which);
}

void EditorSceneRecovery::_remove_dir(const String &p_dir) {
	Ref<DirAccess> dir = DirAccess::open(p_dir);
	if (dir.is_valid()) {
		dir->erase_contents_recursive();
		DirAccess::remove_absolute(ProjectSettings::get_singleton()->globalize_path(p_dir));
	}
}

String EditorSceneRecovery::_file_for(int p_history_id, const String &p_scene_path) {
	const String name = p_scene_path.is_empty() ? String("unsaved") : p_scene_path.get_file().get_basename();
	return name + "_" + itos(p_history_id) + ".tscn";
}

void EditorSceneRecovery::begin_session(bool p_previous_session_crashed) {
	const String current = _dir("current");
	const String previous = _dir("previous");
	if (p_previous_session_crashed && DirAccess::dir_exists_absolute(ProjectSettings::get_singleton()->globalize_path(current))) {
		_remove_dir(previous);
		Ref<DirAccess> dir = DirAccess::open(current.get_base_dir());
		if (dir.is_valid()) {
			dir->rename(current, previous);
		}
	} else {
		_remove_dir(current);
	}

	previous_count = 0;
	Ref<ConfigFile> index;
	index.instantiate();
	if (index->load(previous.path_join("index.cfg")) == OK) {
		for (const String &file : index->get_sections()) {
			if (FileAccess::exists(previous.path_join(file))) {
				previous_count++;
			}
		}
	}
}

void EditorSceneRecovery::end_session() {
	_remove_dir(_dir("current"));
	_remove_dir(_dir("previous"));
}

void EditorSceneRecovery::save_now() {
	EditorNode *editor = EditorNode::get_singleton();
	EditorData &editor_data = EditorNode::get_editor_data();
	const String dir = _dir("current");

	// Documents closed since their last copy: nothing to bring back.
	HashSet<int> open;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		open.insert(editor_data.get_scene_history_id(i));
	}
	LocalVector<int> gone;
	for (const KeyValue<int, String> &E : copied_files) {
		if (!open.has(E.key)) {
			gone.push_back(E.key);
		}
	}
	for (int history_id : gone) {
		_forget(history_id);
	}

	Ref<ConfigFile> index;
	index.instantiate();
	index->load(dir.path_join("index.cfg"));
	bool wrote = false;

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node *root = editor_data.get_edited_scene_root(i);
		const int history_id = editor_data.get_scene_history_id(i);
		if (!root || !editor->is_scene_unsaved(i)) {
			continue;
		}
		const uint64_t version = EditorUndoRedoManager::get_singleton()->get_or_create_history(history_id).undo_redo->get_version();
		if (copied_versions.has(history_id) && copied_versions[history_id] == version) {
			continue;
		}

		Ref<PackedScene> packed;
		packed.instantiate();
		if (packed->pack(root) != OK) {
			continue;
		}
		if (!wrote) {
			DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(dir));
		}
		const String file = _file_for(history_id, editor_data.get_scene_path(i));
		// Not a file of the project: the editor is not to go looking for it,
		// or remember how it was folded.
		editor->saving_resources_in_path.insert(packed);
		const Error err = ResourceSaver::save(packed, dir.path_join(file));
		editor->saving_resources_in_path.erase(packed);
		if (err != OK) {
			continue;
		}
		if (copied_files.has(history_id) && copied_files[history_id] != file) {
			// Saved under another name since the last copy.
			DirAccess::remove_absolute(ProjectSettings::get_singleton()->globalize_path(dir.path_join(copied_files[history_id])));
			if (index->has_section(copied_files[history_id])) {
				index->erase_section(copied_files[history_id]);
			}
		}
		index->set_value(file, "path", editor_data.get_scene_path(i));
		index->set_value(file, "title", editor_data.get_scene_title(i));
		copied_versions[history_id] = version;
		copied_files[history_id] = file;
		wrote = true;
	}

	if (wrote) {
		index->save(dir.path_join("index.cfg"));
	}
}

void EditorSceneRecovery::_forget(int p_history_id) {
	if (!copied_files.has(p_history_id)) {
		return;
	}
	const String dir = _dir("current");
	const String file = copied_files[p_history_id];
	DirAccess::remove_absolute(ProjectSettings::get_singleton()->globalize_path(dir.path_join(file)));
	Ref<ConfigFile> index;
	index.instantiate();
	if (index->load(dir.path_join("index.cfg")) == OK && index->has_section(file)) {
		index->erase_section(file);
		index->save(dir.path_join("index.cfg"));
	}
	copied_files.erase(p_history_id);
	copied_versions.erase(p_history_id);
}

void EditorSceneRecovery::_scene_saved(const String &p_path) {
	// The scene is safe on disk; its copy is only in the way.
	EditorData &editor_data = EditorNode::get_editor_data();
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == p_path) {
			_forget(editor_data.get_scene_history_id(i));
		}
	}
}

int EditorSceneRecovery::restore_previous() {
	const String previous = _dir("previous");
	Ref<ConfigFile> index;
	index.instantiate();
	if (index->load(previous.path_join("index.cfg")) != OK) {
		return 0;
	}

	EditorNode *editor = EditorNode::get_singleton();
	EditorData &editor_data = EditorNode::get_editor_data();
	int restored = 0;
	for (const String &file : index->get_sections()) {
		const String copy = previous.path_join(file);
		if (!FileAccess::exists(copy)) {
			continue;
		}
		const String original = index->get_value(file, "path", "");

		// The original, if it was opened again from disk, is what the copy is
		// the better version of. One with changes of its own is left alone,
		// and the copy opens beside it.
		bool original_kept_open = false;
		for (int i = 0; i < editor_data.get_edited_scene_count() && !original.is_empty(); i++) {
			if (editor_data.get_scene_path(i) == original) {
				if (editor->is_scene_unsaved(i)) {
					original_kept_open = true;
				} else {
					editor->_remove_scene(i, false);
				}
				break;
			}
		}

		if (editor->load_scene(copy, true) != OK) {
			continue;
		}
		// Found by the file it was opened from: which document is current
		// afterwards is not something to rely on, and pointing the wrong one
		// at the original would have it saved over with another scene.
		int at = -1;
		for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
			if (editor_data.get_scene_path(i) == copy) {
				at = i;
				break;
			}
		}
		if (at < 0) {
			continue;
		}
		// In place of the file it came from, with the changes not yet saved
		// there. A new scene if it never was saved - or if the original is
		// open with changes of its own, since two tabs cannot be one file.
		editor_data.set_scene_path(at, original_kept_open ? String() : original);
		EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(editor_data.get_scene_history_id(at));
		restored++;
	}

	// What was opened is not a recent file anyone chose.
	Array recent = EditorSettings::get_singleton()->get_project_metadata("recent_files", "scenes", Array());
	for (int i = recent.size() - 1; i >= 0; i--) {
		if (String(recent[i]).begins_with(previous)) {
			recent.remove_at(i);
		}
	}
	EditorSettings::get_singleton()->set_project_metadata("recent_files", "scenes", recent);
	editor->_update_recent_scenes();
	editor->scene_tabs->update_scene_tabs();

	_remove_dir(previous);
	previous_count = 0;
	return restored;
}

void EditorSceneRecovery::_update_interval() {
	const int minutes = EDITOR_GET("interface/editor/behavior/recovery_copy_interval");
	if (minutes <= 0) {
		timer->stop();
		return;
	}
	if (!Math::is_equal_approx(timer->get_wait_time(), minutes * 60.0) || timer->is_stopped()) {
		timer->start(minutes * 60.0);
	}
}

void EditorSceneRecovery::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			EditorNode::get_singleton()->connect("scene_saved", callable_mp(this, &EditorSceneRecovery::_scene_saved));
			_update_interval();
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/behavior/recovery_copy_interval")) {
				_update_interval();
			}
		} break;
	}
}

EditorSceneRecovery::EditorSceneRecovery() {
	singleton = this;
	set_name("EditorSceneRecovery");
	timer = memnew(Timer);
	timer->connect("timeout", callable_mp(this, &EditorSceneRecovery::save_now));
	add_child(timer);
}

EditorSceneRecovery::~EditorSceneRecovery() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
