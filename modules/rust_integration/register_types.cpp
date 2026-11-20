/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "register_types.h"

#include "rust_language.h"
#include "rust_manager.h"
#include "rust_resource_format.h"
#include "rust_script.h"

#ifdef TOOLS_ENABLED
#include "editor/cargo_manager_dock.h"
#include "editor/editor_node.h"
#include "editor/rust_editor_plugin.h"
#include "editor/rust_syntax_highlighter.h"
#include "editor/script/script_editor_plugin.h"
#endif

RustLanguage *script_language_rust = nullptr;
RustManager *rust_manager = nullptr;
Ref<ResourceFormatLoaderRustScript> resource_loader_rust;
Ref<ResourceFormatSaverRustScript> resource_saver_rust;

#ifdef TOOLS_ENABLED
static void _editor_init_rust() {
	EditorNode::add_editor_plugin(memnew(RustEditorPlugin));

	Ref<RustSyntaxHighlighter> rust_syntax_highlighter;
	rust_syntax_highlighter.instantiate();
	if (ScriptEditor::get_singleton()) {
		ScriptEditor::get_singleton()->register_syntax_highlighter(rust_syntax_highlighter);
	}
}
#endif

void initialize_rust_integration_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		rust_manager = memnew(RustManager);
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(RustScript);
		GDREGISTER_CLASS(RustManager);

		script_language_rust = memnew(RustLanguage);
		script_language_rust->set_language_index(ScriptServer::get_language_count());
		ScriptServer::register_language(script_language_rust);

		resource_loader_rust.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_rust);
		resource_saver_rust.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_rust);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_INTERNAL_CLASS(CargoManagerDock);
		GDREGISTER_CLASS(RustSyntaxHighlighter);
		EditorNode::add_init_callback(&_editor_init_rust);
	}
#endif
}

void uninitialize_rust_integration_module(ModuleInitializationLevel p_level) {

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (script_language_rust) {
			ScriptServer::unregister_language(script_language_rust);
			memdelete(script_language_rust);
			script_language_rust = nullptr;
		}

		if (resource_loader_rust.is_valid()) {
			ResourceLoader::remove_resource_format_loader(resource_loader_rust);
			resource_loader_rust.unref();
		}
		if (resource_saver_rust.is_valid()) {
			ResourceSaver::remove_resource_format_saver(resource_saver_rust);
			resource_saver_rust.unref();
		}
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (rust_manager) {
			memdelete(rust_manager);
			rust_manager = nullptr;
		}
	}
}
