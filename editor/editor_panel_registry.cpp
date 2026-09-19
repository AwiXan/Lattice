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

#include "scene/gui/control.h"

void EditorPanelRegistry::register_type(const PanelType &p_type) {
	ERR_FAIL_COND_MSG(p_type.id == StringName(), "A panel type needs an id to be asked for by.");
	ERR_FAIL_COND_MSG(!p_type.create.is_valid(), vformat("The panel type '%s' has no way to build a panel.", p_type.id));
	ERR_FAIL_COND_MSG(types.has(p_type.id), vformat("A panel type '%s' is already registered.", p_type.id));
	ERR_FAIL_COND_MSG(binding_takes_subject(p_type.binding) && !p_type.bind.is_valid(),
			vformat("The panel type '%s' says it shows one thing but has no way to be pointed at one.", p_type.id));
	types.insert(p_type.id, p_type);
}

void EditorPanelRegistry::unregister_type(const StringName &p_id) {
	types.erase(p_id);
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
	return types.has(p_id);
}

const EditorPanelRegistry::PanelType *EditorPanelRegistry::get_type(const StringName &p_id) {
	HashMap<StringName, PanelType>::ConstIterator it = types.find(p_id);
	return it ? &it->value : nullptr;
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

void EditorPanelRegistry::cleanup() {
	// The callables hold the plugins that registered them, and those are gone by
	// the time the editor is torn down.
	default_types.clear();
	types.clear();
}
