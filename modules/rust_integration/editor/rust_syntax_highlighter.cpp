/**************************************************************************/
/*  rust_syntax_highlighter.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_syntax_highlighter.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "editor/settings/editor_settings.h"

RustSyntaxHighlighter::RustSyntaxHighlighter() {
	highlighter.instantiate();
}

Dictionary RustSyntaxHighlighter::_get_line_syntax_highlighting_impl(int p_line) {
	return highlighter->get_line_syntax_highlighting(p_line);
}

PackedStringArray RustSyntaxHighlighter::_get_supported_languages() const {
	PackedStringArray langs;
	langs.push_back("Rust");
	langs.push_back("rs");
	return langs;
}

Ref<EditorSyntaxHighlighter> RustSyntaxHighlighter::_create() const {
	Ref<RustSyntaxHighlighter> sh;
	sh.instantiate();
	return sh;
}

void RustSyntaxHighlighter::_update_cache() {
	highlighter->set_text_edit(text_edit);
	highlighter->clear_keyword_colors();
	highlighter->clear_member_keyword_colors();
	highlighter->clear_color_regions();

	highlighter->set_symbol_color(EDITOR_GET("text_editor/theme/highlighting/symbol_color"));
	highlighter->set_function_color(EDITOR_GET("text_editor/theme/highlighting/function_color"));
	highlighter->set_number_color(EDITOR_GET("text_editor/theme/highlighting/number_color"));
	highlighter->set_member_variable_color(EDITOR_GET("text_editor/theme/highlighting/member_variable_color"));

	const Color keyword_color = EDITOR_GET("text_editor/theme/highlighting/keyword_color");
	const Color control_flow_color = EDITOR_GET("text_editor/theme/highlighting/control_flow_keyword_color");
	const Color type_color = EDITOR_GET("text_editor/theme/highlighting/base_type_color");
	const Color engine_type_color = EDITOR_GET("text_editor/theme/highlighting/engine_type_color");
	const Color comment_color = EDITOR_GET("text_editor/theme/highlighting/comment_color");
	const Color doc_comment_color = EDITOR_GET("text_editor/theme/highlighting/doc_comment_color");
	const Color string_color = EDITOR_GET("text_editor/theme/highlighting/string_color");
	const Color annotation_color = EDITOR_GET("text_editor/theme/highlighting/annotation_color");

	// Control flow keywords
	static const char *cf_keywords[] = {
		"break", "continue", "else", "for", "if", "loop", "match", "return", "while", "yield", "await", "try", nullptr
	};
	for (int i = 0; cf_keywords[i]; i++) {
		highlighter->add_keyword_color(cf_keywords[i], control_flow_color);
	}

	// General keywords
	static const char *keywords[] = {
		"as", "async", "const", "crate", "dyn", "enum", "extern", "false", "fn",
		"impl", "in", "let", "mod", "move", "mut", "pub", "ref", "self", "Self",
		"static", "struct", "super", "trait", "true", "type", "union", "unsafe", "use",
		"where", "macro_rules", nullptr
	};
	for (int i = 0; keywords[i]; i++) {
		highlighter->add_keyword_color(keywords[i], keyword_color);
	}

	// Rust standard & primitive types
	static const char *types[] = {
		"i8", "i16", "i32", "i64", "i128", "isize",
		"u8", "u16", "u32", "u64", "u128", "usize",
		"f32", "f64", "bool", "char", "str", "String",
		"Option", "Some", "None", "Result", "Ok", "Err",
		"Vec", "Box", "Rc", "Arc", "RefCell", "Cell",
		"HashMap", "HashSet", "BTreeMap", "BTreeSet",
		"Cow", "Pin", "Future", "Send", "Sync", "Drop",
		"Clone", "Copy", "Default", "Debug", "Display",
		nullptr
	};
	for (int i = 0; types[i]; i++) {
		highlighter->add_keyword_color(types[i], type_color);
	}

	// Godot engine types
	LocalVector<StringName> godot_classes;
	ClassDB::get_class_list(godot_classes);
	for (const StringName &cls : godot_classes) {
		highlighter->add_keyword_color(cls, engine_type_color);
	}

	// Common standard macros
	static const char *macros[] = {
		"println", "print", "eprintln", "eprint", "format", "vec", "panic",
		"todo", "unimplemented", "unreachable", "assert", "assert_eq", "assert_ne",
		"debug_assert", "debug_assert_eq", "cfg", "env", "include", "include_str", "include_bytes",
		nullptr
	};
	const Color function_color = EDITOR_GET("text_editor/theme/highlighting/function_color");
	for (int i = 0; macros[i]; i++) {
		highlighter->add_keyword_color(macros[i], function_color);
	}

	// Comments & Doc Comments
	highlighter->add_color_region("///", "", doc_comment_color, true);
	highlighter->add_color_region("//!", "", doc_comment_color, true);
	highlighter->add_color_region("//", "", comment_color, true);
	highlighter->add_color_region("/*", "*/", comment_color, false);

	// Strings & Chars
	highlighter->add_color_region("\"", "\"", string_color, false);
	highlighter->add_color_region("'", "'", string_color, false);

	// Attributes
	highlighter->add_color_region("#[", "]", annotation_color, true);
	highlighter->add_color_region("#![", "]", annotation_color, true);
}
