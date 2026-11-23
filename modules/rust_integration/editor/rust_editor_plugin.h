/**************************************************************************/
/*  rust_editor_plugin.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "cargo_manager_dock.h"
#include "editor/export/editor_export_plugin.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"

class RustProgressSpinner : public Control {
	GDCLASS(RustProgressSpinner, Control);

	float angle = 0.0f;
	bool spinning = false;

protected:
	void _notification(int p_what);

public:
	void set_spinning(bool p_spin);
	bool is_spinning() const { return spinning; }

	RustProgressSpinner();
};

class RustEditorExportPlugin : public EditorExportPlugin {
	GDCLASS(RustEditorExportPlugin, EditorExportPlugin);

protected:
	virtual void _export_begin(const HashSet<String> &p_features, bool p_debug, const String &p_path, int p_flags) override;
};

class RustEditorPlugin : public EditorPlugin {
	GDCLASS(RustEditorPlugin, EditorPlugin);

	static RustEditorPlugin *singleton;

	Ref<RustEditorExportPlugin> export_plugin;
	CargoManagerDock *cargo_dock = nullptr;
	Button *cargo_bottom_button = nullptr;

	HBoxContainer *toolbar_hb = nullptr;
	Button *toolbar_build_btn = nullptr;
	RustProgressSpinner *toolbar_spinner = nullptr;
	MenuButton *toolbar_menu_btn = nullptr;

	void _on_build_rust();
	void _on_generate_scaffolding();
	void _on_menu_item_selected(int p_id);
	void _on_open_cargo_dock();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static RustEditorPlugin *get_singleton() { return singleton; }
	virtual String get_plugin_name() const override { return "Rust"; }

	void set_building(bool p_building);

	RustEditorPlugin();
	~RustEditorPlugin();
};
