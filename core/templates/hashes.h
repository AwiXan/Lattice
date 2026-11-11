/**************************************************************************/
/*  hashes.h                                                              */
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

#include "core/simd/simd.h"
#include "core/typedefs.h"

// A group of GROUP_SIZE hash fingerprints that can be compared against a
// value in one SIMD operation. The table is padded with GROUP_SIZE bytes
// past its end, so a group may be loaded starting at any valid slot.
struct HashGroup {
#ifdef SIMD_AVAILABLE
	static constexpr uint32_t GROUP_SIZE = 16;

private:
#ifdef SIMD_SSE2
	__m128i vector;
#elif defined(SIMD_NEON)
	uint8x16_t vector;
#endif

public:
	_FORCE_INLINE_ HashGroup(const uint8_t *p_begin) {
#ifdef SIMD_SSE2
		vector = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p_begin));
#elif defined(SIMD_NEON)
		vector = vld1q_u8(p_begin);
#endif
	}

	// Bit i of the returned mask is set when the i-th byte of the group equals p_value.
	_FORCE_INLINE_ int get_compare_mask(uint8_t p_value) const {
#ifdef SIMD_SSE2
		__m128i target_vector = _mm_set1_epi8(p_value);
		__m128i cmp = _mm_cmpeq_epi8(target_vector, vector);
		return _mm_movemask_epi8(cmp);
#elif defined(SIMD_NEON)
		uint8x16_t target_vector = vdupq_n_u8(p_value);
		uint8x16_t cmp = vceqq_u8(target_vector, vector);
		alignas(16) uint8_t lanes[16];
		vst1q_u8(lanes, cmp);
		int mask = 0;
		for (uint32_t i = 0; i < 16; i++) {
			mask |= (lanes[i] != 0) << i;
		}
		return mask;
#endif
	}
#else
	static constexpr uint32_t GROUP_SIZE = 1;
#endif // SIMD_AVAILABLE
};

// Open addressing hash slot storage: one fingerprint byte per slot.
// Linear probing, with SIMD group scans where available.
//
// Slot states: EMPTY_HASH (never used), DELETED_HASH (tombstone) and
// END_HASH (padding sentinel, counts as occupied); real fingerprints are
// always >= 3, so sentinel bytes never produce a false match.
//
// Invariant the containers must keep: there is always at least one
// EMPTY_HASH slot, so lookups and insertions terminate.
struct Hashes {
	static constexpr uint8_t EMPTY_HASH = 0;
	static constexpr uint8_t DELETED_HASH = 1;
	static constexpr uint8_t END_HASH = 2;

	uint8_t *ptr = nullptr;

	// The top byte of the hash, remapped to never collide with the sentinels.
	static _FORCE_INLINE_ uint8_t get_fingerprint(uint32_t p_hash) {
		uint8_t fingerprint = (p_hash >> 24) & 0xff;
		if (unlikely(fingerprint <= END_HASH)) {
			fingerprint += END_HASH + 1;
		}
		return fingerprint;
	}

	// p_container must provide `_compare_function(p_hash_idx, p_key) -> bool`.
	template <typename Container, typename TKey>
	inline bool lookup_pos_with_hash(const Container *p_container, const TKey &p_key, uint32_t p_hash, uint32_t p_capacity_mask, uint32_t &r_hash_idx) const {
		uint32_t idx = p_hash & p_capacity_mask;

		if (ptr[idx] == EMPTY_HASH) {
			return false;
		}

		const uint8_t fingerprint = get_fingerprint(p_hash);

		if (ptr[idx] == fingerprint && likely(p_container->_compare_function(idx, p_key))) {
			r_hash_idx = idx;
			return true;
		}

		idx++;
#ifdef SIMD_AVAILABLE
		// Safety net: one full lap plus the groups overlapping the padding.
		uint32_t scanned = 0;
		const uint32_t max_scan = (p_capacity_mask + 1) + 2 * HashGroup::GROUP_SIZE;
		while (true) { // Compares GROUP_SIZE fingerprints at once.
			if (idx > p_capacity_mask) {
				idx = 0;
			}

			HashGroup group(&ptr[idx]);
			uint32_t empty_mask = static_cast<uint32_t>(group.get_compare_mask(EMPTY_HASH));
			uint32_t mask = static_cast<uint32_t>(group.get_compare_mask(fingerprint));
			if (empty_mask) {
				uint32_t first_empty = count_trailing_zeros(empty_mask);
				mask &= (1u << first_empty) - 1;
			}
			while (mask) {
				uint32_t actual_idx = idx + count_trailing_zeros(mask);
				if (likely(p_container->_compare_function(actual_idx, p_key))) {
					r_hash_idx = actual_idx;
					return true;
				}

				mask &= mask - 1;
			}

			if (likely(empty_mask)) {
				return false;
			}

			idx += HashGroup::GROUP_SIZE;
			scanned += HashGroup::GROUP_SIZE;
			if (unlikely(scanned > max_scan)) {
				return false; // Table invariant broken, bail out instead of looping forever.
			}
		}
#else
		uint32_t distance = 1;
		while (true) {
			if (idx > p_capacity_mask) {
				idx = 0;
			}

			if (ptr[idx] == fingerprint && likely(p_container->_compare_function(idx, p_key))) {
				r_hash_idx = idx;
				return true;
			}

			if (ptr[idx] == EMPTY_HASH) {
				return false;
			}

			idx++;
			distance++;
			if (unlikely(distance > p_capacity_mask + 1)) {
				return false;
			}
		}
#endif
	}

	// Finds a slot for a new entry with the given hash and marks it occupied.
	// r_reused_tombstone is set to true when a DELETED slot was recycled.
	uint32_t insert_hash(uint32_t p_hash, uint32_t p_capacity_mask, bool &r_reused_tombstone);

	_FORCE_INLINE_ void delete_hash(uint32_t p_hash_idx) {
		ptr[p_hash_idx] = DELETED_HASH;
	}
};
