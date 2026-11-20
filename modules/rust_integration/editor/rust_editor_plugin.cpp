/**************************************************************************/
/*  rust_editor_plugin.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_editor_plugin.h"

#include "../rust_manager.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"

void RustEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_build_rust"), &RustEditorPlugin::_on_build_rust);
	ClassDB::bind_method(D_METHOD("_on_generate_scaffolding"), &RustEditorPlugin::_on_generate_scaffolding);
}

void RustEditorPlugin::_on_build_rust() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}

	String output;
	int exit_code = 0;
	Error err = rm->build_crate(false, &output, &exit_code);
	if (err == OK && exit_code == 0) {
		print_line("[Rust] Project built and loaded successfully!");
	} else {
		print_error(vformat("[Rust] Build failed (exit code %d):\n%s", exit_code, output));
		if (EditorNode::get_singleton()) {
			EditorNode::get_singleton()->show_warning(TTR("Rust build failed! Check output console for details."));
		}
	}
}

void RustEditorPlugin::_on_generate_scaffolding() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}
	Error err = rm->generate_scaffolding("res://rust");
	if (err == OK) {
		print_line("[Rust] Scaffolding created in res://rust/");
	}
}

void RustEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		add_tool_menu_item(TTR("Build Rust Crates (Cargo)"), callable_mp(this, &RustEditorPlugin::_on_build_rust));
		add_tool_menu_item(TTR("Create Rust Project Scaffolding"), callable_mp(this, &RustEditorPlugin::_on_generate_scaffolding));
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		remove_tool_menu_item(TTR("Build Rust Crates (Cargo)"));
		remove_tool_menu_item(TTR("Create Rust Project Scaffolding"));
	}
}

RustEditorPlugin::RustEditorPlugin() {
}

RustEditorPlugin::~RustEditorPlugin() {
}
