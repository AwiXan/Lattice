/**************************************************************************/
/*  hash_set.h                                                            */
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

#include "core/os/memory.h"
#include "core/math/math_funcs_binary.h"
#include "core/string/print_string.h" // IWYU pragma: keep. `WARN_VERBOSE` macro.
#include "core/templates/hashfuncs.h"
#include "core/templates/hashes.h"
#include "core/templates/index_array.h"

/**
 * Implementation of Set using a bidi indexed hash map.
 * Use RBSet instead of this only if the following conditions are met:
 *
 * - You need to keep an iterator or const pointer to Key and you intend to add/remove elements in the meantime.
 * - Iteration order does matter (via operator<)
 *
 */

template <typename TKey,
		typename Hasher = HashMapHasherDefault,
		typename Comparator = HashMapComparatorDefault<TKey>>
class HashSet {
public:
	// Kept for source compatibility; the table now uses a power-of-two capacity.
	static constexpr uint32_t MIN_CAPACITY_INDEX = 2;
	static constexpr float MAX_OCCUPANCY = 0.9375;
	static constexpr uint32_t INITIAL_CAPACITY = 16;
	static constexpr uint32_t EMPTY_HASH = 0;

private:
	TKey *_keys = nullptr;
	Hashes _hashes;
	IndexArray _hash_idx_to_key_idx;

	uint32_t _capacity_mask = INITIAL_CAPACITY - 1;
	uint32_t _keys_capacity = 0;
	uint32_t _size = 0;
	uint32_t _num_deleted = 0;

	_FORCE_INLINE_ uint32_t _hash(const TKey &p_key) const {
		return Hasher::hash(p_key);
	}

	static _FORCE_INLINE_ uint32_t _get_resize_count(uint32_t p_capacity_mask) {
		return p_capacity_mask - ((p_capacity_mask + 1) >> 4);
	}

public:
	_FORCE_INLINE_ bool _compare_function(uint32_t p_hash_idx, const TKey &p_key) const {
		return Comparator::compare(_keys[_hash_idx_to_key_idx.get_index(p_hash_idx)], p_key);
	}
private:

	bool _lookup_key_idx(const TKey &p_key, uint32_t &r_key_idx, uint32_t &r_hash_idx) const {
		if (_hashes.ptr == nullptr || !_hashes.lookup_pos_with_hash<HashSet, TKey>(this, p_key, _hash(p_key), _capacity_mask, r_hash_idx)) {
			return false;
		}
		r_key_idx = _hash_idx_to_key_idx.get_index(r_hash_idx);
		return true;
	}

	uint32_t _insert_metadata(uint32_t p_hash, uint32_t p_key_idx) {
		bool reused_tombstone = false;
		uint32_t hash_idx = _hashes.insert_hash(p_hash, _capacity_mask, reused_tombstone);
		_hash_idx_to_key_idx.set_index(hash_idx, p_key_idx);
		if (reused_tombstone) {
			_num_deleted--;
		}
		return hash_idx;
	}

	void _resize_and_rehash(uint32_t p_new_capacity) {
		uint32_t capacity = Math::next_power_of_2(MAX(INITIAL_CAPACITY, p_new_capacity));
		uint32_t mask = capacity - 1;
		while (_get_resize_count(mask) + 1 < _size) {
			capacity *= 2;
			mask = capacity - 1;
		}
		uint8_t *old_hashes = _hashes.ptr;
		_capacity_mask = mask;
		_hashes.ptr = reinterpret_cast<uint8_t *>(Memory::alloc_static_zeroed(capacity + HashGroup::GROUP_SIZE));
		memset(_hashes.ptr + capacity, Hashes::END_HASH, HashGroup::GROUP_SIZE);
		_hash_idx_to_key_idx.initialize(capacity, _get_resize_count(mask) + 1);
		_num_deleted = 0;
		if (_keys == nullptr) {
			_keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * MAX(2u, _keys_capacity)));
			_keys_capacity = MAX(2u, _keys_capacity);
		} else if (_keys_capacity < _size) {
			_keys = reinterpret_cast<TKey *>(Memory::realloc_static(_keys, sizeof(TKey) * _size));
			_keys_capacity = _size;
		}
		if (old_hashes != nullptr) {
			for (uint32_t i = 0; i < _size; i++) {
				_insert_metadata(_hash(_keys[i]), i);
			}
			Memory::free_static(old_hashes);
		}
	}

	_FORCE_INLINE_ int32_t _insert(const TKey &p_key) {
		uint32_t key_idx = 0;
		uint32_t hash_idx = 0;
		uint32_t hash = _hash(p_key);
		if (_lookup_key_idx(p_key, key_idx, hash_idx)) {
			return key_idx;
		}
		if (_hashes.ptr == nullptr) {
			_resize_and_rehash(_capacity_mask + 1);
		}
		if (_size + _num_deleted > _get_resize_count(_capacity_mask)) {
			if (_size > _get_resize_count(_capacity_mask)) {
				_resize_and_rehash((_capacity_mask + 1) * 4);
			} else {
				_resize_and_rehash(_capacity_mask + 1);
			}
		}
		if (_size == _keys_capacity) {
			uint32_t new_capacity = MAX(2u, _keys_capacity + MAX(1u, _keys_capacity / 2));
			_keys = reinterpret_cast<TKey *>(Memory::realloc_static(_keys, sizeof(TKey) * new_capacity));
			_keys_capacity = new_capacity;
		}
		memnew_placement(&_keys[_size], TKey(p_key));
		_insert_metadata(hash, _size);
		return _size++;
	}

	void _init_from(const HashSet &p_other) {
		_capacity_mask = p_other._capacity_mask;
		_size = p_other._size;
		_num_deleted = p_other._num_deleted;
		_keys_capacity = p_other._keys_capacity;
		if (_size == 0) {
			return;
		}
		_keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * _keys_capacity));
		for (uint32_t i = 0; i < _size; i++) {
			memnew_placement(&_keys[i], TKey(p_other._keys[i]));
		}
		uint32_t capacity = _capacity_mask + 1;
		_hashes.ptr = reinterpret_cast<uint8_t *>(Memory::alloc_static(sizeof(uint8_t) * (capacity + HashGroup::GROUP_SIZE)));
		memcpy(_hashes.ptr, p_other._hashes.ptr, capacity + HashGroup::GROUP_SIZE);
		_hash_idx_to_key_idx = p_other._hash_idx_to_key_idx;
	}

