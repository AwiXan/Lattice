/**************************************************************************/
/*  rust_syntax_highlighter.h                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "editor/script/syntax_highlighters.h"
#include "scene/gui/code_edit.h"

class RustSyntaxHighlighter : public EditorSyntaxHighlighter {
	GDCLASS(RustSyntaxHighlighter, EditorSyntaxHighlighter);

private:
	Ref<CodeHighlighter> highlighter;
	virtual void _update_cache() override;

public:
	virtual Dictionary _get_line_syntax_highlighting_impl(int p_line) override;
	virtual PackedStringArray _get_supported_languages() const override;
	virtual Ref<EditorSyntaxHighlighter> _create() const override;
	virtual String _get_name() const override { return "Rust"; }

	RustSyntaxHighlighter();
};
