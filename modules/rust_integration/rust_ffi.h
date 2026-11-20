/**************************************************************************/
/*  rust_ffi.h                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/object.h"
#include "core/os/os.h"
#include "core/variant/variant.h"

#ifdef __cplusplus
extern "C" {
#endif

// API version for Rust bridge handshake
uint32_t godot_rust_api_version();

// Diagnostics & Logging
void godot_rust_print(const char *p_msg);
void godot_rust_print_warning(const char *p_msg, const char *p_func, const char *p_file, int p_line);
void godot_rust_print_error(const char *p_msg, const char *p_func, const char *p_file, int p_line);

// Direct zero-copy memory access for PackedArrays
const uint8_t *godot_rust_packed_byte_array_read(const void *p_array_handle);
uint8_t *godot_rust_packed_byte_array_write(void *p_array_handle);
int64_t godot_rust_packed_byte_array_size(const void *p_array_handle);

const float *godot_rust_packed_float32_array_read(const void *p_array_handle);
float *godot_rust_packed_float32_array_write(void *p_array_handle);
int64_t godot_rust_packed_float32_array_size(const void *p_array_handle);

const void *godot_rust_packed_vector3_array_read(const void *p_array_handle);
void *godot_rust_packed_vector3_array_write(void *p_array_handle);
int64_t godot_rust_packed_vector3_array_size(const void *p_array_handle);

// Direct raw Node3D transform access (bypassing Variant boxing)
int32_t godot_rust_node3d_get_transform(uint64_t p_instance_id, float r_basis[9], float r_origin[3]);
int32_t godot_rust_node3d_set_transform(uint64_t p_instance_id, const float p_basis[9], const float p_origin[3]);
int32_t godot_rust_node3d_get_position(uint64_t p_instance_id, float r_pos[3]);
int32_t godot_rust_node3d_set_position(uint64_t p_instance_id, float p_x, float p_y, float p_z);

// Direct multi-threading dispatch via WorkerThreadPool
int64_t godot_rust_thread_pool_add_task(void (*p_task_func)(void *), void *p_userdata, bool p_high_priority);

#ifdef __cplusplus
}
#endif