public:
	_FORCE_INLINE_ uint32_t get_capacity() const { return _capacity_mask + 1; }
	_FORCE_INLINE_ uint32_t size() const { return _size; }

	/* Standard Godot Container API */

	bool is_empty() const {
		return _size == 0;
	}

	void clear() {
		if (_hashes.ptr == nullptr) {
			return;
		}

		memset(_hashes.ptr, Hashes::EMPTY_HASH, _capacity_mask + 1);
		memset(_hashes.ptr + _capacity_mask + 1, Hashes::END_HASH, HashGroup::GROUP_SIZE);

		if constexpr (!std::is_trivially_destructible_v<TKey>) {
			for (uint32_t i = 0; i < _size; i++) {
				_keys[i].~TKey();
			}
		}

		_size = 0;
		_num_deleted = 0;
	}

	_FORCE_INLINE_ bool has(const TKey &p_key) const {
		uint32_t key_idx = 0;
		uint32_t hash_idx = 0;
		return _lookup_key_idx(p_key, key_idx, hash_idx);
	}

	bool erase(const TKey &p_key) {
		uint32_t key_idx = 0;
		uint32_t hash_idx = 0;
		bool exists = _lookup_key_idx(p_key, key_idx, hash_idx);

		if (!exists) {
			return false;
		}

		uint32_t moved_hash_idx = 0;
		if (key_idx + 1 < _size) {
			uint32_t moved_key_idx = 0;
			CRASH_COND_MSG(!_lookup_key_idx(_keys[_size - 1], moved_key_idx, moved_hash_idx) || moved_key_idx != _size - 1, "HashSet element index is inconsistent.");
		}

		_hashes.delete_hash(hash_idx);
		_num_deleted++;
		_keys[key_idx].~TKey();
		_size--;
		if (key_idx < _size) {
			// Not the last key, move the last one here to keep keys contiguous.
			memnew_placement(&_keys[key_idx], TKey(std::move(_keys[_size])));
			_hash_idx_to_key_idx.set_index(moved_hash_idx, key_idx);
			_keys[_size].~TKey();
		}

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old capacity.
	void reserve(uint32_t p_new_capacity) {
		if (p_new_capacity < _size) {
			WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This is likely a mistake.");
			return;
		}
		if (_keys_capacity < p_new_capacity) {
			_keys = reinterpret_cast<TKey *>(Memory::realloc_static(_keys, sizeof(TKey) * p_new_capacity));
			_keys_capacity = p_new_capacity;
		}
		uint32_t needed_capacity = MAX(INITIAL_CAPACITY, p_new_capacity);
		uint32_t needed_mask = Math::next_power_of_2(needed_capacity) - 1;
		while (_get_resize_count(needed_mask) + 1 < p_new_capacity) {
			needed_mask = needed_mask * 2 + 1;
		}
		if (needed_mask > _capacity_mask) {
			if (_hashes.ptr == nullptr) {
				_capacity_mask = needed_mask;
			} else {
				_resize_and_rehash(needed_mask + 1);
			}
		}
	}

	/** Iterator API **/

	struct Iterator {
		_FORCE_INLINE_ const TKey &operator*() const {
			return _keys[_key_idx];
		}
		_FORCE_INLINE_ const TKey *operator->() const {
			return &_keys[_key_idx];
		}
		_FORCE_INLINE_ Iterator &operator++() {
			_key_idx++;
			if (_key_idx >= (int32_t)_num_keys) {
				_key_idx = -1;
				_keys = nullptr;
				_num_keys = 0;
			}
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			_key_idx--;
			if (_key_idx < 0) {
				_key_idx = -1;
				_keys = nullptr;
				_num_keys = 0;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &b) const { return _keys == b._keys && _key_idx == b._key_idx; }
		_FORCE_INLINE_ bool operator!=(const Iterator &b) const { return _keys != b._keys || _key_idx != b._key_idx; }

		_FORCE_INLINE_ explicit operator bool() const {
			return _keys != nullptr;
		}

		_FORCE_INLINE_ Iterator(const TKey *p_keys, uint32_t p_num_keys, int32_t p_key_idx = -1) {
			_keys = p_keys;
			_num_keys = p_num_keys;
			_key_idx = p_key_idx;
		}
		_FORCE_INLINE_ Iterator() {}
		_FORCE_INLINE_ Iterator(const Iterator &p_it) {
			_keys = p_it._keys;
			_num_keys = p_it._num_keys;
			_key_idx = p_it._key_idx;
		}
		_FORCE_INLINE_ void operator=(const Iterator &p_it) {
			_keys = p_it._keys;
			_num_keys = p_it._num_keys;
			_key_idx = p_it._key_idx;
		}

	private:
		const TKey *_keys = nullptr;
		uint32_t _num_keys = 0;
		int32_t _key_idx = -1;
	};

	_FORCE_INLINE_ Iterator begin() const {
		return _size ? Iterator(_keys, _size, 0) : Iterator();
	}
	_FORCE_INLINE_ Iterator end() const {
		return Iterator();
	}
	_FORCE_INLINE_ Iterator last() const {
		if (_size == 0) {
			return Iterator();
		}
		return Iterator(_keys, _size, _size - 1);
	}

	_FORCE_INLINE_ Iterator find(const TKey &p_key) const {
		uint32_t key_idx = 0;
		uint32_t hash_idx = 0;
		bool exists = _lookup_key_idx(p_key, key_idx, hash_idx);
		if (!exists) {
			return end();
		}
		return Iterator(_keys, _size, key_idx);
	}

	_FORCE_INLINE_ void remove(const Iterator &p_iter) {
		if (p_iter) {
			erase(*p_iter);
		}
	}

	/* Insert */

	Iterator insert(const TKey &p_key) {
		uint32_t key_idx = _insert(p_key);
		return Iterator(_keys, _size, key_idx);
	}

	/* Constructors */

	explicit HashSet(const HashSet &p_other) {
		_init_from(p_other);
	}

	HashSet(HashSet &&p_other) {
		_keys = p_other._keys;
		_hash_idx_to_key_idx = std::move(p_other._hash_idx_to_key_idx);
		_hashes = p_other._hashes;

		_capacity_mask = p_other._capacity_mask;
		_keys_capacity = p_other._keys_capacity;
		_size = p_other._size;
		_num_deleted = p_other._num_deleted;

		p_other._keys = nullptr;
		p_other._hashes.ptr = nullptr;
		p_other._capacity_mask = INITIAL_CAPACITY - 1;
		p_other._keys_capacity = 0;
		p_other._size = 0;
		p_other._num_deleted = 0;
	}

	void operator=(const HashSet &p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		reset();
		_init_from(p_other);
	}

	void operator=(HashSet &&p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}
		reset();
		_keys = p_other._keys;
		_hash_idx_to_key_idx = std::move(p_other._hash_idx_to_key_idx);
		_hashes = p_other._hashes;
		_capacity_mask = p_other._capacity_mask;
		_keys_capacity = p_other._keys_capacity;
		_size = p_other._size;
		_num_deleted = p_other._num_deleted;
		p_other._keys = nullptr;
		p_other._hashes.ptr = nullptr;
		p_other._capacity_mask = INITIAL_CAPACITY - 1;
		p_other._keys_capacity = 0;
		p_other._size = 0;
		p_other._num_deleted = 0;
	}

	bool operator==(const HashSet &p_other) const {
		if (_size != p_other._size) {
			return false;
		}
		for (uint32_t i = 0; i < _size; i++) {
			if (!p_other.has(_keys[i])) {
				return false;
			}
		}
		return true;
	}
	bool operator!=(const HashSet &p_other) const {
		return !(*this == p_other);
	}

	HashSet(uint32_t p_initial_capacity) {
		_capacity_mask = INITIAL_CAPACITY - 1;
		reserve(p_initial_capacity);
	}
	HashSet() {
		_capacity_mask = INITIAL_CAPACITY - 1;
	}
	HashSet(std::initializer_list<TKey> p_init) {
		reserve(p_init.size());
		for (const TKey &E : p_init) {
			insert(E);
		}
	}

	void reset() {
		clear();
		if (_keys != nullptr) {
			Memory::free_static(_keys);
			_keys = nullptr;
		}
		if (_hashes.ptr != nullptr) {
			Memory::free_static(_hashes.ptr);
			_hashes.ptr = nullptr;
		}
		_hash_idx_to_key_idx.reset();
		_capacity_mask = INITIAL_CAPACITY - 1;
		_keys_capacity = 0;
		_size = 0;
		_num_deleted = 0;
	}

	~HashSet() {
		reset();
	}
};
