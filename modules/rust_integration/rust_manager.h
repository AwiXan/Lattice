/**************************************************************************/
/*  rust_manager.h                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/class_db.h"
#include "core/os/os.h"

class RustManager : public Object {
	GDCLASS(RustManager, Object);

	static RustManager *singleton;

	void *library_handle = nullptr;
	String loaded_library_path;
	String cargo_path = "cargo";
	bool cargo_available = false;
	String cargo_version;

	void _check_cargo();

	Error _clean_crate_bind() { return clean_crate(); }
	Error _check_crate_bind() { return check_crate(); }
	Error _add_dependency_bind(const String &p_crate_name, const String &p_version = "") { return add_dependency(p_crate_name, p_version); }

protected:
	static void _bind_methods();

public:
	static RustManager *get_singleton() { return singleton; }

	bool is_cargo_available() const { return cargo_available; }
	String get_cargo_version() const { return cargo_version; }

	String find_project_crate_dir() const;
	bool has_rust_project() const;

	Error generate_scaffolding(const String &p_target_dir = "res://rust");
	Error build_crate(bool p_release = false, String *r_output = nullptr, int *r_exit_code = nullptr);
	Error clean_crate(String *r_output = nullptr, int *r_exit_code = nullptr);
	Error check_crate(String *r_output = nullptr, int *r_exit_code = nullptr);
	Error add_dependency(const String &p_crate_name, const String &p_version = "", String *r_output = nullptr, int *r_exit_code = nullptr);
	PackedStringArray get_dependencies() const;
	Error reload_library();

	bool is_library_loaded() const { return library_handle != nullptr; }
	String get_loaded_library_path() const { return loaded_library_path; }
	void *get_symbol(const String &p_name);

	RustManager();
	~RustManager();
};
