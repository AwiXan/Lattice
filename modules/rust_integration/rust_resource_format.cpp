/**************************************************************************/
/*  rust_resource_format.cpp                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_resource_format.h"

#include "core/io/file_access.h"
#include "rust_script.h"

Ref<Resource> ResourceFormatLoaderRustScript::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Error err;
	String code = FileAccess::get_file_as_string(p_path, &err);
	if (err != OK) {
		return Ref<Resource>();
	}

	Ref<RustScript> scr;
	scr.instantiate();
	scr->set_source_code(code);
	scr->set_path(p_path);

	if (r_error) {
		*r_error = OK;
	}
	return scr;
}

void ResourceFormatLoaderRustScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("rs");
}

bool ResourceFormatLoaderRustScript::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "RustScript" || p_type == "Resource";
}

String ResourceFormatLoaderRustScript::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "rs") {
		return "RustScript";
	}
	return "";
}

Error ResourceFormatSaverRustScript::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<RustScript> scr = p_resource;
	if (scr.is_null()) {
		return ERR_INVALID_PARAMETER;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);
	if (err != OK) {
		return err;
	}

	f->store_string(scr->get_source_code());
	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void ResourceFormatSaverRustScript::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<RustScript>(p_resource.ptr())) {
		p_extensions->push_back("rs");
	}
}

bool ResourceFormatSaverRustScript::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<RustScript>(p_resource.ptr()) != nullptr;
}
