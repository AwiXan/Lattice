/**************************************************************************/
/*  editor_panel_registry.cpp                                             */
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

#include "editor_panel_registry.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_command_palette.h"

#include "scene/gui/control.h"

void EditorPanelRegistry::register_type(const PanelType &p_type) {
	ERR_FAIL_COND_MSG(p_type.id == StringName(), "A panel type needs an id to be asked for by.");
	if (replacements.has(p_type.id)) {
		// Something already stands in for it.
		return;
	}
	if (replacements.has(p_type.id)) {
		// Something already stands in for it.
		return;
	}
	ERR_FAIL_COND_MSG(!p_type.create.is_valid(), vformat("The panel type '%s' has no way to build a panel.", p_type.id));
	ERR_FAIL_COND_MSG(types.has(p_type.id), vformat("A panel type '%s' is already registered.", p_type.id));
	ERR_FAIL_COND_MSG(p_type.lent && !p_type.release.is_valid(),
			vformat("The panel type '%s' lends its panels but has no way to be given one back.", p_type.id));
	ERR_FAIL_COND_MSG(binding_takes_subject(p_type.binding) && !p_type.bind.is_valid(),
			vformat("The panel type '%s' says it shows one thing but has no way to be pointed at one.", p_type.id));
	types.insert(p_type.id, p_type);
	_add_palette_command(p_type);
}

void EditorPanelRegistry::unregister_type(const StringName &p_id) {
	types.erase(p_id);
	_remove_palette_command(p_id);
}

String EditorPanelRegistry::get_palette_command_key(const StringName &p_id) {
	return "panels/show_" + String(p_id);
}

void EditorPanelRegistry::_add_palette_command(const PanelType &p_type) {
	if (p_type.binding == BINDING_RESOURCE) {
		// Opened by the resource it shows, not by name.
		return;
	}
	EditorCommandPalette *palette = EditorCommandPalette::get_singleton();
	const String key = get_palette_command_key(p_type.id);
	if (palette->has_command(key)) {
		palette->remove_command(key);
	}
	const String title = p_type.title.is_empty() ? String(p_type.id) : TTRGET(p_type.title);
	palette->add_command(vformat(TTR("Show Panel: %s"), title), key, callable_mp_static(&EditorPanelRegistry::_show_from_palette).bind(p_type.id));
}

void EditorPanelRegistry::_remove_palette_command(const StringName &p_id) {
	EditorCommandPalette *palette = EditorCommandPalette::get_singleton();
	const String key = get_palette_command_key(p_id);
	if (palette->has_command(key)) {
		palette->remove_command(key);
	}
}

void EditorPanelRegistry::_show_from_palette(const StringName &p_id) {
	// Where it is, where it was last, or beside the pane being worked in -
	// the same as asking for it any other way.
	EditorMainScreen *main_screen = EditorNode::get_editor_main_screen();
	if (main_screen) {
		main_screen->show_panel(p_id);
	}
}

void EditorPanelRegistry::set_default_type_for(Binding p_binding, const StringName &p_id) {
	if (p_id == StringName()) {
		default_types.erase((int)p_binding);
		return;
	}
	default_types[(int)p_binding] = p_id;
}

StringName EditorPanelRegistry::get_default_type_for(Binding p_binding) {
	const StringName *id = default_types.getptr((int)p_binding);
	// Only a type that is still registered: an addon can be turned off between
	// one drop and the next.
	if (id && types.has(*id)) {
		return *id;
	}
	return StringName();
}

bool EditorPanelRegistry::has_type(const StringName &p_id) {
	return types.has(resolve(p_id));
}

const EditorPanelRegistry::PanelType *EditorPanelRegistry::get_type(const StringName &p_id) {
	HashMap<StringName, PanelType>::ConstIterator it = types.find(resolve(p_id));
	return it ? &it->value : nullptr;
}

