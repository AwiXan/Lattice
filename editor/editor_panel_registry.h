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
#include "scene/resources/texture.h"

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
		// Shows one resource: a script, a material, an image. Opening a texture
		// in a window of its own is a panel of this kind, not a document one.
		BINDING_RESOURCE,
	};

	// Whether panels of this kind are pointed at something, and therefore
	// whether a pane holding one has to record what.
	enum Side {
		SIDE_NONE,
		SIDE_LEFT,
		SIDE_RIGHT,
		SIDE_BOTTOM,
	};

	static bool binding_takes_subject(Binding p_binding) {
		return p_binding == BINDING_DOCUMENT || p_binding == BINDING_RESOURCE;
	}

	struct PanelType {
		StringName id;
		String title;
		// An icon named in the editor's theme, and failing that one the type
		// brought with it - a plugin's icon is a texture, not a name.
		StringName icon;
		Ref<Texture2D> icon_texture;
		Binding binding = BINDING_CONTEXT;
		// Builds a panel of this type. Returns a Control the caller owns.
		Callable create;
		// Some panels are not built but lent. The editor has one FileSystem: a
		// pane showing it is showing that one, taken out of its slot for as long
		// as the pane holds it. A pane never frees a lent panel - it gives it
		// back, and whoever lent it decides where it goes.
		bool lent = false;
		// Offered as a button of its own in every pane's header, rather than
		// only in the menu. For the few things a pane is most often for - the 2D
		// and 3D views - so the header stays a handful of icons.
		bool featured = false;
		// Where it goes the first time it is asked for with nowhere to go back
		// to: beside the pane being worked in, on this side. None puts it in
		// that pane, as a tab.
		Side side = SIDE_NONE;
		// How well a panel of this type suits a resource, asked as
		// (String path, StringName resource_class) and answered with a number.
		// Zero, or unset, means it cannot show that resource at all. The highest
		// answer wins, so an inspector that shows anything answers 1 and a
		// viewer that knows the format answers more.
		//
		// This is the whole of it: nothing asks "is it a script", and a plugin
		// that brings a better viewer for something says so here rather than
		// being added to a list somewhere.
		Callable rank;
		// Shows a resource in a panel of this type, for a type that would
		// rather do it itself than be pointed at it. The script editor would:
		// it has tabs of its own, and opening a script means adding one to them
		// rather than making a second script editor. Answers whether it took
		// it. Unset means "point the panel at it", which is what a panel bound
		// to a resource is for.
		Callable open;
		// What a panel of this type wants written down beyond its subject, and
		// how to read it back. A scene panel remembers whether it was on the
		// running game and which session; a viewport could remember its camera.
		// Anything a layout has to survive a restart with goes here rather than
		// into the arrangement, which knows nothing about any panel in
		// particular.
		Callable save_state;
		Callable load_state;
		// Called with the panel when a pane stops showing it, and says whether
		// it took it back. A lent type always does. A type that builds its
		// panels may still want the first one back - the editor's own 3D view
		// is built before any pane exists - and says so by answering true for
		// that one and false for the rest, which the caller then frees.
		Callable release;
		// Points a panel of this type at what it shows. The subject is whatever
		// that kind of binding means: a document's history id, where -1 is
		// "whichever is current", or a resource's path. Kept as a Variant so a
		// binding kind added later needs nothing here.
		//
		// A history id lives only as long as the session. What a saved layout
		// records is the *persistent* form - a scene path for a document, the
		// path itself for a resource - which is why a page is restored by
		// reopening the document and binding to the id it gets, rather than by
		// writing the id down.
		Callable bind;
		// What a panel's tab says, when that is more than the kind of panel -
		// which script, and whether it has changes not saved. Called with the
		// panel; optional.
		Callable title_of;
		// Told, with the panel, that the user closed it - as against it going
		// with a layout being replaced. Optional.
		Callable closed_by_user;
	};

	static void register_type(const PanelType &p_type);
	static void unregister_type(const StringName &p_id);

	// What a subject of this kind turns into when it is dropped somewhere and
	// nothing has named a type - a scene tab dragged onto a pane. The editor
	// keeps it on whichever view was last worked in, so a drop gives the kind
	// of view being used rather than one chosen here once and for all.
	static void set_default_type_for(Binding p_binding, const StringName &p_id);
	static StringName get_default_type_for(Binding p_binding);

	static bool has_type(const StringName &p_id);
	static const PanelType *get_type(const StringName &p_id);
	static Vector<StringName> get_type_ids();

	// Builds a panel of the named type, or null if there is no such type or it
	// refused. The caller owns what comes back and must parent or free it.
	static Control *create_panel(const StringName &p_id);
	// Points a panel at what it should show. Does nothing for a type that shows
	// the same thing whoever holds it, so callers need not ask first.
	static void bind_panel(const StringName &p_id, Control *p_panel, const Variant &p_subject);
	// Gives a panel back to whoever lent it. False means nobody did, and the
	// caller is the one who has to free it.
	static bool release_panel(const StringName &p_id, Control *p_panel);

	// Which registered type suits this resource best, or nothing if none does.
	static StringName find_type_for_resource(const String &p_path);
	// Asks a type to show a resource itself. False means it would rather be
	// pointed at it in the ordinary way.
	static bool open_resource(const StringName &p_id, Control *p_panel, const String &p_path);

	// Whatever a panel wants remembered besides its subject. Empty for a type
	// that wants nothing, which is most of them.
	static Dictionary save_panel_state(const StringName &p_id, Control *p_panel);
	static void load_panel_state(const StringName &p_id, Control *p_panel, const Dictionary &p_state);

	// A type another stands in for. Asked for by the old id - a saved layout,
	// a quick list, a dock asked for by name - the new one answers, and the old
	// one is no longer offered beside it. The new one takes the old one's title
	// and icon: it is what people know it by.
	static void set_replacement(const StringName &p_id, const StringName &p_by);
	// The id to use for p_id: itself, or whatever stands in for it.
	static StringName resolve(const StringName &p_id);

	// The Command Palette's name for showing a panel of this type.
	static String get_palette_command_key(const StringName &p_id);

	// What a panel's tab says.
	static String get_panel_title(const StringName &p_id, Control *p_panel);
	static void notify_closed_by_user(const StringName &p_id, Control *p_panel);

	static void cleanup();

private:
	static inline HashMap<StringName, PanelType> types;
	static inline HashMap<int, StringName> default_types;
	static inline HashMap<StringName, StringName> replacements;
	// Every kind of panel can be asked for from the Command Palette, by name.
	static void _add_palette_command(const PanelType &p_type);
	static void _remove_palette_command(const StringName &p_id);
	static void _show_from_palette(const StringName &p_id);
};
