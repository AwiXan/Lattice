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

#ifdef TOOLS_ENABLED
static String _map_rust_type_to_godot(const String &p_type) {
	String t = p_type.strip_edges();
	if (t == "f32" || t == "f64" || t == "real_t") {
		return "float";
	} else if (t == "i8" || t == "i16" || t == "i32" || t == "i64" || t == "u8" || t == "u16" || t == "u32" || t == "u64" || t == "isize" || t == "usize") {
		return "int";
	} else if (t == "bool") {
		return "bool";
	} else if (t == "String" || t == "&str" || t == "StringName") {
		return "String";
	} else if (t == "Vector3" || t == "[f32; 3]") {
		return "Vector3";
	} else if (t == "Vector2" || t == "[f32; 2]") {
		return "Vector2";
	} else if (t == "Transform3D") {
		return "Transform3D";
	} else if (t == "Basis") {
		return "Basis";
	} else if (t == "Quaternion") {
		return "Quaternion";
	} else if (t == "Color") {
		return "Color";
	} else if (t == "RID") {
		return "RID";
	} else if (t == "NodePath") {
		return "NodePath";
	} else if (t == "()" || t.is_empty()) {
		return "void";
	}
	return t;
}

void RustScript::_update_doc_data() {
	docs.clear();

	if (source_code.is_empty()) {
		return;
	}

	DocData::ClassDoc cdoc;

	// Infer class name from path or struct definition
	if (class_name == StringName() || class_name == StringName("RustScript")) {
		if (!path.is_empty()) {
			String base = path.get_file().get_basename();
			Vector<String> parts = base.split("_");
			String pascal;
			for (int i = 0; i < parts.size(); i++) {
				pascal += parts[i].capitalize().replace(" ", "");
			}
			class_name = pascal.is_empty() ? base : pascal;
		}
	}

	cdoc.name = class_name;
	cdoc.inherits = base_type;
	cdoc.is_script_doc = true;
	cdoc.script_path = path;

	PackedStringArray lines = source_code.split("\n");
	String accumulated_class_doc;
	String accumulated_item_doc;

	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();

		if (line.begins_with("//!")) {
			String d = line.substr(3).strip_edges();
			if (!accumulated_class_doc.is_empty()) {
				accumulated_class_doc += "\n";
			}
			accumulated_class_doc += d;
			continue;
		}

		if (line.begins_with("///")) {
			String d = line.substr(3).strip_edges();
			if (!accumulated_item_doc.is_empty()) {
				accumulated_item_doc += "\n";
			}
			accumulated_item_doc += d;
			continue;
		}

		if (line.begins_with("#[")) {
			// Skip attributes
			continue;
		}

		if (line.is_empty()) {
			continue;
		}

		// Check for struct definition to infer class_name or base_type
		if (line.begins_with("pub struct ") || line.begins_with("struct ")) {
			int start = line.find("struct ") + 7;
			int end = line.find_char('{', start);
			if (end == -1) {
				end = line.find_char(';', start);
			}
			if (end == -1) {
				end = line.length();
			}
			String sname = line.substr(start, end - start).strip_edges();
			if (!sname.is_empty()) {
				cdoc.name = sname;
				class_name = sname;
			}
		}

		// Check for function
		int fn_pos = line.find("fn ");
		if (fn_pos != -1 && (fn_pos == 0 || line[fn_pos - 1] == ' ' || line[fn_pos - 1] == '\t')) {
			int name_start = fn_pos + 3;
			int paren_open = line.find_char('(', name_start);
			if (paren_open != -1) {
				String fn_name = line.substr(name_start, paren_open - name_start).strip_edges();
				int paren_close = line.find_char(')', paren_open);
				String params_str = "";
				if (paren_close != -1) {
					params_str = line.substr(paren_open + 1, paren_close - paren_open - 1).strip_edges();
				}

				String ret_type = "void";
				int arrow = line.find("->", paren_close != -1 ? paren_close : paren_open);
				if (arrow != -1) {
					int brace = line.find_char('{', arrow);
					int semi = line.find_char(';', arrow);
					int ret_end = line.length();
					if (brace != -1 && brace < ret_end) {
						ret_end = brace;
					}
					if (semi != -1 && semi < ret_end) {
						ret_end = semi;
					}
					ret_type = _map_rust_type_to_godot(line.substr(arrow + 2, ret_end - arrow - 2));
				}

				DocData::MethodDoc mdoc;
				mdoc.name = fn_name;
				mdoc.description = accumulated_item_doc;
				mdoc.return_type = ret_type;

				if (!params_str.is_empty()) {
					PackedStringArray params = params_str.split(",");
					for (int p = 0; p < params.size(); p++) {
						String param = params[p].strip_edges();
						if (param.begins_with("&self") || param.begins_with("&mut self") || param == "self") {
							continue;
						}
						int colon = param.find_char(':');
						if (colon != -1) {
							DocData::ArgumentDoc adoc;
							adoc.name = param.substr(0, colon).strip_edges();
							adoc.type = _map_rust_type_to_godot(param.substr(colon + 1));
							mdoc.arguments.push_back(adoc);
						}
					}
				}

				cdoc.methods.push_back(mdoc);
				accumulated_item_doc = "";
				continue;
			}
		}

		// Check for struct field property: "pub name: Type," or "name: Type,"
		if (!accumulated_item_doc.is_empty() && line.contains(":") && (line.ends_with(",") || line.ends_with(";"))) {
			String pline = line;
			if (pline.begins_with("pub ")) {
				pline = pline.substr(4).strip_edges();
			}
			int colon = pline.find_char(':');
			if (colon != -1) {
				String prop_name = pline.substr(0, colon).strip_edges();
				String prop_type = pline.substr(colon + 1).replace(",", "").replace(";", "").strip_edges();

				DocData::PropertyDoc pdoc;
				pdoc.name = prop_name;
				pdoc.type = _map_rust_type_to_godot(prop_type);
				pdoc.description = accumulated_item_doc;
				cdoc.properties.push_back(pdoc);
				accumulated_item_doc = "";
				continue;
			}
		}

		accumulated_item_doc = "";
	}

	int first_nl = accumulated_class_doc.find("\n\n");
	if (first_nl != -1) {
		cdoc.brief_description = accumulated_class_doc.substr(0, first_nl).strip_edges();
		cdoc.description = accumulated_class_doc.substr(first_nl + 2).strip_edges();
	} else {
		cdoc.brief_description = accumulated_class_doc.strip_edges();
		cdoc.description = "";
	}

	docs.push_back(cdoc);
}
#endif

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

