/**************************************************************************/
/*  rust_editor_plugin.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_editor_plugin.h"

#include "../rust_manager.h"
#include "cargo_manager_dock.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"

enum MenuItems {
	MENU_BUILD_DEBUG = 0,
	MENU_BUILD_RELEASE,
	MENU_CHECK,
	MENU_CLEAN,
	MENU_OPEN_DOCK,
	MENU_OPEN_TOML,
	MENU_RELOAD_LIB,
	MENU_SCAFFOLDING,
};

void RustEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_build_rust"), &RustEditorPlugin::_on_build_rust);
	ClassDB::bind_method(D_METHOD("_on_generate_scaffolding"), &RustEditorPlugin::_on_generate_scaffolding);
	ClassDB::bind_method(D_METHOD("_on_menu_item_selected", "id"), &RustEditorPlugin::_on_menu_item_selected);
	ClassDB::bind_method(D_METHOD("_on_open_cargo_dock"), &RustEditorPlugin::_on_open_cargo_dock);
}

void RustEditorPlugin::_on_build_rust() {
	if (cargo_dock) {
		cargo_dock->trigger_build(false);
	} else {
		RustManager *rm = RustManager::get_singleton();
		if (rm) {
			String output;
			int exit_code = 0;
			rm->build_crate(false, &output, &exit_code);
		}
	}
}

void RustEditorPlugin::_on_generate_scaffolding() {
	RustManager *rm = RustManager::get_singleton();
	if (rm) {
		rm->generate_scaffolding("res://rust");
	}
}

void RustEditorPlugin::_on_open_cargo_dock() {
	if (cargo_dock) {
		make_bottom_panel_item_visible(cargo_dock);
	}
}

void RustEditorPlugin::_on_menu_item_selected(int p_id) {
	switch (p_id) {
		case MENU_BUILD_DEBUG: {
			if (cargo_dock) {
				cargo_dock->trigger_build(false);
			}
		} break;
		case MENU_BUILD_RELEASE: {
			if (cargo_dock) {
				cargo_dock->trigger_build(true);
			}
		} break;
		case MENU_CHECK: {
			if (cargo_dock) {
				cargo_dock->trigger_build(false);
			} else {
				RustManager *rm = RustManager::get_singleton();
				if (rm) {
					String out;
					int code = 0;
					rm->check_crate(&out, &code);
				}
			}
		} break;
		case MENU_CLEAN: {
			RustManager *rm = RustManager::get_singleton();
			if (rm) {
				String out;
				int code = 0;
				rm->clean_crate(&out, &code);
			}
		} break;
		case MENU_OPEN_DOCK: {
			_on_open_cargo_dock();
		} break;
		case MENU_OPEN_TOML: {
			RustManager *rm = RustManager::get_singleton();
			if (rm) {
				String dir = rm->find_project_crate_dir();
				if (!dir.is_empty() && ScriptEditor::get_singleton()) {
					ScriptEditor::get_singleton()->open_file(dir.path_join("Cargo.toml"));
				}
			}
		} break;
		case MENU_RELOAD_LIB: {
			RustManager *rm = RustManager::get_singleton();
			if (rm) {
				rm->reload_library();
			}
		} break;
		case MENU_SCAFFOLDING: {
			_on_generate_scaffolding();
		} break;
	}
}

void RustEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		// 1. Cargo Manager Bottom Panel
		cargo_dock = memnew(CargoManagerDock);
		cargo_bottom_button = add_control_to_bottom_panel(cargo_dock, "Cargo");

		// 2. Toolbar controls
		toolbar_hb = memnew(HBoxContainer);
		toolbar_hb->set_alignment(BoxContainer::ALIGNMENT_CENTER);

		toolbar_build_btn = memnew(Button(TTR("Build Rust")));
		toolbar_build_btn->set_tooltip_text(TTR("Build Rust crates with Cargo and hot-reload library"));
		toolbar_build_btn->connect(SceneStringName(pressed), callable_mp(this, &RustEditorPlugin::_on_build_rust));
		if (EditorNode::get_singleton() && EditorNode::get_singleton()->get_gui_base()) {
			Control *gui = EditorNode::get_singleton()->get_gui_base();
			if (gui->has_theme_icon(SNAME("Tools"), EditorStringName(EditorIcons))) {
				toolbar_build_btn->set_button_icon(gui->get_theme_icon(SNAME("Tools"), EditorStringName(EditorIcons)));
			}
		}
		toolbar_hb->add_child(toolbar_build_btn);

		toolbar_menu_btn = memnew(MenuButton);
		toolbar_menu_btn->set_tooltip_text(TTR("Cargo Build Options"));
		if (EditorNode::get_singleton() && EditorNode::get_singleton()->get_gui_base()) {
			Control *gui = EditorNode::get_singleton()->get_gui_base();
			if (gui->has_theme_icon(SNAME("GuiTabMenuHl"), EditorStringName(EditorIcons))) {
				toolbar_menu_btn->set_button_icon(gui->get_theme_icon(SNAME("GuiTabMenuHl"), EditorStringName(EditorIcons)));
			}
		}
		PopupMenu *popup = toolbar_menu_btn->get_popup();
		popup->add_item(TTR("Build Debug (Cargo)"), MENU_BUILD_DEBUG);
		popup->add_item(TTR("Build Release (Cargo)"), MENU_BUILD_RELEASE);
		popup->add_item(TTR("Cargo Check"), MENU_CHECK);
		popup->add_item(TTR("Cargo Clean"), MENU_CLEAN);
		popup->add_separator();
		popup->add_item(TTR("Cargo Manager Dock..."), MENU_OPEN_DOCK);
		popup->add_item(TTR("Open Cargo.toml"), MENU_OPEN_TOML);
		popup->add_item(TTR("Reload Rust Library"), MENU_RELOAD_LIB);
		popup->add_separator();
		popup->add_item(TTR("Create Rust Scaffolding (res://rust)"), MENU_SCAFFOLDING);
		popup->connect(SceneStringName(id_pressed), callable_mp(this, &RustEditorPlugin::_on_menu_item_selected));
		toolbar_hb->add_child(toolbar_menu_btn);

		add_control_to_container(CONTAINER_TOOLBAR, toolbar_hb);

		// 3. Project -> Tools menu items
		add_tool_menu_item(TTR("Build Rust Crates (Cargo)"), callable_mp(this, &RustEditorPlugin::_on_build_rust));
		add_tool_menu_item(TTR("Open Cargo Manager Dock"), callable_mp(this, &RustEditorPlugin::_on_open_cargo_dock));
		add_tool_menu_item(TTR("Create Rust Project Scaffolding"), callable_mp(this, &RustEditorPlugin::_on_generate_scaffolding));
	}
}

RustEditorPlugin::RustEditorPlugin() {
}

RustEditorPlugin::~RustEditorPlugin() {
}
