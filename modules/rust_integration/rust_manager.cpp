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
#include "rust_ffi.h"
#include "scene/2d/node_2d.h"

#include <cmath>
#include <queue>
#include <vector>

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

	ClassDB::bind_method(D_METHOD("run_rust_math", "iterations"), &RustManager::run_rust_math, DEFVAL(100));
	ClassDB::bind_method(D_METHOD("run_rust_spawn", "count", "parent"), &RustManager::run_rust_spawn, DEFVAL(5000), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("run_rust_pathfinding", "grid_size", "runs"), &RustManager::run_rust_pathfinding, DEFVAL(128), DEFVAL(50));

	ClassDB::bind_method(D_METHOD("run_cpp_math", "iterations"), &RustManager::run_cpp_math, DEFVAL(100));
	ClassDB::bind_method(D_METHOD("run_cpp_spawn", "count", "parent"), &RustManager::run_cpp_spawn, DEFVAL(5000), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("run_cpp_pathfinding", "grid_size", "runs"), &RustManager::run_cpp_pathfinding, DEFVAL(128), DEFVAL(50));
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
					"    pub fn godot_rust_node2d_create() -> u64;\n"
					"    pub fn godot_rust_node_add_child(parent_id: u64, child_id: u64);\n"
					"    pub fn godot_rust_node_remove_child(parent_id: u64, child_id: u64);\n"
					"    pub fn godot_rust_node_free(instance_id: u64);\n"
					"    pub fn godot_rust_node2d_get_position(instance_id: u64, out_pos: *mut f32) -> i32;\n"
					"    pub fn godot_rust_node2d_set_position(instance_id: u64, x: f32, y: f32) -> i32;\n"
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
	search_subpaths.push_back("target/release/godot_rust_scripts.dll");
	search_subpaths.push_back("target/debug/godot_rust_scripts.dll");
#elif defined(MACOS_ENABLED)
	search_subpaths.push_back("target/release/libgodot_rust_scripts.dylib");
	search_subpaths.push_back("target/debug/libgodot_rust_scripts.dylib");
#else
	search_subpaths.push_back("target/release/libgodot_rust_scripts.so");
	search_subpaths.push_back("target/debug/libgodot_rust_scripts.so");
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

		struct GodotRustApiTable {
			uint64_t (*godot_rust_node2d_create)();
			void (*godot_rust_node_add_child)(uint64_t, uint64_t);
			void (*godot_rust_node_remove_child)(uint64_t, uint64_t);
			void (*godot_rust_node_free)(uint64_t);
			int32_t (*godot_rust_node2d_get_position)(uint64_t, float[2]);
			int32_t (*godot_rust_node2d_set_position)(uint64_t, float, float);
			void (*godot_rust_print)(const char *);
		};
		typedef void (*InitApiFunc)(const GodotRustApiTable *);
		InitApiFunc init_api = (InitApiFunc)get_symbol("init_godot_rust_api");
		if (init_api) {
			GodotRustApiTable table;
			table.godot_rust_node2d_create = godot_rust_node2d_create;
			table.godot_rust_node_add_child = godot_rust_node_add_child;
			table.godot_rust_node_remove_child = godot_rust_node_remove_child;
			table.godot_rust_node_free = godot_rust_node_free;
			table.godot_rust_node2d_get_position = godot_rust_node2d_get_position;
			table.godot_rust_node2d_set_position = godot_rust_node2d_set_position;
			table.godot_rust_print = godot_rust_print;
			init_api(&table);
		}
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

struct RustBenchResult {
	double time_ms;
	int64_t checksum;
};

typedef void (*RustBenchMathFunc)(int64_t iterations, RustBenchResult *out);
typedef void (*RustBenchSpawnFunc)(int64_t count, uint64_t parent_id, RustBenchResult *out);
typedef void (*RustBenchPathfindingFunc)(int64_t grid_size, int64_t runs, RustBenchResult *out);

