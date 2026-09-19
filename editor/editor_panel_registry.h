/**************************************************************************/
/*  editor_panel_registry.h                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/variant/callable.h"

class Control;

// What the editor can put in a pane, a tab or a window of its own.
//
// A panel type is named once and built many times. Nothing that arranges panels
// - a pane, a page, a torn-off window - names a class: it stores the type's id,
// what the panel is bound to and where it sits, and asks here for the rest. An
// addon registers a type the same way the editor's own do, so a layout saved
// with an addon's panel in it comes back with that panel in it.
//
// There is deliberately no common base class. A scene tree is a Control, an
// inspector a ScrollContainer, a viewport editor a VBoxContainer, and forcing
// them into one hierarchy would buy nothing: the type says how to build its
// panel and how to point it at a document, and that is all anyone needs.
class EditorPanelRegistry {
public:
	// What a panel of this type shows, which decides what a layout has to record
	// about it and what a pane may offer to change.
	enum Binding {
		// One for the whole editor: the FileSystem, the Log. Several panes may
		// show it, and they all show the same thing.
		BINDING_GLOBAL,
		// Follows the pane being worked in. This is how the editor's docks have
		// always behaved, and stays the default.
		BINDING_CONTEXT,
		// Shows one open document, whichever the pane holding it was pointed at.
		BINDING_DOCUMENT,
	};

	struct PanelType {
		StringName id;
		String title;
		StringName icon;
		Binding binding = BINDING_CONTEXT;
		// Builds a panel of this type. Returns a Control the caller owns.
		Callable create;
		// Points a panel of this type at a document, by history id; -1 means it
		// follows whichever document is current. Only for BINDING_DOCUMENT.
		Callable bind;
	};

	static void register_type(const PanelType &p_type);
	static void unregister_type(const StringName &p_id);

	static bool has_type(const StringName &p_id);
	static const PanelType *get_type(const StringName &p_id);
	static Vector<StringName> get_type_ids();

	// Builds a panel of the named type, or null if there is no such type or it
	// refused. The caller owns what comes back and must parent or free it.
	static Control *create_panel(const StringName &p_id);
	// Points a panel at a document. Does nothing for a type that is not bound to
	// one, so callers need not ask first.
	static void bind_panel(const StringName &p_id, Control *p_panel, int p_document_id);

	static void cleanup();

private:
	static inline HashMap<StringName, PanelType> types;
};
