/**************************************************************************/
/*  rust_script.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_script.h"

#include "rust_instance.h"
#include "rust_language.h"

void RustScript::_bind_methods() {
}

bool RustScript::can_instantiate() const {
	return true;
}

ScriptInstance *RustScript::instance_create(Object *p_this) {
	return memnew(RustInstance(p_this, Ref<RustScript>(this)));
}

PlaceHolderScriptInstance *RustScript::placeholder_instance_create(Object *p_this) {
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(get_language(), Ref<Script>(this), p_this));
	return si;
}

Error RustScript::reload(bool p_keep_state) {
	return OK;
}

bool RustScript::has_method(const StringName &p_method) const {
	return p_method == SNAME("_ready") || p_method == SNAME("_process") || p_method == SNAME("_physics_process");
}

MethodInfo RustScript::get_method_info(const StringName &p_method) const {
	MethodInfo mi;
	mi.name = p_method;
	if (p_method == SNAME("_process") || p_method == SNAME("_physics_process")) {
		PropertyInfo pi;
		pi.name = "delta";
		pi.type = Variant::FLOAT;
		mi.arguments.push_back(pi);
	}
	return mi;
}

ScriptLanguage *RustScript::get_language() const {
	return RustLanguage::get_singleton();
}

void RustScript::get_script_method_list(List<MethodInfo> *p_list) const {
	p_list->push_back(get_method_info(SNAME("_ready")));
	p_list->push_back(get_method_info(SNAME("_process")));
	p_list->push_back(get_method_info(SNAME("_physics_process")));
}

void RustScript::get_script_property_list(List<PropertyInfo> *p_list) const {
}

RustScript::RustScript() {
}

RustScript::~RustScript() {
}
