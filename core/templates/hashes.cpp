/**************************************************************************/
/*  hashes.cpp                                                            */
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

#include "hashes.h"

uint32_t Hashes::insert_hash(uint32_t p_hash, uint32_t p_capacity_mask, bool &r_reused_tombstone) {
	uint32_t idx = p_hash & p_capacity_mask;
	const uint8_t fingerprint = get_fingerprint(p_hash);

	if (ptr[idx] <= DELETED_HASH) {
		r_reused_tombstone = ptr[idx] == DELETED_HASH;
		ptr[idx] = fingerprint;
		return idx;
	}
	idx++;

#ifdef SIMD_AVAILABLE
	while (true) {
		if (idx > p_capacity_mask) {
			idx = 0;
		}

		HashGroup group(&ptr[idx]);
		int mask = group.get_compare_mask(EMPTY_HASH) | group.get_compare_mask(DELETED_HASH);
		if (likely(mask)) {
			uint32_t actual_idx = idx + count_trailing_zeros(static_cast<uint32_t>(mask));
			r_reused_tombstone = ptr[actual_idx] == DELETED_HASH;
			ptr[actual_idx] = fingerprint;
			return actual_idx;
		}

		idx += HashGroup::GROUP_SIZE;
	}
#else
	while (true) {
		if (idx > p_capacity_mask) {
			idx = 0;
		}

		if (ptr[idx] <= DELETED_HASH) {
			r_reused_tombstone = ptr[idx] == DELETED_HASH;
			ptr[idx] = fingerprint;
			return idx;
		}
		idx++;
	}
#endif
}