Dictionary RustManager::run_rust_math(int64_t p_iterations) {
	Dictionary res;
	res["time_ms"] = 0.0;
	res["checksum"] = (int64_t)0;
	res["status"] = "Error";

	if (!is_library_loaded()) {
		reload_library();
	}
	if (!is_library_loaded()) {
		res["status"] = "Rust library not loaded. Build with Cargo first!";
		return res;
	}

	RustBenchMathFunc fn = (RustBenchMathFunc)get_symbol("rust_benchmark_math_ext");
	if (!fn) {
		res["status"] = "Symbol rust_benchmark_math_ext not found in Rust library.";
		return res;
	}

	RustBenchResult r;
	r.time_ms = 0.0;
	r.checksum = 0;
	fn(p_iterations, &r);

	res["time_ms"] = r.time_ms;
	res["checksum"] = r.checksum;
	res["status"] = "OK";
	return res;
}

Dictionary RustManager::run_rust_spawn(int64_t p_count, Node *p_parent) {
	Dictionary res;
	res["time_ms"] = 0.0;
	res["checksum"] = (int64_t)0;
	res["status"] = "Error";

	if (!is_library_loaded()) {
		reload_library();
	}
	if (!is_library_loaded()) {
		res["status"] = "Rust library not loaded. Build with Cargo first!";
		return res;
	}

	RustBenchSpawnFunc fn = (RustBenchSpawnFunc)get_symbol("rust_benchmark_spawn_ext");
	if (!fn) {
		res["status"] = "Symbol rust_benchmark_spawn_ext not found in Rust library.";
		return res;
	}

	uint64_t parent_id = p_parent ? (uint64_t)p_parent->get_instance_id() : (uint64_t)0;
	RustBenchResult r;
	r.time_ms = 0.0;
	r.checksum = 0;
	fn(p_count, parent_id, &r);

	res["time_ms"] = r.time_ms;
	res["checksum"] = r.checksum;
	res["status"] = "OK";
	return res;
}

Dictionary RustManager::run_rust_pathfinding(int64_t p_grid_size, int64_t p_runs) {
	Dictionary res;
	res["time_ms"] = 0.0;
	res["checksum"] = (int64_t)0;
	res["status"] = "Error";

	if (!is_library_loaded()) {
		reload_library();
	}
	if (!is_library_loaded()) {
		res["status"] = "Rust library not loaded. Build with Cargo first!";
		return res;
	}

	RustBenchPathfindingFunc fn = (RustBenchPathfindingFunc)get_symbol("rust_benchmark_pathfinding_ext");
	if (!fn) {
		res["status"] = "Symbol rust_benchmark_pathfinding_ext not found in Rust library.";
		return res;
	}

	RustBenchResult r;
	r.time_ms = 0.0;
	r.checksum = 0;
	fn(p_grid_size, p_runs, &r);

	res["time_ms"] = r.time_ms;
	res["checksum"] = r.checksum;
	res["status"] = "OK";
	return res;
}

Dictionary RustManager::run_cpp_math(int64_t p_iterations) {
	Dictionary res;
	uint64_t start_time = OS::get_singleton()->get_ticks_usec();

	int width = 300;
	int height = 300;
	int max_iter = (int)p_iterations;
	int64_t escaped_count = 0;

	for (int y = 0; y < height; y++) {
		double ci = -1.5 + (double)y / (double)height * 3.0;
		for (int x = 0; x < width; x++) {
			double cr = -2.0 + (double)x / (double)width * 3.0;
			double zr = 0.0;
			double zi = 0.0;
			for (int iter = 0; iter < max_iter; iter++) {
				double zr2 = zr * zr;
				double zi2 = zi * zi;
				if (zr2 + zi2 > 4.0) {
					escaped_count++;
					break;
				}
				zi = 2.0 * zr * zi + ci;
				zr = zr2 - zi2 + cr;
			}
		}
	}

	const int LIMIT = 1000000;
	std::vector<bool> is_prime(LIMIT + 1, true);
	is_prime[0] = is_prime[1] = false;
	for (int p = 2; p * p <= LIMIT; p++) {
		if (is_prime[p]) {
			for (int i = p * p; i <= LIMIT; i += p) {
				is_prime[i] = false;
			}
		}
	}
	int64_t prime_count = 0;
	for (int p = 2; p <= LIMIT; p++) {
		if (is_prime[p]) {
			prime_count++;
		}
	}

	uint64_t end_time = OS::get_singleton()->get_ticks_usec();
	res["time_ms"] = (double)(end_time - start_time) / 1000.0;
	res["checksum"] = escaped_count + prime_count;
	res["status"] = "OK";
	return res;
}

