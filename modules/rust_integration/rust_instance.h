/**************************************************************************/
/*  rust_instance.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/object/script_instance.h"
#include "core/templates/hash_map.h"
#include "rust_script.h"

class RustInstance : public ScriptInstance {
	Object *owner = nullptr;
	Ref<RustScript> script;
	HashMap<StringName, Variant> properties;

	typedef void (*RustReadyFunc)(uint64_t);
	typedef void (*RustProcessFunc)(uint64_t, double);

	RustReadyFunc ready_func = nullptr;
	RustProcessFunc process_func = nullptr;
	RustProcessFunc physics_process_func = nullptr;

	void _resolve_symbols();

public:
	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
	virtual void validate_property(PropertyInfo &p_property) const override {}

	virtual bool property_can_revert(const StringName &p_name) const override { return false; }
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const override { return false; }

	virtual Object *get_owner() override { return owner; }

	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
	virtual void notification(int p_notification, bool p_reversed = false) override;

	virtual Ref<Script> get_script() const override;
	virtual ScriptLanguage *get_language() override;

	RustInstance(Object *p_owner, Ref<RustScript> p_script);
	virtual ~RustInstance();
};
