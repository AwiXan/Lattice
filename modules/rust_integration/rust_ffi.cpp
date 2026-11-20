/**************************************************************************/
/*  rust_ffi.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "rust_ffi.h"

#include "core/object/worker_thread_pool.h"
#include "scene/3d/node_3d.h"

uint32_t godot_rust_api_version() {
	return 1;
}

void godot_rust_print(const char *p_msg) {
	if (!p_msg) {
		return;
	}
	print_line(String::utf8(p_msg));
}

void godot_rust_print_warning(const char *p_msg, const char *p_func, const char *p_file, int p_line) {
	if (!p_msg) {
		return;
	}
	_err_print_error(p_func ? p_func : "Rust", p_file ? p_file : "rust_source.rs", p_line, String::utf8(p_msg), true, ERR_HANDLER_WARNING);
}

void godot_rust_print_error(const char *p_msg, const char *p_func, const char *p_file, int p_line) {
	if (!p_msg) {
		return;
	}
	_err_print_error(p_func ? p_func : "Rust", p_file ? p_file : "rust_source.rs", p_line, String::utf8(p_msg), false, ERR_HANDLER_ERROR);
}

const uint8_t *godot_rust_packed_byte_array_read(const void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	const PackedByteArray *arr = reinterpret_cast<const PackedByteArray *>(p_array_handle);
	return arr->ptr();
}

uint8_t *godot_rust_packed_byte_array_write(void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	PackedByteArray *arr = reinterpret_cast<PackedByteArray *>(p_array_handle);
	return arr->ptrw();
}

int64_t godot_rust_packed_byte_array_size(const void *p_array_handle) {
	if (!p_array_handle) {
		return 0;
	}
	const PackedByteArray *arr = reinterpret_cast<const PackedByteArray *>(p_array_handle);
	return arr->size();
}

const float *godot_rust_packed_float32_array_read(const void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	const PackedFloat32Array *arr = reinterpret_cast<const PackedFloat32Array *>(p_array_handle);
	return arr->ptr();
}

float *godot_rust_packed_float32_array_write(void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	PackedFloat32Array *arr = reinterpret_cast<PackedFloat32Array *>(p_array_handle);
	return arr->ptrw();
}

int64_t godot_rust_packed_float32_array_size(const void *p_array_handle) {
	if (!p_array_handle) {
		return 0;
	}
	const PackedFloat32Array *arr = reinterpret_cast<const PackedFloat32Array *>(p_array_handle);
	return arr->size();
}

const void *godot_rust_packed_vector3_array_read(const void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	const PackedVector3Array *arr = reinterpret_cast<const PackedVector3Array *>(p_array_handle);
	return reinterpret_cast<const void *>(arr->ptr());
}

void *godot_rust_packed_vector3_array_write(void *p_array_handle) {
	if (!p_array_handle) {
		return nullptr;
	}
	PackedVector3Array *arr = reinterpret_cast<PackedVector3Array *>(p_array_handle);
	return reinterpret_cast<void *>(arr->ptrw());
}

int64_t godot_rust_packed_vector3_array_size(const void *p_array_handle) {
	if (!p_array_handle) {
		return 0;
	}
	const PackedVector3Array *arr = reinterpret_cast<const PackedVector3Array *>(p_array_handle);
	return arr->size();
}

int32_t godot_rust_node3d_get_transform(uint64_t p_instance_id, float r_basis[9], float r_origin[3]) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_instance_id));
	Node3D *node = Object::cast_to<Node3D>(obj);
	if (!node) {
		return -1;
	}
	Transform3D xform = node->get_transform();
	r_basis[0] = xform.basis.rows[0].x;
	r_basis[1] = xform.basis.rows[0].y;
	r_basis[2] = xform.basis.rows[0].z;
	r_basis[3] = xform.basis.rows[1].x;
	r_basis[4] = xform.basis.rows[1].y;
	r_basis[5] = xform.basis.rows[1].z;
	r_basis[6] = xform.basis.rows[2].x;
	r_basis[7] = xform.basis.rows[2].y;
	r_basis[8] = xform.basis.rows[2].z;
	r_origin[0] = xform.origin.x;
	r_origin[1] = xform.origin.y;
	r_origin[2] = xform.origin.z;
	return 0;
}

int32_t godot_rust_node3d_set_transform(uint64_t p_instance_id, const float p_basis[9], const float p_origin[3]) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_instance_id));
	Node3D *node = Object::cast_to<Node3D>(obj);
	if (!node) {
		return -1;
	}
	Basis b;
	b.rows[0] = Vector3(p_basis[0], p_basis[1], p_basis[2]);
	b.rows[1] = Vector3(p_basis[3], p_basis[4], p_basis[5]);
	b.rows[2] = Vector3(p_basis[6], p_basis[7], p_basis[8]);
	Vector3 o = Vector3(p_origin[0], p_origin[1], p_origin[2]);
	node->set_transform(Transform3D(b, o));
	return 0;
}

int32_t godot_rust_node3d_get_position(uint64_t p_instance_id, float r_pos[3]) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_instance_id));
	Node3D *node = Object::cast_to<Node3D>(obj);
	if (!node) {
		return -1;
	}
	Vector3 pos = node->get_position();
	r_pos[0] = pos.x;
	r_pos[1] = pos.y;
	r_pos[2] = pos.z;
	return 0;
}

int32_t godot_rust_node3d_set_position(uint64_t p_instance_id, float p_x, float p_y, float p_z) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_instance_id));
	Node3D *node = Object::cast_to<Node3D>(obj);
	if (!node) {
		return -1;
	}
	node->set_position(Vector3(p_x, p_y, p_z));
	return 0;
}

int64_t godot_rust_thread_pool_add_task(void (*p_task_func)(void *), void *p_userdata, bool p_high_priority) {
	if (!p_task_func) {
		return -1;
	}
	return (int64_t)WorkerThreadPool::get_singleton()->add_native_task(p_task_func, p_userdata, p_high_priority, SNAME("RustWorkerTask"));
}
