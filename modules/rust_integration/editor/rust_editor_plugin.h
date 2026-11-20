/**************************************************************************/
/*  rust_editor_plugin.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "editor/plugins/editor_plugin.h"

class RustEditorPlugin : public EditorPlugin {
	GDCLASS(RustEditorPlugin, EditorPlugin);

	PopupMenu *project_menu = nullptr;

	void _on_build_rust();
	void _on_generate_scaffolding();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Rust"; }

	RustEditorPlugin();
	~RustEditorPlugin();
};
