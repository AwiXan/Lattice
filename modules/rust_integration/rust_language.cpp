/**************************************************************************/
/*  rust_language.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_language.h"

#include "rust_manager.h"
#include "rust_script.h"

RustLanguage *RustLanguage::singleton = nullptr;

void RustLanguage::init() {
}

void RustLanguage::finish() {
}

Vector<String> RustLanguage::get_reserved_words() const {
	static const char *_reserved_words[] = {
		"as", "async", "await", "break", "const", "continue", "crate", "dyn",
		"else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
		"let", "loop", "macro_rules", "match", "mod", "move", "mut", "pub",
		"ref", "return", "self", "Self", "static", "struct", "super", "trait",
		"true", "try", "type", "union", "unsafe", "use", "where", "while",
		"yield", "abstract", "become", "box", "do", "final", "macro",
		"override", "priv", "typeof", "unsized", "virtual",
		nullptr
	};

	Vector<String> words;
	const char **w = _reserved_words;
	while (*w) {
		words.push_back(*w);
		w++;
	}
	return words;
}

bool RustLanguage::is_control_flow_keyword(const String &p_keyword) const {
	return p_keyword == "if" || p_keyword == "else" || p_keyword == "match" ||
			p_keyword == "while" || p_keyword == "for" || p_keyword == "loop" ||
			p_keyword == "return" || p_keyword == "break" || p_keyword == "continue" ||
			p_keyword == "yield" || p_keyword == "await" || p_keyword == "try";
}

Vector<String> RustLanguage::get_comment_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("//");
	delimiters.push_back("/* */");
	return delimiters;
}

Vector<String> RustLanguage::get_doc_comment_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("///");
	delimiters.push_back("//!");
	return delimiters;
}

Vector<String> RustLanguage::get_string_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("\" \"");
	delimiters.push_back("' '");
	return delimiters;
}

Ref<Script> RustLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<RustScript> scr;
	scr.instantiate();

	String processed = p_template;
	processed = processed.replace("_CLASS_", p_class_name);
	processed = processed.replace("_BASE_", p_base_class_name);

	scr->set_source_code(processed);
	return scr;
}

Vector<ScriptLanguage::ScriptTemplate> RustLanguage::get_built_in_templates(const StringName &p_object) {
	Vector<ScriptTemplate> templates;

	ScriptTemplate t;
	t.inherit = "Node";
	t.name = "Default";
	t.description = "Native high-performance Rust node with _ready and _process";
	t.content =
			"// Native Rust script for _CLASS_\n"
			"mod godot_ffi;\n"
			"use godot_ffi::*;\n\n"
			"#[no_mangle]\n"
			"pub extern \"C\" fn rust_node_ready(instance_id: u64) {\n"
			"    print(&format!(\"_CLASS_ ready on instance: {}\", instance_id));\n"
			"}\n\n"
			"#[no_mangle]\n"
			"pub extern \"C\" fn rust_node_process(instance_id: u64, delta: f64) {\n"
			"    // High-performance process loop (runs in native compiled Rust without boxing)\n"
			"}\n";
	templates.push_back(t);

	return templates;
}

String RustLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const {
	String f = "pub fn " + p_name + "(";
	for (int i = 0; i < p_args.size(); i++) {
		if (i > 0) {
			f += ", ";
		}
		f += p_args[i];
	}
	f += ") {\n    \n}\n";
	return f;
}

void RustLanguage::reload_all_scripts() {
	if (RustManager::get_singleton()) {
		RustManager::get_singleton()->reload_library();
	}
}

void RustLanguage::reload_scripts(const Array &p_scripts, bool p_soft_reload) {
	if (RustManager::get_singleton()) {
		RustManager::get_singleton()->reload_library();
	}
}

void RustLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
	if (RustManager::get_singleton()) {
		RustManager::get_singleton()->reload_library();
	}
}

void RustLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("rs");
}

RustLanguage::RustLanguage() {
	singleton = this;
}

RustLanguage::~RustLanguage() {
	singleton = nullptr;
}
