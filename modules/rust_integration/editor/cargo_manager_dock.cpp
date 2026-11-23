/**************************************************************************/
/*  cargo_manager_dock.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "cargo_manager_dock.h"

#include "../rust_manager.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/worker_thread_pool.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "rust_editor_plugin.h"

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
	_update_diagnostics();
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

void CargoManagerDock::_update_diagnostics() {
	RustManager *rm = RustManager::get_singleton();
	String crate_dir = rm ? rm->find_project_crate_dir() : "";
	String toml_path = crate_dir.is_empty() ? "" : crate_dir.path_join("Cargo.toml");

	String opt_dev = "0 (Debug default)";
	String opt_release = "3 (Default)";
	if (!toml_path.is_empty() && FileAccess::exists(toml_path)) {
		Ref<FileAccess> fa = FileAccess::open(toml_path, FileAccess::READ);
		if (fa.is_valid()) {
			String content = fa->get_as_utf8_string();
			if (content.contains("opt-level")) {
				int dev_idx = content.find("[profile.dev]");
				if (dev_idx != -1) {
					int opt_idx = content.find("opt-level", dev_idx);
					int next_sec = content.find("\n[", dev_idx + 13);
					if (opt_idx != -1 && (next_sec == -1 || opt_idx < next_sec)) {
						int eq = content.find("=", opt_idx);
						int nl = content.find("\n", eq);
						if (eq != -1 && nl != -1) {
							opt_dev = content.substr(eq + 1, nl - eq - 1).strip_edges();
						}
					}
				}
			}
		}
	}

	String backtrace_env = OS::get_singleton()->get_environment("RUST_BACKTRACE");
	bool backtrace_on = (backtrace_env == "full" || backtrace_env == "1");

	if (backtrace_status) {
		backtrace_status->set_text(backtrace_on ? TTR("RUST_BACKTRACE: ON (full)") : TTR("RUST_BACKTRACE: OFF"));
	}

	if (profile_info) {
		profile_info->clear();
		profile_info->push_bold();
		profile_info->add_text(TTR("Active Profile Settings (Cargo.toml):\n"));
		profile_info->pop();
		profile_info->add_text(vformat(TTR("  • [profile.dev] opt-level: %s\n"), opt_dev));
		profile_info->add_text(vformat(TTR("  • [profile.release] opt-level: %s\n\n"), opt_release));

		profile_info->push_bold();
		profile_info->add_text(TTR("Performance & FPS Optimization Tips:\n"));
		profile_info->pop();
		profile_info->add_text(TTR(
				"  1. In development, pure debug builds (opt-level = 0) can cause 15 FPS due to lack of inlining.\n"
				"     Click 'Dev-Optimized (opt-level = 2)' below to get 100+ FPS while retaining full debug symbols!\n"
				"  2. Follow the Read -> Compute -> Write pattern: do not query Node transforms inside tight loops.\n"
				"  3. Keep physics calculations inside pure Rust structs (10-20 ns/op) rather than engine FFI.\n"));
	}
}

void CargoManagerDock::_on_set_opt_level(int p_level) {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}
	String crate_dir = rm->find_project_crate_dir();
	if (crate_dir.is_empty()) {
		return;
	}
	String toml_path = crate_dir.path_join("Cargo.toml");
	if (!FileAccess::exists(toml_path)) {
		return;
	}

	Ref<FileAccess> fa = FileAccess::open(toml_path, FileAccess::READ);
	if (fa.is_null()) {
		return;
	}
	String content = fa->get_as_utf8_string();
	fa.unref();

	if (content.contains("[profile.dev]")) {
		int pos = content.find("[profile.dev]");
		int next_section = content.find("\n[", pos + 13);
		String new_section = vformat("[profile.dev]\nopt-level = %d\n", p_level);
		if (next_section != -1) {
			content = content.substr(0, pos) + new_section + content.substr(next_section + 1);
		} else {
			content = content.substr(0, pos) + new_section;
		}
	} else {
		content += vformat("\n[profile.dev]\nopt-level = %d\n", p_level);
	}

	Ref<FileAccess> fa_write = FileAccess::open(toml_path, FileAccess::WRITE);
	if (fa_write.is_valid()) {
		fa_write->store_string(content);
		fa_write.unref();
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(vformat(TTR("Set [profile.dev] opt-level = %d in Cargo.toml"), p_level), EditorToaster::SEVERITY_INFO);
		}
	}
	_update_diagnostics();
}

void CargoManagerDock::_on_setup_vscode() {
	String vscode_dir = "res://.vscode";
	DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(vscode_dir));

	String launch_path = vscode_dir.path_join("launch.json");
	String godot_bin = OS::get_singleton()->get_executable_path().replace("\\", "/");

	String launch_json = "{\n"
		"    \"version\": \"0.2.0\",\n"
		"    \"configurations\": [\n"
		"        {\n"
		"            \"name\": \"Godot: Debug Rust (CodeLLDB)\",\n"
		"            \"type\": \"lldb\",\n"
		"            \"request\": \"launch\",\n"
		"            \"program\": \"" + godot_bin + "\",\n"
		"            \"args\": [\"--path\", \"${workspaceFolder}\"],\n"
		"            \"cwd\": \"${workspaceFolder}\",\n"
		"            \"stopOnEntry\": false\n"
		"        },\n"
		"        {\n"
		"            \"name\": \"Godot: Debug Rust (C++/MSVC)\",\n"
		"            \"type\": \"cppvsdbg\",\n"
		"            \"request\": \"launch\",\n"
		"            \"program\": \"" + godot_bin + "\",\n"
		"            \"args\": [\"--path\", \"${workspaceFolder}\"],\n"
		"            \"cwd\": \"${workspaceFolder}\",\n"
		"            \"stopAtEntry\": false\n"
		"        }\n"
		"    ]\n"
		"}\n";

	Ref<FileAccess> fa = FileAccess::open(launch_path, FileAccess::WRITE);
	if (fa.is_valid()) {
		fa->store_string(launch_json);
		fa.unref();
		if (EditorToaster::get_singleton()) {
			EditorToaster::get_singleton()->popup_str(TTR("Created .vscode/launch.json for Rust debugging!"), EditorToaster::SEVERITY_INFO);
		}
	}
}

void CargoManagerDock::_on_toggle_backtrace() {
	String current = OS::get_singleton()->get_environment("RUST_BACKTRACE");
	if (current == "full" || current == "1") {
		OS::get_singleton()->set_environment("RUST_BACKTRACE", "0");
	} else {
		OS::get_singleton()->set_environment("RUST_BACKTRACE", "full");
	}
	if (EditorToaster::get_singleton()) {
		String status = (OS::get_singleton()->get_environment("RUST_BACKTRACE") == "full") ? "Enabled" : "Disabled";
		EditorToaster::get_singleton()->popup_str(vformat(TTR("RUST_BACKTRACE is now %s."), status), EditorToaster::SEVERITY_INFO);
	}
	_update_diagnostics();
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

	if (dock_spinner) {
		dock_spinner->set_spinning(false);
	}
	if (RustEditorPlugin::get_singleton()) {
		RustEditorPlugin::get_singleton()->set_building(false);
	}

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

	if (dock_spinner) {
		dock_spinner->set_spinning(true);
	}
	if (RustEditorPlugin::get_singleton()) {
		RustEditorPlugin::get_singleton()->set_building(true);
	}

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
	set_custom_minimum_size(Size2(0, 220 * EDSCALE));

	// Top control bar
	HBoxContainer *top_bar = memnew(HBoxContainer);
	add_child(top_bar);

	status_label = memnew(Label);
	status_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_bar->add_child(status_label);

	dock_spinner = memnew(RustProgressSpinner);
	dock_spinner->set_tooltip_text(TTR("Cargo compilation in progress..."));
	top_bar->add_child(dock_spinner);

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

	// Tab 2: Profiling & Optimization
	VBoxContainer *prof_tab = memnew(VBoxContainer);
	prof_tab->set_name(TTR("Profiling & Performance"));
	tabs->add_child(prof_tab);

	profile_info = memnew(RichTextLabel);
	profile_info->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	profile_info->set_selection_enabled(true);
	prof_tab->add_child(profile_info);

	HBoxContainer *prof_btn_bar = memnew(HBoxContainer);
	prof_tab->add_child(prof_btn_bar);

	Label *lbl_quick_set = memnew(Label(TTR("Quick Configure Dev Opt-Level:")));
	prof_btn_bar->add_child(lbl_quick_set);

	btn_opt_dev = memnew(Button(TTR("Debug (opt 0)")));
	btn_opt_dev->set_tooltip_text(TTR("Set [profile.dev] opt-level = 0 (fast compilation, slow runtime execution)"));
	btn_opt_dev->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_set_opt_level).bind(0));
	prof_btn_bar->add_child(btn_opt_dev);

	btn_opt_fast = memnew(Button(TTR("Dev-Optimized (opt 2) [Recommended]")));
	btn_opt_fast->set_tooltip_text(TTR("Set [profile.dev] opt-level = 2 (fixes 15 FPS in editor while retaining debug symbols)"));
	btn_opt_fast->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_set_opt_level).bind(2));
	prof_btn_bar->add_child(btn_opt_fast);

	btn_opt_release = memnew(Button(TTR("Release (opt 3)")));
	btn_opt_release->set_tooltip_text(TTR("Set [profile.dev] opt-level = 3 (maximum speed, slower compile times)"));
	btn_opt_release->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_set_opt_level).bind(3));
	prof_btn_bar->add_child(btn_opt_release);

	// Tab 3: Debugging & Tooling
	VBoxContainer *debug_tab = memnew(VBoxContainer);
	debug_tab->set_name(TTR("Debugging & Tools"));
	tabs->add_child(debug_tab);

	Label *debug_info = memnew(Label);
	debug_info->set_text(
			TTR("Native Rust Debugging Suite:\n\n"
				"• Setup VSCode Launch: generates .vscode/launch.json for CodeLLDB / MSVC.\n"
				"  You can set breakpoints directly inside any .rs source file and press F5.\n"
				"• RUST_BACKTRACE: provides full symbolic stack traces with source lines on crash.\n"));
	debug_info->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	debug_tab->add_child(debug_info);

	HBoxContainer *debug_btn_bar = memnew(HBoxContainer);
	debug_tab->add_child(debug_btn_bar);

	btn_setup_vscode = memnew(Button(TTR("Setup VSCode Debugger (.vscode/launch.json)")));
	btn_setup_vscode->set_tooltip_text(TTR("Generate or update VSCode debugger launch configurations for Rust"));
	btn_setup_vscode->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_setup_vscode));
	debug_btn_bar->add_child(btn_setup_vscode);

	btn_toggle_backtrace = memnew(Button(TTR("Toggle RUST_BACKTRACE")));
	btn_toggle_backtrace->set_tooltip_text(TTR("Toggle RUST_BACKTRACE between full and disabled"));
	btn_toggle_backtrace->connect(SceneStringName(pressed), callable_mp(this, &CargoManagerDock::_on_toggle_backtrace));
	debug_btn_bar->add_child(btn_toggle_backtrace);

	backtrace_status = memnew(Label(TTR("RUST_BACKTRACE: OFF")));
	debug_btn_bar->add_child(backtrace_status);

	// Tab 4: Scaffolding / Bridge info
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
