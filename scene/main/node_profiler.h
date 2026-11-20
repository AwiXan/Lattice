/**************************************************************************/
/*  node_profiler.h                                                       */
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

#pragma once

#include "core/object/object_id.h"
#include "core/os/os.h"
#include "core/os/spin_lock.h"
#include "core/string/node_path.h"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/variant/array.h"

class Node;

class NodeProfiler {
public:
	struct NodeData {
		ObjectID id;
		NodePath path;
		StringName class_name;
		uint64_t process_usec = 0;
		uint64_t physics_process_usec = 0;
		uint32_t process_calls = 0;
		uint32_t physics_calls = 0;
	};

private:
	static bool profiling_enabled;
	static HashMap<ObjectID, NodeData> current_frame_data;
	static SpinLock lock;

public:
	static void set_profiling_enabled(bool p_enabled);
	_FORCE_INLINE_ static bool is_profiling_enabled() { return profiling_enabled; }

	static void record_process(Node *p_node, uint64_t p_usec);
	static void record_physics_process(Node *p_node, uint64_t p_usec);

	static Array finish_frame();
};