void RustScript::set_source_code(const String &p_code) {
	source_code = p_code;
#ifdef TOOLS_ENABLED
	_update_doc_data();
#endif
}

void RustScript::set_path(const String &p_path, bool p_take_over) {
	path = p_path;
	Script::set_path(p_path, p_take_over);
#ifdef TOOLS_ENABLED
	_update_doc_data();
#endif
}

Error RustScript::reload(bool p_keep_state) {
#ifdef TOOLS_ENABLED
	_update_doc_data();
#endif
	return OK;
}

bool RustScript::has_method(const StringName &p_method) const {
	if (p_method == SNAME("_ready") || p_method == SNAME("_process") || p_method == SNAME("_physics_process")) {
		return true;
	}
#ifdef TOOLS_ENABLED
	if (!docs.is_empty()) {
		for (const DocData::MethodDoc &m : docs[0].methods) {
			if (m.name == p_method) {
				return true;
			}
		}
	}
#endif
	return false;
}

MethodInfo RustScript::get_method_info(const StringName &p_method) const {
	MethodInfo mi;
	mi.name = p_method;
	if (p_method == SNAME("_process") || p_method == SNAME("_physics_process")) {
		PropertyInfo pi;
		pi.name = "delta";
		pi.type = Variant::FLOAT;
		mi.arguments.push_back(pi);
		return mi;
	}
#ifdef TOOLS_ENABLED
	if (!docs.is_empty()) {
		for (const DocData::MethodDoc &m : docs[0].methods) {
			if (m.name == p_method) {
				for (const DocData::ArgumentDoc &a : m.arguments) {
					PropertyInfo pi;
					pi.name = a.name;
					if (a.type == "float") {
						pi.type = Variant::FLOAT;
					} else if (a.type == "int") {
						pi.type = Variant::INT;
					} else if (a.type == "bool") {
						pi.type = Variant::BOOL;
					} else if (a.type == "String") {
						pi.type = Variant::STRING;
					} else if (a.type == "Vector3") {
						pi.type = Variant::VECTOR3;
					} else if (a.type == "Vector2") {
						pi.type = Variant::VECTOR2;
					} else if (a.type == "Transform3D") {
						pi.type = Variant::TRANSFORM3D;
					} else {
						pi.type = Variant::NIL;
					}
					mi.arguments.push_back(pi);
				}
				if (m.return_type == "float") {
					mi.return_val.type = Variant::FLOAT;
				} else if (m.return_type == "int") {
					mi.return_val.type = Variant::INT;
				} else if (m.return_type == "bool") {
					mi.return_val.type = Variant::BOOL;
				} else if (m.return_type == "String") {
					mi.return_val.type = Variant::STRING;
				} else if (m.return_type == "Vector3") {
					mi.return_val.type = Variant::VECTOR3;
				} else if (m.return_type == "Vector2") {
					mi.return_val.type = Variant::VECTOR2;
				} else if (m.return_type == "Transform3D") {
					mi.return_val.type = Variant::TRANSFORM3D;
				}
				return mi;
			}
		}
	}
#endif
	return mi;
}

ScriptLanguage *RustScript::get_language() const {
	return RustLanguage::get_singleton();
}

void RustScript::get_script_method_list(List<MethodInfo> *p_list) const {
	p_list->push_back(get_method_info(SNAME("_ready")));
	p_list->push_back(get_method_info(SNAME("_process")));
	p_list->push_back(get_method_info(SNAME("_physics_process")));
#ifdef TOOLS_ENABLED
	if (!docs.is_empty()) {
		for (const DocData::MethodDoc &m : docs[0].methods) {
			if (m.name != "_ready" && m.name != "_process" && m.name != "_physics_process") {
				p_list->push_back(get_method_info(m.name));
			}
		}
	}
#endif
}

void RustScript::get_script_property_list(List<PropertyInfo> *p_list) const {
#ifdef TOOLS_ENABLED
	if (!docs.is_empty()) {
		for (const DocData::PropertyDoc &p : docs[0].properties) {
			PropertyInfo pi;
			pi.name = p.name;
			if (p.type == "float") {
				pi.type = Variant::FLOAT;
			} else if (p.type == "int") {
				pi.type = Variant::INT;
			} else if (p.type == "bool") {
				pi.type = Variant::BOOL;
			} else if (p.type == "String") {
				pi.type = Variant::STRING;
			} else if (p.type == "Vector3") {
				pi.type = Variant::VECTOR3;
			} else if (p.type == "Vector2") {
				pi.type = Variant::VECTOR2;
			} else if (p.type == "Transform3D") {
				pi.type = Variant::TRANSFORM3D;
			} else {
				pi.type = Variant::NIL;
			}
			p_list->push_back(pi);
		}
	}
#endif
}

RustScript::RustScript() {
}

RustScript::~RustScript() {
}
