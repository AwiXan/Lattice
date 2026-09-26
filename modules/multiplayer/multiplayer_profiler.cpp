/**************************************************************************/
/*  multiplayer_profiler.cpp                                              */
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

#include "multiplayer_profiler.h"

#include "core/object/class_db.h"
#include "core/os/os.h"
#include "scene/main/node.h"

static const char *traffic_names[MultiplayerProfiler::TRAFFIC_MAX] = {
	"rpc_in",
	"rpc_out",
	"sync_in",
	"sync_out",
	"delta_in",
	"delta_out",
};

void MultiplayerProfiler::_roll(uint64_t p_now) {
	if (p_now - second_start < 1000) {
		return;
	}
	// A whole second with nothing in it means nothing for the last one.
	const bool skipped = p_now - second_start >= 2000;
	packets_in.roll(skipped);
	packets_out.roll(skipped);
	for (KeyValue<ObjectID, PerNode> &E : nodes) {
		for (int i = 0; i < TRAFFIC_MAX; i++) {
			E.value.traffic[i].roll(skipped);
		}
	}
	second_start = skipped ? p_now : second_start + 1000;
}

void MultiplayerProfiler::set_enabled(bool p_enabled) {
	if (p_enabled && !enabled) {
		// From nothing: what was counted before it was switched off is gone.
		reset();
	}
	enabled = p_enabled;
}

void MultiplayerProfiler::_record_packet(bool p_out, int p_bytes) {
	_roll(OS::get_singleton()->get_ticks_msec());
	(p_out ? packets_out : packets_in).add(p_bytes);
}

void MultiplayerProfiler::_record_node(NodeTraffic p_what, ObjectID p_node, int p_bytes) {
	ERR_FAIL_INDEX(p_what, TRAFFIC_MAX);
	_roll(OS::get_singleton()->get_ticks_msec());
	nodes[p_node].traffic[p_what].add(p_bytes);
}

Dictionary MultiplayerProfiler::get_data() {
	_roll(OS::get_singleton()->get_ticks_msec());

	Dictionary data;
	data["packets_in"] = packets_in.last.count;
	data["bytes_in"] = packets_in.last.bytes;
	data["packets_out"] = packets_out.last.count;
	data["bytes_out"] = packets_out.last.bytes;
	data["total_packets_in"] = packets_in.total.count;
	data["total_bytes_in"] = packets_in.total.bytes;
	data["total_packets_out"] = packets_out.total.count;
	data["total_bytes_out"] = packets_out.total.bytes;

	Array node_list;
	LocalVector<ObjectID> gone;
	for (KeyValue<ObjectID, PerNode> &E : nodes) {
		Node *node = ObjectDB::get_instance<Node>(E.key);
		bool quiet = true;
		Dictionary entry;
		entry["node"] = (node && node->is_inside_tree()) ? node->get_path() : NodePath();
		entry["name"] = node ? String(node->get_name()) : String();
		for (int i = 0; i < TRAFFIC_MAX; i++) {
			const Traffic &traffic = E.value.traffic[i];
			const String name = traffic_names[i];
			entry[name] = traffic.last.count;
			entry[name + "_bytes"] = traffic.last.bytes;
			entry["total_" + name] = traffic.total.count;
			entry["total_" + name + "_bytes"] = traffic.total.bytes;
			quiet = quiet && traffic.last.count == 0 && traffic.current.count == 0;
		}
		if (!node && quiet) {
			// Freed and done with: let go of, or a game spawning and freeing
			// nodes all match long would keep every one of them.
			gone.push_back(E.key);
			continue;
		}
		node_list.push_back(entry);
	}
	for (const ObjectID &id : gone) {
		nodes.erase(id);
	}
	data["nodes"] = node_list;
	return data;
}

void MultiplayerProfiler::reset() {
	packets_in = Traffic();
	packets_out = Traffic();
	nodes.clear();
	second_start = OS::get_singleton()->get_ticks_msec();
}

void MultiplayerProfiler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &MultiplayerProfiler::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &MultiplayerProfiler::is_enabled);
	ClassDB::bind_method(D_METHOD("get_data"), &MultiplayerProfiler::get_data);
	ClassDB::bind_method(D_METHOD("reset"), &MultiplayerProfiler::reset);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
}
