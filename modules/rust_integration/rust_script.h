/**************************************************************************/
/*  rust_script.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/script_language.h"

#ifdef TOOLS_ENABLED
#include "core/doc_data.h"
#endif

class RustLanguage;

class RustScript : public Script {
	GDCLASS(RustScript, Script);

	String source_code;
	String path;
	StringName class_name;
	StringName base_type = "Node";

#ifdef TOOLS_ENABLED
	Vector<DocData::ClassDoc> docs;
	void _update_doc_data();
#endif

protected:
	static void _bind_methods();

public:
	virtual bool can_instantiate() const override;

	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual StringName get_global_name() const override { return class_name; }
	virtual bool inherits_script(const Ref<Script> &p_script) const override { return false; }

	virtual StringName get_instance_base_type() const override { return base_type; }
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) override;

	virtual bool has_source_code() const override { return !source_code.is_empty(); }
	virtual String get_source_code() const override { return source_code; }
	virtual void set_source_code(const String &p_code) override;
	virtual void set_path(const String &p_path, bool p_take_over = false) override;
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override { return class_name; }
	virtual Vector<DocData::ClassDoc> get_documentation() const override { return docs; }
	virtual String get_class_icon_path() const override { return ""; }
#endif

	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual bool is_tool() const override { return false; }
	virtual bool is_valid() const override { return true; }
	virtual bool is_abstract() const override { return false; }

	virtual const Variant get_rpc_config() const override { return Variant(); }

	virtual ScriptLanguage *get_language() const override;

	virtual bool has_script_signal(const StringName &p_signal) const override { return false; }
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override {}

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override { return false; }

	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;

	RustScript();
	~RustScript();
};
