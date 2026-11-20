/**************************************************************************/
/*  cargo_manager_dock.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "cargo_manager_dock.h"

#include "../rust_manager.h"
#include "core/object/worker_thread_pool.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"

CargoManagerDock *CargoManagerDock::singleton = nullptr;

void CargoManagerDock::_bind_methods() {
}

void CargoManagerDock::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED || p_what == NOTIFICATION_ENTER_TREE) {
		if (btn_build_debug && has_theme_icon(SNAME("Tools"), EditorStringName(EditorIcons))) {
			btn_build_debug->set_button_icon(get_theme_icon(SNAME("Tools"), EditorStringName(EditorIcons)));
		}
		if (btn_build_release && has_theme_icon(SNAME("Play"), EditorStringName(EditorIcons))) {
			btn_build_release->set_button_icon(get_theme_icon(SNAME("Play"), EditorStringName(EditorIcons)));
		}
		if (btn_reload && has_theme_icon(SNAME("Reload"), EditorStringName(EditorIcons))) {
			btn_reload->set_button_icon(get_theme_icon(SNAME("Reload"), EditorStringName(EditorIcons)));
		}
		if (btn_open_toml && has_theme_icon(SNAME("Script"), EditorStringName(EditorIcons))) {
			btn_open_toml->set_button_icon(get_theme_icon(SNAME("Script"), EditorStringName(EditorIcons)));
		}
		if (btn_clear_log && has_theme_icon(SNAME("Clear"), EditorStringName(EditorIcons))) {
			btn_clear_log->set_button_icon(get_theme_icon(SNAME("Clear"), EditorStringName(EditorIcons)));
		}
		if (btn_refresh_deps && has_theme_icon(SNAME("ReloadSmall"), EditorStringName(EditorIcons))) {
			btn_refresh_deps->set_button_icon(get_theme_icon(SNAME("ReloadSmall"), EditorStringName(EditorIcons)));
		}
		_update_status();
	}
}

void CargoManagerDock::_update_status() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}

	String text;
	if (rm->is_cargo_available()) {
		text = vformat("[%s]", rm->get_cargo_version());
	} else {
		text = "[Cargo: Not Found on PATH]";
	}

	if (rm->is_library_loaded()) {
		text += " | [Native Lib: Loaded]";
	} else {
		text += " | [Native Lib: Not Loaded]";
	}

	if (status_label) {
		status_label->set_text(text);
	}

	_update_deps();
}

void CargoManagerDock::_update_deps() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm || !deps_list) {
		return;
	}

	deps_list->clear();
	PackedStringArray deps = rm->get_dependencies();
	for (int i = 0; i < deps.size(); i++) {
		deps_list->add_item(deps[i]);
	}

	if (deps.is_empty()) {
		deps_list->add_item(TTR("No dependencies found in Cargo.toml."));
	}
}

void CargoManagerDock::_build_worker(void *p_userdata) {
	BuildTaskData *task_data = static_cast<BuildTaskData *>(p_userdata);
	RustManager *rm = RustManager::get_singleton();
	if (rm) {
		if (task_data->action == "build") {
			task_data->err = rm->build_crate(task_data->is_release, &task_data->output, &task_data->exit_code);
		} else if (task_data->action == "check") {
			task_data->err = rm->check_crate(&task_data->output, &task_data->exit_code);
		} else if (task_data->action == "clean") {
			task_data->err = rm->clean_crate(&task_data->output, &task_data->exit_code);
		}
	}

	callable_mp(CargoManagerDock::get_singleton(), &CargoManagerDock::_on_build_completed).call_deferred(
			task_data->action, task_data->exit_code, task_data->output, (int)task_data->err);
	memdelete(task_data);
}

void CargoManagerDock::_on_build_completed(String p_action, int p_exit_code, String p_output, int p_err) {
	is_building = false;

	if (btn_build_debug) {
		btn_build_debug->set_disabled(false);
	}
	if (btn_build_release) {
		btn_build_release->set_disabled(false);
	}
	if (btn_check) {
		btn_check->set_disabled(false);
	}
	if (btn_clean) {
		btn_clean->set_disabled(false);
	}

	if (log_view) {
		log_view->clear();
		log_view->add_text(p_output);
	}

	if (p_err == OK && p_exit_code == 0) {
		if (build_status_label) {
			build_status_label->set_text(vformat(TTR("Task '%s' completed successfully."), p_action));
		}
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(vformat(TTR("Rust %s succeeded!"), p_action), EditorToaster::SEVERITY_INFO);
		}
	} else {
		if (build_status_label) {
			build_status_label->set_text(vformat(TTR("Task '%s' failed (exit code %d)."), p_action, p_exit_code));
		}
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(vformat(TTR("Rust %s failed! Check Cargo dock for details."), p_action), EditorToaster::SEVERITY_ERROR);
		}
	}

	_update_status();
}

void CargoManagerDock::_start_cargo_task(const String &p_action, bool p_release) {
	if (is_building) {
		return;
	}

	is_building = true;
	if (btn_build_debug) {
		btn_build_debug->set_disabled(true);
	}
	if (btn_build_release) {
		btn_build_release->set_disabled(true);
	}
	if (btn_check) {
		btn_check->set_disabled(true);
	}
	if (btn_clean) {
		btn_clean->set_disabled(true);
	}

	if (build_status_label) {
		build_status_label->set_text(vformat(TTR("Running 'cargo %s'..."), p_action));
	}

	BuildTaskData *task_data = memnew(BuildTaskData);
	task_data->action = p_action;
	task_data->is_release = p_release;

	WorkerThreadPool::get_singleton()->add_native_task(&CargoManagerDock::_build_worker, task_data, true, "CargoTask");
}

void CargoManagerDock::trigger_build(bool p_release) {
	_start_cargo_task("build", p_release);
}

void CargoManagerDock::_on_build_debug() {
	_start_cargo_task("build", false);
}

void CargoManagerDock::_on_build_release() {
	_start_cargo_task("build", true);
}

void CargoManagerDock::_on_check() {
	_start_cargo_task("check", false);
}

void CargoManagerDock::_on_clean() {
	_start_cargo_task("clean", false);
}

void CargoManagerDock::_on_reload() {
	RustManager *rm = RustManager::get_singleton();
	if (rm) {
		Error err = rm->reload_library();
		if (err == OK) {
			if (EditorToaster::get_singleton()) {
				EditorToaster::get_singleton()->popup_str(TTR("Rust dynamic library reloaded!"), EditorToaster::SEVERITY_INFO);
			}
		} else {
			if (EditorToaster::get_singleton()) {
				EditorToaster::get_singleton()->popup_str(TTR("Failed to reload Rust library."), EditorToaster::SEVERITY_WARNING);
			}
		}
		_update_status();
	}
}

void CargoManagerDock::_on_open_toml() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}
	String crate_dir = rm->find_project_crate_dir();
	if (crate_dir.is_empty()) {
		rm->generate_scaffolding("res://rust");
		crate_dir = "res://rust";
	}
	String toml_path = crate_dir.path_join("Cargo.toml");
	if (FileAccess::exists(toml_path) && ScriptEditor::get_singleton()) {
		ScriptEditor::get_singleton()->open_file(toml_path);
	}
}

void CargoManagerDock::_on_clear_log() {
	if (log_view) {
		log_view->clear();
	}
	if (build_status_label) {
		build_status_label->set_text(TTR("Log cleared."));
	}
}

void CargoManagerDock::_on_add_dep() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm || !dep_name_edit) {
		return;
	}
	String crate_name = dep_name_edit->get_text().strip_edges();
	if (crate_name.is_empty()) {
		return;
	}
	String version = dep_version_edit ? dep_version_edit->get_text().strip_edges() : "";

	String output;
	int exit_code = 0;
	Error err = rm->add_dependency(crate_name, version, &output, &exit_code);
	if (err == OK) {
		dep_name_edit->clear();
		if (dep_version_edit) {
			dep_version_edit->clear();
		}
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(vformat(TTR("Added dependency '%s' to Cargo.toml"), crate_name), EditorToaster::SEVERITY_INFO);
		}
	} else {
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(vformat(TTR("Failed to add dependency '%s'"), crate_name), EditorToaster::SEVERITY_ERROR);
		}
	}
	_update_deps();
}

void CargoManagerDock::_on_refresh_deps() {
	_update_deps();
}

void CargoManagerDock::_on_create_scaffold() {
	RustManager *rm = RustManager::get_singleton();
	if (rm) {
		Error err = rm->generate_scaffolding("res://rust");
		if (err == OK) {
			if (EditorToaster::get_singleton()) {
				EditorToaster::get_singleton()->popup_str(TTR("Rust project scaffolding created in res://rust/"), EditorToaster::SEVERITY_INFO);
			}
		}
		_update_status();
	}
}

CargoManagerDock::CargoManagerDock() {
	singleton = this;
	set_custom_minimum_size(Size2(0, 200 * EDSCALE));

	// Top control bar
	HBoxContainer *top_bar = memnew(HBoxContainer);
	add_child(top_bar);

	status_label = memnew(Label);
	status_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_bar->add_child(status_label);

	btn_build_debug = memnew(Button(TTR("Build (Debug)")));
	btn_build_debug->set_tooltip_text(TTR("Compile Rust crate in Debug mode and hot-reload library"));
	btn_build_debug->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_build_debug));
	top_bar->add_child(btn_build_debug);

	btn_build_release = memnew(Button(TTR("Build (Release)")));
	btn_build_release->set_tooltip_text(TTR("Compile Rust crate with full release optimizations (--release)"));
	btn_build_release->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_build_release));
	top_bar->add_child(btn_build_release);

	btn_check = memnew(Button(TTR("Check")));
	btn_check->set_tooltip_text(TTR("Fast syntax and type checking without producing binaries (cargo check)"));
	btn_check->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_check));
	top_bar->add_child(btn_check);

	btn_clean = memnew(Button(TTR("Clean")));
	btn_clean->set_tooltip_text(TTR("Remove target artifacts (cargo clean)"));
	btn_clean->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_clean));
	top_bar->add_child(btn_clean);

	btn_reload = memnew(Button(TTR("Reload Lib")));
	btn_reload->set_tooltip_text(TTR("Hot-reload the compiled native Rust shared library"));
	btn_reload->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_reload));
	top_bar->add_child(btn_reload);

	btn_open_toml = memnew(Button(TTR("Cargo.toml")));
	btn_open_toml->set_tooltip_text(TTR("Open project Cargo.toml in editor"));
	btn_open_toml->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_open_toml));
	top_bar->add_child(btn_open_toml);

	// Tabs
	tabs = memnew(TabContainer);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(tabs);

	// Tab 0: Build Log
	VBoxContainer *log_tab = memnew(VBoxContainer);
	log_tab->set_name(TTR("Build Log"));
	tabs->add_child(log_tab);

	log_view = memnew(RichTextLabel);
	log_view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	log_view->set_selection_enabled(true);
	log_tab->add_child(log_view);

	HBoxContainer *log_bottom_bar = memnew(HBoxContainer);
	log_tab->add_child(log_bottom_bar);

	build_status_label = memnew(Label(TTR("Ready.")));
	build_status_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	log_bottom_bar->add_child(build_status_label);

	btn_clear_log = memnew(Button(TTR("Clear Log")));
	btn_clear_log->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_clear_log));
	log_bottom_bar->add_child(btn_clear_log);

	// Tab 1: Dependencies
	VBoxContainer *deps_tab = memnew(VBoxContainer);
	deps_tab->set_name(TTR("Dependencies"));
	tabs->add_child(deps_tab);

	deps_list = memnew(ItemList);
	deps_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	deps_tab->add_child(deps_list);

	HBoxContainer *dep_add_bar = memnew(HBoxContainer);
	deps_tab->add_child(dep_add_bar);

	dep_name_edit = memnew(LineEdit);
	dep_name_edit->set_placeholder(TTR("Crate name (e.g. serde, rand, rayon, glam)"));
	dep_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dep_add_bar->add_child(dep_name_edit);

	dep_version_edit = memnew(LineEdit);
	dep_version_edit->set_placeholder(TTR("Version (optional, e.g. 1.0)"));
	dep_version_edit->set_custom_minimum_size(Size2(140 * EDSCALE, 0));
	dep_add_bar->add_child(dep_version_edit);

	btn_add_dep = memnew(Button(TTR("Add Dependency")));
	btn_add_dep->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_add_dep));
	dep_add_bar->add_child(btn_add_dep);

	btn_refresh_deps = memnew(Button(TTR("Refresh")));
	btn_refresh_deps->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_refresh_deps));
	dep_add_bar->add_child(btn_refresh_deps);

	// Tab 2: Scaffolding / Bridge info
	VBoxContainer *scaffold_tab = memnew(VBoxContainer);
	scaffold_tab->set_name(TTR("Project Setup"));
	tabs->add_child(scaffold_tab);

	scaffold_info = memnew(Label);
	scaffold_info->set_text(
			TTR("Zero-overhead native Rust integration for Godot.\n"
				"Project crate directory: res://rust/ (or root res:// if Cargo.toml exists).\n"
				"Export high-performance C-ABI functions using #[no_mangle] pub extern \"C\" fn.\n"
				"Pre-generated zero-cost bindings are available in src/godot_ffi.rs."));
	scaffold_info->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scaffold_tab->add_child(scaffold_info);

	btn_create_scaffold = memnew(Button(TTR("Create Rust Scaffolding (res://rust)")));
	btn_create_scaffold->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_create_scaffold));
	scaffold_tab->add_child(btn_create_scaffold);
}

CargoManagerDock::~CargoManagerDock() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
