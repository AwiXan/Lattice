/**************************************************************************/
/*  multiplayer_profiler.h                                                */
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

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/variant/dictionary.h"

// What a SceneMultiplayer sends and receives, for the game itself to read while
// it runs, release builds included: packets and bytes, and per node its RPCs
// and replication. Off until enabled; off, recording costs one check.
class MultiplayerProfiler : public RefCounted {
	GDCLASS(MultiplayerProfiler, RefCounted);

public:
	enum NodeTraffic {
		RPC_IN,
		RPC_OUT,
		SYNC_IN,
		SYNC_OUT,
		DELTA_IN,
		DELTA_OUT,
		TRAFFIC_MAX
	};

private:
	struct Counts {
		uint64_t count = 0;
		uint64_t bytes = 0;

		void add(int p_bytes) {
			count++;
			bytes += p_bytes;
		}
	};

	// Per second: the one going on, and the last whole one, which is what is
	// reported.
	struct Traffic {
		Counts total;
		Counts current;
		Counts last;

		void add(int p_bytes) {
			total.add(p_bytes);
			current.add(p_bytes);
		}
		void roll(bool p_skipped) {
			last = p_skipped ? Counts() : current;
			current = Counts();
		}
	};

	bool enabled = false;
	uint64_t second_start = 0;
	Traffic packets_in;
	Traffic packets_out;
	struct PerNode {
		Traffic traffic[TRAFFIC_MAX];
	};
	HashMap<ObjectID, PerNode> nodes;

	void _roll(uint64_t p_now);
	void _record_packet(bool p_out, int p_bytes);
	void _record_node(NodeTraffic p_what, ObjectID p_node, int p_bytes);

protected:
	static void _bind_methods();

public:
	void set_enabled(bool p_enabled);
	bool is_enabled() const { return enabled; }

	// A check while off, so a game that never turns it on pays next to nothing.
	_FORCE_INLINE_ void record_packet(bool p_out, int p_bytes) {
		if (enabled) {
			_record_packet(p_out, p_bytes);
		}
	}
	_FORCE_INLINE_ void record_node(NodeTraffic p_what, ObjectID p_node, int p_bytes) {
		if (enabled) {
			_record_node(p_what, p_node, p_bytes);
		}
	}

	Dictionary get_data();
	void reset();
};
