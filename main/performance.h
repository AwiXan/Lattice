/**************************************************************************/
/*  performance.h                                                         */
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

#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/variant/type_info.h"

#include "modules/modules_enabled.gen.h"

#define PERF_WARN_OFFLINE_FUNCTION
#define PERF_WARN_PROCESS_SYNC

template <typename T>
class TypedArray;

class Performance : public Object {
	GDCLASS(Performance, Object);

	static Performance *singleton;
	static void _bind_methods();

#ifndef DISABLE_DEPRECATED
	void _add_custom_monitor_bind_compat_110433(const StringName &p_id, const Callable &p_callable, const Vector<Variant> &p_args);
	static void _bind_compatibility_methods();
#endif

	int _get_node_count() const;
	int _get_orphan_node_count() const;

	double _process_time;
	double _physics_process_time;
	double _navigation_process_time;

	// Frames at least hitch_threshold_msec long, and what went on in them.
	static constexpr uint32_t HITCH_LOG_SIZE = 32;
	double hitch_threshold_msec = 50.0;
	// Taken from the project settings when first wanted: they are not
	// there yet when this is made. A value set by the game stays.
	bool hitch_threshold_read = false;
	LocalVector<Dictionary> hitch_log;
	bool frame_work_known = false;
	uint64_t frame_process_usec = 0;
	uint64_t frame_render_usec = 0;
	uint64_t frame_physics_usec = 0;
	int frame_physics_steps = 0;
	uint64_t last_shader_wait_usec = 0;
	uint32_t last_shaders_done = 0;
	uint32_t last_pipelines_done = 0;
	double _hitch_threshold();

public:
	enum Monitor {
		TIME_FPS,
		TIME_PROCESS,
		TIME_PHYSICS_PROCESS,
		TIME_NAVIGATION_PROCESS,
		MEMORY_STATIC,
		MEMORY_STATIC_MAX,
		MEMORY_MESSAGE_BUFFER_MAX,
		OBJECT_COUNT,
		OBJECT_RESOURCE_COUNT,
		OBJECT_NODE_COUNT,
		OBJECT_ORPHAN_NODE_COUNT,
		RENDER_TOTAL_OBJECTS_IN_FRAME,
		RENDER_TOTAL_PRIMITIVES_IN_FRAME,
		RENDER_TOTAL_DRAW_CALLS_IN_FRAME,
		RENDER_VIDEO_MEM_USED,
		RENDER_TEXTURE_MEM_USED,
		RENDER_BUFFER_MEM_USED,
		PHYSICS_2D_ACTIVE_OBJECTS,
		PHYSICS_2D_COLLISION_PAIRS,
		PHYSICS_2D_ISLAND_COUNT,
		PHYSICS_3D_ACTIVE_OBJECTS,
		PHYSICS_3D_COLLISION_PAIRS,
		PHYSICS_3D_ISLAND_COUNT,
		AUDIO_OUTPUT_LATENCY,
		// Deprecated, use the 2D/3D specific ones instead.
		NAVIGATION_ACTIVE_MAPS,
		NAVIGATION_REGION_COUNT,
		NAVIGATION_AGENT_COUNT,
		NAVIGATION_LINK_COUNT,
		NAVIGATION_POLYGON_COUNT,
		NAVIGATION_EDGE_COUNT,
		NAVIGATION_EDGE_MERGE_COUNT,
		NAVIGATION_EDGE_CONNECTION_COUNT,
		NAVIGATION_EDGE_FREE_COUNT,
		NAVIGATION_OBSTACLE_COUNT,
		PIPELINE_COMPILATIONS_CANVAS,
		PIPELINE_COMPILATIONS_MESH,
		PIPELINE_COMPILATIONS_SURFACE,
		PIPELINE_COMPILATIONS_DRAW,
		PIPELINE_COMPILATIONS_SPECIALIZATION,
		NAVIGATION_2D_ACTIVE_MAPS,
		NAVIGATION_2D_REGION_COUNT,
		NAVIGATION_2D_AGENT_COUNT,
		NAVIGATION_2D_LINK_COUNT,
		NAVIGATION_2D_POLYGON_COUNT,
		NAVIGATION_2D_EDGE_COUNT,
		NAVIGATION_2D_EDGE_MERGE_COUNT,
		NAVIGATION_2D_EDGE_CONNECTION_COUNT,
		NAVIGATION_2D_EDGE_FREE_COUNT,
		NAVIGATION_2D_OBSTACLE_COUNT,
#ifndef _3D_DISABLED
		NAVIGATION_3D_ACTIVE_MAPS,
		NAVIGATION_3D_REGION_COUNT,
		NAVIGATION_3D_AGENT_COUNT,
		NAVIGATION_3D_LINK_COUNT,
		NAVIGATION_3D_POLYGON_COUNT,
		NAVIGATION_3D_EDGE_COUNT,
		NAVIGATION_3D_EDGE_MERGE_COUNT,
		NAVIGATION_3D_EDGE_CONNECTION_COUNT,
		NAVIGATION_3D_EDGE_FREE_COUNT,
		NAVIGATION_3D_OBSTACLE_COUNT,
#endif // _3D_DISABLED
#ifdef MODULE_TEXTURE_STREAMING_ENABLED
		RENDER_STREAMING_TEXTURE_MEM_USED,
#endif
		MONITOR_MAX
	};

	enum MonitorType {
		MONITOR_TYPE_QUANTITY,
		MONITOR_TYPE_MEMORY,
		MONITOR_TYPE_TIME,
		MONITOR_TYPE_PERCENTAGE,
	};

	double get_monitor(Monitor p_monitor) const;
	String get_monitor_name(Monitor p_monitor) const;

	MonitorType get_monitor_type(Monitor p_monitor) const;

	void set_process_time(double p_pt);
	void set_physics_process_time(double p_pt);
	void set_navigation_process_time(double p_pt);

	// From Main, each frame: what its parts took; then, as the next one
	// begins, how long it took in all.
	void set_frame_work(uint64_t p_process_usec, uint64_t p_render_usec, uint64_t p_physics_usec, int p_physics_steps);
	void frame_ended(uint64_t p_frame_usec);

	Array get_hitch_log() const;
	void clear_hitch_log();
	void set_hitch_threshold_msec(double p_msec);
	double get_hitch_threshold_msec();

	void add_custom_monitor(const StringName &p_id, const Callable &p_callable, const Vector<Variant> &p_args, MonitorType p_type = MONITOR_TYPE_QUANTITY);
	void remove_custom_monitor(const StringName &p_id);
	bool has_custom_monitor(const StringName &p_id);
	Variant get_custom_monitor(const StringName &p_id);
	TypedArray<StringName> get_custom_monitor_names();
	Vector<int> get_custom_monitor_types();

	uint64_t get_monitor_modification_time();

	static Performance *get_singleton() { return singleton; }

	Performance();

private:
	class MonitorCall {
		MonitorType _type = MONITOR_TYPE_QUANTITY;
		Callable _callable;
		Vector<Variant> _arguments;

	public:
		MonitorCall(MonitorType p_type, const Callable &p_callable, const Vector<Variant> &p_arguments);
		MonitorCall();
		Variant call(bool &r_error, String &r_error_message);
		inline MonitorType get_monitor_type() const { return _type; }
	};

	HashMap<StringName, MonitorCall> _monitor_map;
	uint64_t _monitor_modification_time;
};

VARIANT_ENUM_CAST(Performance::Monitor);
VARIANT_ENUM_CAST(Performance::MonitorType);