Dictionary RustManager::run_cpp_spawn(int64_t p_count, Node *p_parent) {
	Dictionary res;
	bool created_parent = false;
	if (!p_parent) {
		p_parent = memnew(Node2D);
		created_parent = true;
	}

	uint64_t start_time = OS::get_singleton()->get_ticks_usec();
	std::vector<Node2D *> nodes;
	nodes.reserve(p_count);

	for (int64_t i = 0; i < p_count; i++) {
		Node2D *n = memnew(Node2D);
		p_parent->add_child(n);
		n->set_position(Point2((float)i * 0.1f, (float)i * 0.1f));
		nodes.push_back(n);
	}

	for (int64_t i = 0; i < p_count; i++) {
		Point2 pos = nodes[i]->get_position();
		nodes[i]->set_position(Point2(pos.x + 1.0f, pos.y + 1.0f));
	}

	for (int64_t i = 0; i < p_count; i++) {
		p_parent->remove_child(nodes[i]);
		memdelete(nodes[i]);
	}

	uint64_t end_time = OS::get_singleton()->get_ticks_usec();

	if (created_parent) {
		memdelete(p_parent);
	}

	res["time_ms"] = (double)(end_time - start_time) / 1000.0;
	res["checksum"] = p_count;
	res["status"] = "OK";
	return res;
}

struct CppAStarNode {
	int x, y;
	int g, f;
	bool operator>(const CppAStarNode &other) const {
		return f > other.f;
	}
};

Dictionary RustManager::run_cpp_pathfinding(int64_t p_grid_size, int64_t p_runs) {
	Dictionary res;
	uint64_t start_time = OS::get_singleton()->get_ticks_usec();
	int size = (int)p_grid_size;

	std::vector<bool> walls(size * size, false);
	uint32_t seed = 1337;
	for (int i = 0; i < size * size; i++) {
		seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
		if ((seed % 100) < 25) {
			walls[i] = true;
		}
	}

	int64_t total_path_len = 0;
	const int dx[4] = { 0, 1, 0, -1 };
	const int dy[4] = { -1, 0, 1, 0 };

	for (int run = 0; run < (int)p_runs; run++) {
		int sx = (run * 37) % (size - 4) + 2;
		int sy = (run * 47) % (size - 4) + 2;
		int gx = (run * 59) % (size - 4) + 2;
		int gy = (run * 71) % (size - 4) + 2;

		walls[sy * size + sx] = false;
		walls[gy * size + gx] = false;

		std::priority_queue<CppAStarNode, std::vector<CppAStarNode>, std::greater<CppAStarNode>> open;
		std::vector<int> dist(size * size, 10000000);
		dist[sy * size + sx] = 0;

		int h0 = std::abs(sx - gx) + std::abs(sy - gy);
		open.push({ sx, sy, 0, h0 });

		while (!open.empty()) {
			CppAStarNode cur = open.top();
			open.pop();

			if (cur.x == gx && cur.y == gy) {
				total_path_len += cur.g;
				break;
			}
			if (cur.g > dist[cur.y * size + cur.x]) {
				continue;
			}

			for (int d = 0; d < 4; d++) {
				int nx = cur.x + dx[d];
				int ny = cur.y + dy[d];
				if (nx >= 0 && nx < size && ny >= 0 && ny < size && !walls[ny * size + nx]) {
					int ng = cur.g + 1;
					if (ng < dist[ny * size + nx]) {
						dist[ny * size + nx] = ng;
						int h = std::abs(nx - gx) + std::abs(ny - gy);
						open.push({ nx, ny, ng, ng + h });
					}
				}
			}
		}
	}

	uint64_t end_time = OS::get_singleton()->get_ticks_usec();
	res["time_ms"] = (double)(end_time - start_time) / 1000.0;
	res["checksum"] = total_path_len;
	res["status"] = "OK";
	return res;
}
