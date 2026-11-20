/**************************************************************************/
/*  node_profiler.cpp                                                     */
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

#include "node_profiler.h"

#include "scene/main/node.h"

bool NodeProfiler::profiling_enabled = false;
HashMap<ObjectID, NodeProfiler::NodeData> NodeProfiler::current_frame_data;
SpinLock NodeProfiler::lock;

void NodeProfiler::set_profiling_enabled(bool p_enabled) {
	lock.lock();
	profiling_enabled = p_enabled;
	if (!p_enabled) {
		current_frame_data.clear();
	}
	lock.unlock();
}

void NodeProfiler::record_process(Node *p_node, uint64_t p_usec) {
	if (!p_node) {
		return;
	}
	ObjectID id = p_node->get_instance_id();

	lock.lock();
	HashMap<ObjectID, NodeData>::Iterator E = current_frame_data.find(id);
	if (E) {
		E->value.process_usec += p_usec;
		E->value.process_calls++;
	} else {
		NodeData data;
		data.id = id;
		data.path = p_node->get_path();
		data.class_name = p_node->get_class_name();
		data.process_usec = p_usec;
		data.process_calls = 1;
		current_frame_data[id] = data;
	}
	lock.unlock();
}

void NodeProfiler::record_physics_process(Node *p_node, uint64_t p_usec) {
	if (!p_node) {
		return;
	}
	ObjectID id = p_node->get_instance_id();

	lock.lock();
	HashMap<ObjectID, NodeData>::Iterator E = current_frame_data.find(id);
	if (E) {
		E->value.physics_process_usec += p_usec;
		E->value.physics_calls++;
	} else {
		NodeData data;
		data.id = id;
		data.path = p_node->get_path();
		data.class_name = p_node->get_class_name();
		data.physics_process_usec = p_usec;
		data.physics_calls = 1;
		current_frame_data[id] = data;
	}
	lock.unlock();
}

Array NodeProfiler::finish_frame() {
	lock.lock();
	if (current_frame_data.is_empty()) {
		lock.unlock();
		return Array();
	}

	LocalVector<NodeData> sorted_nodes;
	sorted_nodes.reserve(current_frame_data.size());
	for (const KeyValue<ObjectID, NodeData> &E : current_frame_data) {
		sorted_nodes.push_back(E.value);
	}
	current_frame_data.clear();
	lock.unlock();

	// Sort descending by total time:
	for (uint32_t i = 1; i < sorted_nodes.size(); i++) {
		for (uint32_t j = i; j > 0 && (sorted_nodes[j - 1].process_usec + sorted_nodes[j - 1].physics_process_usec) < (sorted_nodes[j].process_usec + sorted_nodes[j].physics_process_usec); j--) {
			SWAP(sorted_nodes[j - 1], sorted_nodes[j]);
		}
	}

	uint32_t limit = MIN((uint32_t)64, (uint32_t)sorted_nodes.size());
	Array arr;
	for (uint32_t i = 0; i < limit; i++) {
		const NodeData &nd = sorted_nodes[i];
		Array node_item;
		node_item.push_back(nd.path);
		node_item.push_back(nd.class_name);
		node_item.push_back(double(nd.process_usec) / 1000.0);
		node_item.push_back(double(nd.physics_process_usec) / 1000.0);
		node_item.push_back(nd.process_calls);
		node_item.push_back(nd.physics_calls);
		node_item.push_back(nd.id);
		arr.push_back(node_item);
	}

	return arr;
}
