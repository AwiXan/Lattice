/**************************************************************************/
/*  rust_instance.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_instance.h"

#include "rust_language.h"
#include "rust_manager.h"
#include "rust_script.h"
#include "scene/main/node.h"

void RustInstance::_resolve_symbols() {
	RustManager *rm = RustManager::get_singleton();
	if (!rm) {
		return;
	}

	if (!rm->is_library_loaded()) {
		rm->reload_library();
	}

	if (rm->is_library_loaded()) {
		ready_func = (RustReadyFunc)rm->get_symbol("rust_node_ready");
		process_func = (RustProcessFunc)rm->get_symbol("rust_node_process");
		physics_process_func = (RustProcessFunc)rm->get_symbol("rust_node_physics_process");
	}
}

bool RustInstance::set(const StringName &p_name, const Variant &p_value) {
	properties[p_name] = p_value;
	return true;
}

bool RustInstance::get(const StringName &p_name, Variant &r_ret) const {
	if (properties.has(p_name)) {
		r_ret = properties[p_name];
		return true;
	}
	return false;
}

void RustInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	for (const KeyValue<StringName, Variant> &E : properties) {
		p_properties->push_back(PropertyInfo(E.value.get_type(), E.key));
	}
}

Variant::Type RustInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (properties.has(p_name)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return properties[p_name].get_type();
	}
	if (r_is_valid) {
		*r_is_valid = false;
	}
	return Variant::NIL;
}

void RustInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool RustInstance::has_method(const StringName &p_method) const {
	if (script.is_valid()) {
		return script->has_method(p_method);
	}
	return false;
}

Variant RustInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	r_error.error = Callable::CallError::CALL_OK;
	if (p_method == SNAME("_ready")) {
		notification(Node::NOTIFICATION_READY);
		return Variant();
	} else if (p_method == SNAME("_process") && p_argcount >= 1 && p_args[0]) {
		double delta = *p_args[0];
		if (process_func && owner) {
			process_func(owner->get_instance_id(), delta);
		}
		return Variant();
	} else if (p_method == SNAME("_physics_process") && p_argcount >= 1 && p_args[0]) {
		double delta = *p_args[0];
		if (physics_process_func && owner) {
			physics_process_func(owner->get_instance_id(), delta);
		}
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
	return Variant();
}

void RustInstance::notification(int p_notification, bool p_reversed) {
	if (!owner) {
		return;
	}

	if (p_notification == Node::NOTIFICATION_READY) {
		_resolve_symbols();
		if (ready_func) {
			ready_func(owner->get_instance_id());
		}
	} else if (p_notification == Node::NOTIFICATION_PROCESS) {
		if (process_func) {
			Node *node = Object::cast_to<Node>(owner);
			double delta = node ? node->get_process_delta_time() : 0.0;
			process_func(owner->get_instance_id(), delta);
		}
	} else if (p_notification == Node::NOTIFICATION_PHYSICS_PROCESS) {
		if (physics_process_func) {
			Node *node = Object::cast_to<Node>(owner);
			double delta = node ? node->get_physics_process_delta_time() : 0.0;
			physics_process_func(owner->get_instance_id(), delta);
		}
	}
}

Ref<Script> RustInstance::get_script() const {
	return script;
}

ScriptLanguage *RustInstance::get_language() {
	return RustLanguage::get_singleton();
}

RustInstance::RustInstance(Object *p_owner, Ref<RustScript> p_script) {
	owner = p_owner;
	script = p_script;
	_resolve_symbols();
}

RustInstance::~RustInstance() {
}
