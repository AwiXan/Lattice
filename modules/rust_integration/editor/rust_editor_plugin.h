/**************************************************************************/
/*  rust_editor_plugin.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "cargo_manager_dock.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"

class RustEditorPlugin : public EditorPlugin {
	GDCLASS(RustEditorPlugin, EditorPlugin);

	CargoManagerDock *cargo_dock = nullptr;
	Button *cargo_bottom_button = nullptr;

	HBoxContainer *toolbar_hb = nullptr;
	Button *toolbar_build_btn = nullptr;
	MenuButton *toolbar_menu_btn = nullptr;

	void _on_build_rust();
	void _on_generate_scaffolding();
	void _on_menu_item_selected(int p_id);
	void _on_open_cargo_dock();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Rust"; }

	RustEditorPlugin();
	~RustEditorPlugin();
};
