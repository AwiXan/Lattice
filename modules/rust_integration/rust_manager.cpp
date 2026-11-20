/**************************************************************************/
/*  rust_manager.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_manager.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"

RustManager *RustManager::singleton = nullptr;

void RustManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_cargo_available"), &RustManager::is_cargo_available);
	ClassDB::bind_method(D_METHOD("get_cargo_version"), &RustManager::get_cargo_version);
	ClassDB::bind_method(D_METHOD("has_rust_project"), &RustManager::has_rust_project);
	ClassDB::bind_method(D_METHOD("generate_scaffolding", "target_dir"), &RustManager::generate_scaffolding, DEFVAL("res://rust"));
	ClassDB::bind_method(D_METHOD("clean_crate"), &RustManager::_clean_crate_bind);
	ClassDB::bind_method(D_METHOD("check_crate"), &RustManager::_check_crate_bind);
	ClassDB::bind_method(D_METHOD("add_dependency", "crate_name", "version"), &RustManager::_add_dependency_bind, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_dependencies"), &RustManager::get_dependencies);
	ClassDB::bind_method(D_METHOD("reload_library"), &RustManager::reload_library);
	ClassDB::bind_method(D_METHOD("is_library_loaded"), &RustManager::is_library_loaded);
	ClassDB::bind_method(D_METHOD("get_loaded_library_path"), &RustManager::get_loaded_library_path);
}

void RustManager::_check_cargo() {
	if (GLOBAL_GET("rust/general/cargo_path").get_type() == Variant::STRING) {
		String custom_path = GLOBAL_GET("rust/general/cargo_path");
		if (!custom_path.is_empty()) {
			cargo_path = custom_path;
		}
	}

	List<String> args;
	args.push_back("--version");
	String output;
	int exit_code = -1;

	Error err = OS::get_singleton()->execute(cargo_path, args, &output, &exit_code, true);
	if (err == OK && exit_code == 0) {
		cargo_available = true;
		cargo_version = output.strip_edges();
	} else {
		cargo_available = false;
		cargo_version = "";
	}
}

String RustManager::find_project_crate_dir() const {
	String path1 = "res://rust/Cargo.toml";
	if (FileAccess::exists(path1)) {
		return "res://rust";
	}
	String path2 = "res://Cargo.toml";
	if (FileAccess::exists(path2)) {
		return "res://";
	}
	return String();
}

bool RustManager::has_rust_project() const {
	return !find_project_crate_dir().is_empty();
}

Error RustManager::generate_scaffolding(const String &p_target_dir) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (!da->dir_exists(p_target_dir)) {
		da->make_dir_recursive(p_target_dir);
	}
	da->make_dir_recursive(p_target_dir.path_join("src"));

	// 1. Cargo.toml
	String cargo_toml_path = p_target_dir.path_join("Cargo.toml");
	if (!FileAccess::exists(cargo_toml_path)) {
		Ref<FileAccess> f = FileAccess::open(cargo_toml_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_string(
					"[package]\n"
					"name = \"godot_rust_scripts\"\n"
					"version = \"0.1.0\"\n"
					"edition = \"2021\"\n\n"
					"[lib]\n"
					"crate-type = [\"cdylib\"]\n\n"
					"[dependencies]\n");
		}
	}

	// 2. src/godot_ffi.rs (Zero-overhead C-ABI bindings)
	String godot_ffi_path = p_target_dir.path_join("src/godot_ffi.rs");
	if (!FileAccess::exists(godot_ffi_path)) {
		Ref<FileAccess> f = FileAccess::open(godot_ffi_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_string(
					"// Zero-overhead Godot Engine Rust FFI Bridge\n"
					"#![allow(dead_code)]\n\n"
					"extern \"C\" {\n"
					"    pub fn godot_rust_api_version() -> u32;\n"
					"    pub fn godot_rust_print(msg: *const u8);\n"
					"    pub fn godot_rust_print_error(msg: *const u8, func: *const u8, file: *const u8, line: i32);\n"
					"    pub fn godot_rust_packed_byte_array_read(handle: *const std::ffi::c_void) -> *const u8;\n"
					"    pub fn godot_rust_packed_byte_array_write(handle: *mut std::ffi::c_void) -> *mut u8;\n"
					"    pub fn godot_rust_packed_byte_array_size(handle: *const std::ffi::c_void) -> i64;\n"
					"    pub fn godot_rust_node3d_get_position(instance_id: u64, out_pos: *mut f32) -> i32;\n"
					"    pub fn godot_rust_node3d_set_position(instance_id: u64, x: f32, y: f32, z: f32) -> i32;\n"
					"    pub fn godot_rust_thread_pool_add_task(func: extern \"C\" fn(*mut std::ffi::c_void), userdata: *mut std::ffi::c_void, high_priority: bool) -> i64;\n"
					"}\n\n"
					"pub fn print(s: &str) {\n"
					"    let c_str = std::ffi::CString::new(s).unwrap_or_default();\n"
					"    unsafe { godot_rust_print(c_str.as_ptr() as *const u8); }\n"
					"}\n");
		}
	}

	// 3. src/lib.rs
	String lib_rs_path = p_target_dir.path_join("src/lib.rs");
	if (!FileAccess::exists(lib_rs_path)) {
		Ref<FileAccess> f = FileAccess::open(lib_rs_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_string(
					"mod godot_ffi;\n"
					"use godot_ffi::*;\n\n"
					"#[no_mangle]\n"
					"pub extern \"C\" fn rust_node_ready(instance_id: u64) {\n"
					"    print(&format!(\"Rust Node initialized! Instance ID: {}\", instance_id));\n"
					"}\n\n"
					"#[no_mangle]\n"
					"pub extern \"C\" fn rust_node_process(instance_id: u64, delta: f64) {\n"
					"    // Fast, zero-overhead process loop executed in native Rust\n"
					"}\n");
		}
	}

	return OK;
}

Error RustManager::build_crate(bool p_release, String *r_output, int *r_exit_code) {
	if (!cargo_available) {
		_check_cargo();
		if (!cargo_available) {
			if (r_output) {
				*r_output = "Error: Cargo was not found on system PATH. Please install Rust from https://rustup.rs";
			}
			if (r_exit_code) {
				*r_exit_code = -1;
			}
			return ERR_CANT_CREATE;
		}
	}

	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		generate_scaffolding("res://rust");
		crate_dir = "res://rust";
	}

	String global_crate_dir = ProjectSettings::get_singleton()->globalize_path(crate_dir);

	List<String> args;
	args.push_back("build");
	args.push_back("--manifest-path");
	args.push_back(global_crate_dir.path_join("Cargo.toml"));

	if (p_release) {
		args.push_back("--release");
	}

	String output;
	int exit_code = -1;
	Error err = OS::get_singleton()->execute(cargo_path, args, &output, &exit_code, true);

	if (r_output) {
		*r_output = output;
	}
	if (r_exit_code) {
		*r_exit_code = exit_code;
	}

	if (err == OK && exit_code == 0) {
		reload_library();
		return OK;
	}

	return FAILED;
}

Error RustManager::clean_crate(String *r_output, int *r_exit_code) {
	if (!cargo_available) {
		_check_cargo();
		if (!cargo_available) {
			if (r_output) {
				*r_output = "Error: Cargo was not found on system PATH.";
			}
			if (r_exit_code) {
				*r_exit_code = -1;
			}
			return ERR_CANT_CREATE;
		}
	}

	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		return ERR_FILE_NOT_FOUND;
	}

	String global_crate_dir = ProjectSettings::get_singleton()->globalize_path(crate_dir);
	List<String> args;
	args.push_back("clean");
	args.push_back("--manifest-path");
	args.push_back(global_crate_dir.path_join("Cargo.toml"));

	String output;
	int exit_code = -1;
	Error err = OS::get_singleton()->execute(cargo_path, args, &output, &exit_code, true);
	if (r_output) {
		*r_output = output;
	}
	if (r_exit_code) {
		*r_exit_code = exit_code;
	}
	return (err == OK && exit_code == 0) ? OK : FAILED;
}

Error RustManager::check_crate(String *r_output, int *r_exit_code) {
	if (!cargo_available) {
		_check_cargo();
		if (!cargo_available) {
			if (r_output) {
				*r_output = "Error: Cargo was not found on system PATH.";
			}
			if (r_exit_code) {
				*r_exit_code = -1;
			}
			return ERR_CANT_CREATE;
		}
	}

	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		return ERR_FILE_NOT_FOUND;
	}

	String global_crate_dir = ProjectSettings::get_singleton()->globalize_path(crate_dir);
	List<String> args;
	args.push_back("check");
	args.push_back("--manifest-path");
	args.push_back(global_crate_dir.path_join("Cargo.toml"));

	String output;
	int exit_code = -1;
	Error err = OS::get_singleton()->execute(cargo_path, args, &output, &exit_code, true);
	if (r_output) {
		*r_output = output;
	}
	if (r_exit_code) {
		*r_exit_code = exit_code;
	}
	return (err == OK && exit_code == 0) ? OK : FAILED;
}

Error RustManager::add_dependency(const String &p_crate_name, const String &p_version, String *r_output, int *r_exit_code) {
	if (p_crate_name.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		generate_scaffolding("res://rust");
		crate_dir = "res://rust";
	}

	String global_crate_dir = ProjectSettings::get_singleton()->globalize_path(crate_dir);
	String toml_path = global_crate_dir.path_join("Cargo.toml");

	// Try cargo add first if available
	if (cargo_available) {
		List<String> args;
		args.push_back("add");
		args.push_back("--manifest-path");
		args.push_back(toml_path);
		if (!p_version.is_empty()) {
			args.push_back(p_crate_name + "@" + p_version);
		} else {
			args.push_back(p_crate_name);
		}
		String output;
		int exit_code = -1;
		Error err = OS::get_singleton()->execute(cargo_path, args, &output, &exit_code, true);
		if (err == OK && exit_code == 0) {
			if (r_output) {
				*r_output = output;
			}
			if (r_exit_code) {
				*r_exit_code = exit_code;
			}
			return OK;
		}
	}

	// Fallback: directly append or insert into Cargo.toml
	Error err;
	String content = FileAccess::get_file_as_string(crate_dir.path_join("Cargo.toml"), &err);
	if (err == OK) {
		String dep_entry = p_version.is_empty() ? vformat("%s = \"*\"\n", p_crate_name) : vformat("%s = \"%s\"\n", p_crate_name, p_version);
		int dep_idx = content.find("[dependencies]");
		if (dep_idx != -1) {
			int insert_pos = dep_idx + String("[dependencies]").length();
			content = content.insert(insert_pos, "\n" + dep_entry);
		} else {
			content += "\n[dependencies]\n" + dep_entry;
		}
		Ref<FileAccess> f = FileAccess::open(crate_dir.path_join("Cargo.toml"), FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_string(content);
			if (r_output) {
				*r_output = vformat("Added %s to Cargo.toml", p_crate_name);
			}
			if (r_exit_code) {
				*r_exit_code = 0;
			}
			return OK;
		}
	}
	return FAILED;
}

PackedStringArray RustManager::get_dependencies() const {
	PackedStringArray deps;
	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		return deps;
	}
	Error err;
	String content = FileAccess::get_file_as_string(crate_dir.path_join("Cargo.toml"), &err);
	if (err != OK) {
		return deps;
	}
	Vector<String> lines = content.split("\n");
	bool in_deps = false;
	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();
		if (line.is_empty() || line.begins_with("#")) {
			continue;
		}
		if (line.begins_with("[")) {
			in_deps = (line == "[dependencies]");
			continue;
		}
		if (in_deps) {
			deps.push_back(line);
		}
	}
	return deps;
}

Error RustManager::reload_library() {
	if (library_handle) {
		OS::get_singleton()->close_dynamic_library(library_handle);
		library_handle = nullptr;
		loaded_library_path = "";
	}

	String crate_dir = find_project_crate_dir();
	if (crate_dir.is_empty()) {
		return ERR_FILE_NOT_FOUND;
	}

	String global_crate_dir = ProjectSettings::get_singleton()->globalize_path(crate_dir);

	// Detect target dynamic library
	PackedStringArray search_subpaths;
#if defined(WINDOWS_ENABLED)
	search_subpaths.push_back("target/debug/godot_rust_scripts.dll");
	search_subpaths.push_back("target/release/godot_rust_scripts.dll");
#elif defined(MACOS_ENABLED)
	search_subpaths.push_back("target/debug/libgodot_rust_scripts.dylib");
	search_subpaths.push_back("target/release/libgodot_rust_scripts.dylib");
#else
	search_subpaths.push_back("target/debug/libgodot_rust_scripts.so");
	search_subpaths.push_back("target/release/libgodot_rust_scripts.so");
#endif

	String lib_full_path;
	for (int i = 0; i < search_subpaths.size(); i++) {
		String test_path = global_crate_dir.path_join(search_subpaths[i]);
		if (FileAccess::exists(test_path)) {
			lib_full_path = test_path;
			break;
		}
	}

	if (lib_full_path.is_empty()) {
		return ERR_FILE_NOT_FOUND;
	}

	Error err = OS::get_singleton()->open_dynamic_library(lib_full_path, library_handle);
	if (err == OK && library_handle) {
		loaded_library_path = lib_full_path;
		print_line(vformat("[RustManager] Loaded native Rust library: %s", lib_full_path));
		return OK;
	}

	return err;
}

void *RustManager::get_symbol(const String &p_name) {
	if (!library_handle) {
		return nullptr;
	}
	void *symbol = nullptr;
	Error err = OS::get_singleton()->get_dynamic_library_symbol_handle(library_handle, p_name, symbol, true);
	if (err == OK) {
		return symbol;
	}
	return nullptr;
}

RustManager::RustManager() {
	singleton = this;
	_check_cargo();
}

RustManager::~RustManager() {
	if (library_handle) {
		OS::get_singleton()->close_dynamic_library(library_handle);
		library_handle = nullptr;
	}
	singleton = nullptr;
}
