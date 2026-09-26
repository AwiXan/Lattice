/**************************************************************************/
/*  main_thread_work.h                                                    */
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

#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"

// What took the main thread's time since the last frame ended, for the hitch
// log (Performance.get_hitch_log()): loading resources and instantiating
// scenes - the outermost call of each only - with the slowest few by name.
// Main thread only: the log is about frames, which the main thread makes.
struct MainThreadWork {
	enum Kind {
		KIND_LOAD,
		KIND_INSTANTIATE,
		KIND_MAX,
	};

	struct Item {
		String what;
		uint64_t usec = 0;
	};
	static constexpr uint32_t SLOWEST = 8;

	static inline uint64_t usec[KIND_MAX] = {};
	static inline uint32_t count[KIND_MAX] = {};
	static inline LocalVector<Item> slowest[KIND_MAX];
	static inline uint32_t depth[KIND_MAX] = {};

	class Scope {
		int kind = -1;
		bool outermost = false;
		uint64_t begin = 0;
		String what;

	public:
		Scope(Kind p_kind, const String &p_what) {
			if (!Thread::is_main_thread()) {
				return;
			}
			kind = p_kind;
			outermost = depth[kind]++ == 0;
			if (outermost) {
				what = p_what;
				begin = OS::get_singleton()->get_ticks_usec();
			}
		}

		~Scope() {
			if (kind < 0) {
				return;
			}
			depth[kind]--;
			if (!outermost) {
				return;
			}
			const uint64_t spent = OS::get_singleton()->get_ticks_usec() - begin;
			usec[kind] += spent;
			count[kind]++;
			LocalVector<Item> &list = slowest[kind];
			if (list.size() < SLOWEST) {
				list.push_back({ what, spent });
				return;
			}
			uint32_t least = 0;
			for (uint32_t i = 1; i < list.size(); i++) {
				if (list[i].usec < list[least].usec) {
					least = i;
				}
			}
			if (spent > list[least].usec) {
				list[least] = { what, spent };
			}
		}
	};

	static void reset() {
		for (int i = 0; i < KIND_MAX; i++) {
			usec[i] = 0;
			count[i] = 0;
			slowest[i].clear();
		}
	}
};