void EditorPanelRegistry::set_replacement(const StringName &p_id, const StringName &p_by) {
	ERR_FAIL_COND(p_id == p_by);
	const PanelType *replaced = types.getptr(p_id);
	PanelType *by = types.getptr(p_by);
	if (replaced && by) {
		if (!replaced->title.is_empty()) {
			by->title = replaced->title;
		}
		if (replaced->icon != StringName()) {
			by->icon = replaced->icon;
		}
		if (replaced->icon_texture.is_valid()) {
			by->icon_texture = replaced->icon_texture;
		}
		if (replaced->side != SIDE_NONE) {
			by->side = replaced->side;
		}
	}
	types.erase(p_id);
	replacements[p_id] = p_by;
	_remove_palette_command(p_id);
	if (by) {
		_add_palette_command(*by);
	}
}

StringName EditorPanelRegistry::resolve(const StringName &p_id) {
	const StringName *by = replacements.getptr(p_id);
	return by ? *by : p_id;
}

Vector<StringName> EditorPanelRegistry::get_type_ids() {
	Vector<StringName> ids;
	ids.resize(types.size());
	int i = 0;
	for (const KeyValue<StringName, PanelType> &E : types) {
		ids.write[i++] = E.key;
	}
	return ids;
}

Control *EditorPanelRegistry::create_panel(const StringName &p_id) {
	const PanelType *type = get_type(p_id);
	ERR_FAIL_NULL_V_MSG(type, nullptr, vformat("No panel type '%s' is registered.", p_id));

	Variant result = type->create.call();
	Control *panel = Object::cast_to<Control>(result.get_validated_object());
	// A type is allowed to refuse - a plugin that cannot be shown twice says so
	// by handing back nothing - so this is not an error.
	return panel;
}

void EditorPanelRegistry::bind_panel(const StringName &p_id, Control *p_panel, const Variant &p_subject) {
	if (!p_panel) {
		return;
	}
	const PanelType *type = get_type(p_id);
	if (!type || !binding_takes_subject(type->binding) || !type->bind.is_valid()) {
		// Nothing to point anywhere: this panel shows the same thing whoever
		// holds it, or follows the pane being worked in.
		return;
	}
	type->bind.call(p_panel, p_subject);
}

StringName EditorPanelRegistry::find_type_for_resource(const String &p_path) {
	if (p_path.is_empty()) {
		return StringName();
	}
	const StringName resource_class = ResourceLoader::get_resource_type(p_path);

	StringName best;
	int best_rank = 0;
	for (const KeyValue<StringName, PanelType> &E : types) {
		if (!E.value.rank.is_valid()) {
			continue;
		}
		const Variant answer = E.value.rank.call(p_path, resource_class);
		const int rank = answer.get_type() == Variant::INT ? (int)answer : 0;
		if (rank > best_rank) {
			best_rank = rank;
			best = E.key;
		}
	}
	return best;
}

bool EditorPanelRegistry::open_resource(const StringName &p_id, Control *p_panel, const String &p_path) {
	const PanelType *type = get_type(p_id);
	if (!type || !type->open.is_valid()) {
		return false;
	}
	const Variant answer = type->open.call(p_panel, p_path);
	return answer.get_type() == Variant::BOOL ? (bool)answer : true;
}

Dictionary EditorPanelRegistry::save_panel_state(const StringName &p_id, Control *p_panel) {
	const PanelType *type = get_type(p_id);
	if (!type || !type->save_state.is_valid() || !p_panel) {
		return Dictionary();
	}
	return type->save_state.call(p_panel);
}

void EditorPanelRegistry::load_panel_state(const StringName &p_id, Control *p_panel, const Dictionary &p_state) {
	const PanelType *type = get_type(p_id);
	if (!type || !type->load_state.is_valid() || !p_panel || p_state.is_empty()) {
		return;
	}
	type->load_state.call(p_panel, p_state);
}

bool EditorPanelRegistry::release_panel(const StringName &p_id, Control *p_panel) {
	const PanelType *type = get_type(p_id);
	if (!type || !type->release.is_valid()) {
		// Either nobody wants it back, or whoever lent it is gone - at which
		// point the caller freeing it is the right thing.
		return false;
	}
	const Variant answer = type->release.call(p_panel);
	// A release that says nothing has taken it: only a type that hands some of
	// its panels back and not others needs to answer.
	return answer.get_type() == Variant::BOOL ? (bool)answer : true;
}

void EditorPanelRegistry::cleanup() {
	// The callables hold the plugins that registered them, and those are gone by
	// the time the editor is torn down.
	default_types.clear();
	types.clear();
	replacements.clear();
}
