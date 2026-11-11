/**************************************************************************/
/*  a_hash_map.h                                                          */
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

#include "core/math/math_funcs_binary.h"
#include "core/os/memory.h"
#include "core/string/print_string.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/hashes.h"
#include "core/templates/index_array.h"
#include "core/templates/pair.h"

#include <initializer_list>

class String;
class StringName;
class Variant;

/**
 * An array-based implementation of a hash map. It is very efficient in terms of performance and
 * memory usage. Works like a dynamic array, adding elements to the end of the array, and
 * allows you to access array elements by their index by using `get_by_index` method.
 * Example:
 * ```
 *  AHashMap<int, Object *> map;
 *
 *  int get_object_id_by_number(int p_number) {
 *		int id = map.get_index(p_number);
 *		return id;
 *  }
 *
 *  Object *get_object_by_id(int p_id) {
 *		map.get_by_index(p_id).value;
 *  }
 * ```
 * Still, don`t erase the elements because ID can break.
 *
 * When an element erase, its place is taken by the element from the end.
 *
 *        <-------------
 *      |               |
 *  6 8 X 9 32 -1 5 -10 7 X X X
 *  6 8 7 9 32 -1 5 -10 X X X X
 *
 *
 * Use RBMap if you need to iterate over sorted elements.
 *
 * Use HashMap if:
 *   - You need to keep an iterator or const pointer to Key and you intend to add/remove elements in the meantime.
 *   - You need to preserve the insertion order when using erase.
 *
 * It is recommended to use `HashMap` if `KeyValue` size is very large.
 */
template <typename TKey, typename TValue,
		typename Hasher = HashMapHasherDefault,
		typename Comparator = HashMapComparatorDefault<TKey>>
class AHashMap {
public:
	// Must be a power of two.
	static constexpr uint32_t INITIAL_CAPACITY = 16;
	static constexpr uint32_t EMPTY_HASH = 0;

private:
	typedef KeyValue<TKey, TValue> MapKeyValue;
	MapKeyValue *_elements = nullptr;
	Hashes _hashes;
	IndexArray _hash_idx_to_element_idx;

	uint32_t _capacity_mask = 0;
	uint32_t _elements_capacity = 0;
	uint32_t _size = 0;
	uint32_t _num_deleted = 0;

	uint32_t _hash(const TKey &p_key) const {
		return Hasher::hash(p_key);
	}

	static _FORCE_INLINE_ uint32_t _get_resize_count(uint32_t p_capacity_mask) {
		return p_capacity_mask - ((p_capacity_mask + 1) >> 4);
	}

public:
	_FORCE_INLINE_ bool _compare_function(uint32_t p_hash_idx, const TKey &p_key) const {
		return Comparator::compare(_elements[_hash_idx_to_element_idx.get_index(p_hash_idx)].key, p_key);
	}
private:

	bool _lookup_idx_with_hash(const TKey &p_key, uint32_t &r_element_idx, uint32_t &r_hash_idx, uint32_t p_hash) const {
		if (_hashes.ptr == nullptr) {
			return false;
		}
		if (!_hashes.lookup_pos_with_hash<AHashMap, TKey>(this, p_key, p_hash, _capacity_mask, r_hash_idx)) {
			return false;
		}
		r_element_idx = _hash_idx_to_element_idx.get_index(r_hash_idx);
		return true;
	}

	bool _lookup_idx(const TKey &p_key, uint32_t &r_element_idx, uint32_t &r_hash_idx) const {
		return _lookup_idx_with_hash(p_key, r_element_idx, r_hash_idx, _hash(p_key));
	}

	uint32_t _insert_metadata(uint32_t p_hash, uint32_t p_element_idx) {
		bool reused_tombstone = false;
		uint32_t hash_idx = _hashes.insert_hash(p_hash, _capacity_mask, reused_tombstone);
		_hash_idx_to_element_idx.set_index(hash_idx, p_element_idx);
		if (reused_tombstone) {
			_num_deleted--;
		}
		return hash_idx;
	}

	void _resize_and_rehash(uint32_t p_new_capacity) {
		uint32_t new_capacity = Math::next_power_of_2(MAX(INITIAL_CAPACITY, p_new_capacity));
		uint32_t new_mask = new_capacity - 1;
		while (_get_resize_count(new_mask) + 1 < _size) {
			new_capacity *= 2;
			new_mask = new_capacity - 1;
		}

		uint8_t *old_hashes = _hashes.ptr;
		_capacity_mask = new_mask;
		_hashes.ptr = reinterpret_cast<uint8_t *>(Memory::alloc_static_zeroed(new_capacity + HashGroup::GROUP_SIZE));
		memset(_hashes.ptr + new_capacity, Hashes::END_HASH, HashGroup::GROUP_SIZE);
		_hash_idx_to_element_idx.initialize(new_capacity, _get_resize_count(_capacity_mask) + 1);
		_num_deleted = 0;

		if (old_hashes != nullptr) {
			for (uint32_t i = 0; i < _size; i++) {
				_insert_metadata(_hash(_elements[i].key), i);
			}
			Memory::free_static(old_hashes);
		}
	}

	void _ensure_element_capacity(uint32_t p_capacity) {
		if (_elements != nullptr && _elements_capacity >= p_capacity) {
			return;
		}
		uint32_t new_capacity = MAX(2u, _elements_capacity);
		while (new_capacity < p_capacity) {
			new_capacity += MAX(1u, new_capacity / 2);
		}
		_elements = reinterpret_cast<MapKeyValue *>(Memory::realloc_static(_elements, sizeof(MapKeyValue) * new_capacity));
		_elements_capacity = new_capacity;
	}

	int32_t _insert_element(const TKey &p_key, const TValue &p_value, uint32_t p_hash) {
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
		_ensure_element_capacity(_size + 1);
		memnew_placement(&_elements[_size], MapKeyValue(p_key, p_value));
		_insert_metadata(p_hash, _size);
		return _size++;
	}

	void _init_from(const AHashMap &p_other) {
		_capacity_mask = p_other._capacity_mask;
		_size = p_other._size;
		_num_deleted = p_other._num_deleted;
		_elements_capacity = p_other._elements_capacity;
		if (_size == 0) {
			return;
		}
		_elements = reinterpret_cast<MapKeyValue *>(Memory::alloc_static(sizeof(MapKeyValue) * _elements_capacity));
		if constexpr (std::is_trivially_copyable_v<MapKeyValue>) {
			memcpy(_elements, p_other._elements, sizeof(MapKeyValue) * _size);
		} else {
			for (uint32_t i = 0; i < _size; i++) {
				memnew_placement(&_elements[i], MapKeyValue(p_other._elements[i]));
			}
		}
		uint32_t capacity = _capacity_mask + 1;
		_hashes.ptr = reinterpret_cast<uint8_t *>(Memory::alloc_static(sizeof(uint8_t) * (capacity + HashGroup::GROUP_SIZE)));
		memcpy(_hashes.ptr, p_other._hashes.ptr, capacity + HashGroup::GROUP_SIZE);
		_hash_idx_to_element_idx = p_other._hash_idx_to_element_idx;
	}

public:
	/* Standard Godot Container API */

	_FORCE_INLINE_ uint32_t get_capacity() const { return _capacity_mask + 1; }
	_FORCE_INLINE_ uint32_t size() const { return _size; }

	_FORCE_INLINE_ bool is_empty() const {
		return _size == 0;
	}

	void clear() {
		if (_hashes.ptr == nullptr) {
			return;
		}

		memset(_hashes.ptr, Hashes::EMPTY_HASH, _capacity_mask + 1);
		memset(_hashes.ptr + _capacity_mask + 1, Hashes::END_HASH, HashGroup::GROUP_SIZE);
		if constexpr (!(std::is_trivially_destructible_v<TKey> && std::is_trivially_destructible_v<TValue>)) {
			for (uint32_t i = 0; i < _size; i++) {
				_elements[i].key.~TKey();
				_elements[i].value.~TValue();
			}
		}

		_size = 0;
		_num_deleted = 0;
	}

	TValue &get(const TKey &p_key) {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND_MSG(!exists, "AHashMap key not found.");
		return _elements[element_idx].value;
	}

	const TValue &get(const TKey &p_key) const {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND_MSG(!exists, "AHashMap key not found.");
		return _elements[element_idx].value;
	}

	const TValue *getptr(const TKey &p_key) const {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (exists) {
			return &_elements[element_idx].value;
		}
		return nullptr;
	}

	TValue *getptr(const TKey &p_key) {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (exists) {
			return &_elements[element_idx].value;
		}
		return nullptr;
	}

	bool has(const TKey &p_key) const {
		uint32_t _idx = 0;
		uint32_t meta_idx = 0;
		return _lookup_idx(p_key, _idx, meta_idx);
	}

	bool erase(const TKey &p_key) {
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (!exists) {
			return false;
		}

		uint32_t moved_hash_idx = 0;
		if (element_idx + 1 < _size) {
			uint32_t moved_element_idx = 0;
			CRASH_COND_MSG(!_lookup_idx(_elements[_size - 1].key, moved_element_idx, moved_hash_idx) || moved_element_idx != _size - 1, "AHashMap element index is inconsistent.");
		}

		_hashes.delete_hash(meta_idx);
		_num_deleted++;
		_elements[element_idx].key.~TKey();
		_elements[element_idx].value.~TValue();
		_size--;

		if (element_idx < _size) {
			memnew_placement(&_elements[element_idx], MapKeyValue(std::move(_elements[_size])));
			_hash_idx_to_element_idx.set_index(moved_hash_idx, element_idx);
			_elements[_size].~MapKeyValue();
		}

		return true;
	}

	// Replace the key of an entry in-place, without invalidating iterators or changing the entries position during iteration.
	// p_old_key must exist in the map and p_new_key must not, unless it is equal to p_old_key.
	bool replace_key(const TKey &p_old_key, const TKey &p_new_key) {
		if (p_old_key == p_new_key) {
			return true;
		}
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		ERR_FAIL_COND_V(_lookup_idx(p_new_key, element_idx, meta_idx), false);
		ERR_FAIL_COND_V(!_lookup_idx(p_old_key, element_idx, meta_idx), false);
		MapKeyValue &element = _elements[element_idx];
		const_cast<TKey &>(element.key) = p_new_key;

		_hashes.delete_hash(meta_idx);
		_num_deleted++;

		uint32_t hash = _hash(p_new_key);
		_insert_metadata(hash, element_idx);
		if (_size + _num_deleted > _capacity_mask) {
			_resize_and_rehash(_capacity_mask + 1);
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
		_ensure_element_capacity(p_new_capacity);
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

	struct ConstIterator {
		_FORCE_INLINE_ const MapKeyValue &operator*() const {
			return *pair;
		}
		_FORCE_INLINE_ const MapKeyValue *operator->() const {
			return pair;
		}
		_FORCE_INLINE_ ConstIterator &operator++() {
			pair++;
			return *this;
		}

		_FORCE_INLINE_ ConstIterator &operator--() {
			pair--;
			if (pair < begin) {
				pair = end;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator &b) const { return pair == b.pair; }
		_FORCE_INLINE_ bool operator!=(const ConstIterator &b) const { return pair != b.pair; }

		_FORCE_INLINE_ explicit operator bool() const {
			return pair != end;
		}

		_FORCE_INLINE_ ConstIterator(MapKeyValue *p_key, MapKeyValue *p_begin, MapKeyValue *p_end) {
			pair = p_key;
			begin = p_begin;
			end = p_end;
		}
		_FORCE_INLINE_ ConstIterator() {}
		_FORCE_INLINE_ ConstIterator(const ConstIterator &p_it) {
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}
		_FORCE_INLINE_ void operator=(const ConstIterator &p_it) {
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

	private:
		MapKeyValue *pair = nullptr;
		MapKeyValue *begin = nullptr;
		MapKeyValue *end = nullptr;
	};

	struct Iterator {
		_FORCE_INLINE_ MapKeyValue &operator*() const {
			return *pair;
		}
		_FORCE_INLINE_ MapKeyValue *operator->() const {
			return pair;
		}
		_FORCE_INLINE_ Iterator &operator++() {
			pair++;
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			pair--;
			if (pair < begin) {
				pair = end;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &b) const { return pair == b.pair; }
		_FORCE_INLINE_ bool operator!=(const Iterator &b) const { return pair != b.pair; }

		_FORCE_INLINE_ explicit operator bool() const {
			return pair != end;
		}

		_FORCE_INLINE_ Iterator(MapKeyValue *p_key, MapKeyValue *p_begin, MapKeyValue *p_end) {
			pair = p_key;
			begin = p_begin;
			end = p_end;
		}
		_FORCE_INLINE_ Iterator() {}
		_FORCE_INLINE_ Iterator(const Iterator &p_it) {
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}
		_FORCE_INLINE_ void operator=(const Iterator &p_it) {
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

		operator ConstIterator() const {
			return ConstIterator(pair, begin, end);
		}

	private:
		MapKeyValue *pair = nullptr;
		MapKeyValue *begin = nullptr;
		MapKeyValue *end = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() {
		return Iterator(_elements, _elements, _elements == nullptr ? nullptr : _elements + _size);
	}
	_FORCE_INLINE_ Iterator end() {
		return Iterator(_elements == nullptr ? nullptr : _elements + _size, _elements, _elements == nullptr ? nullptr : _elements + _size);
	}
	_FORCE_INLINE_ Iterator last() {
		if (unlikely(_size == 0)) {
			return Iterator(nullptr, nullptr, nullptr);
		}
		return Iterator(_elements + _size - 1, _elements, _elements + _size);
	}

	Iterator find(const TKey &p_key) {
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return end();
		}
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	void remove(const Iterator &p_iter) {
		if (p_iter) {
			erase(p_iter->key);
		}
	}

	_FORCE_INLINE_ ConstIterator begin() const {
		return ConstIterator(_elements, _elements, _elements == nullptr ? nullptr : _elements + _size);
	}
	_FORCE_INLINE_ ConstIterator end() const {
		return ConstIterator(_elements == nullptr ? nullptr : _elements + _size, _elements, _elements == nullptr ? nullptr : _elements + _size);
	}
	_FORCE_INLINE_ ConstIterator last() const {
		if (unlikely(_size == 0)) {
			return ConstIterator(nullptr, nullptr, nullptr);
		}
		return ConstIterator(_elements + _size - 1, _elements, _elements + _size);
	}

	ConstIterator find(const TKey &p_key) const {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return end();
		}
		return ConstIterator(_elements + element_idx, _elements, _elements + _size);
	}

	/* Indexing */

	const TValue &operator[](const TKey &p_key) const {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND(!exists);
		return _elements[element_idx].value;
	}

	TValue &operator[](const TKey &p_key) {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		uint32_t hash = _hash(p_key);
		bool exists = _lookup_idx_with_hash(p_key, element_idx, meta_idx, hash);

		if (exists) {
			return _elements[element_idx].value;
		} else {
			element_idx = _insert_element(p_key, TValue(), hash);
			return _elements[element_idx].value;
		}
	}

	/* Insert */

	Iterator insert(const TKey &p_key, const TValue &p_value) {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		uint32_t hash = _hash(p_key);
		bool exists = _lookup_idx_with_hash(p_key, element_idx, meta_idx, hash);

		if (!exists) {
			element_idx = _insert_element(p_key, p_value, hash);
		} else {
			_elements[element_idx].value = p_value;
		}
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	// Inserts an element without checking if it already exists.
	Iterator insert_new(const TKey &p_key, const TValue &p_value) {
		DEV_ASSERT(!has(p_key));
		uint32_t hash = _hash(p_key);
		uint32_t element_idx = _insert_element(p_key, p_value, hash);
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	/* Array methods. */

	// Unsafe. Changing keys and going outside the bounds of an array can lead to undefined behavior.
	KeyValue<TKey, TValue> *get_elements_ptr() {
		return _elements;
	}

	// Returns the element index. If not found, returns -1.
	int get_index(const TKey &p_key) {
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return -1;
		}
		return element_idx;
	}

	KeyValue<TKey, TValue> &get_by_index(uint32_t p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, _size);
		return _elements[p_index];
	}

	bool erase_by_index(uint32_t p_index) {
		if (p_index >= size()) {
			return false;
		}
		return erase(_elements[p_index].key);
	}

	/* Constructors */

	AHashMap(AHashMap &&p_other) {
		_elements = p_other._elements;
		_hashes = p_other._hashes;
		_hash_idx_to_element_idx = std::move(p_other._hash_idx_to_element_idx);
		_capacity_mask = p_other._capacity_mask;
		_elements_capacity = p_other._elements_capacity;
		_size = p_other._size;
		_num_deleted = p_other._num_deleted;

		p_other._elements = nullptr;
		p_other._hashes.ptr = nullptr;
		p_other._capacity_mask = INITIAL_CAPACITY - 1;
		p_other._elements_capacity = 0;
		p_other._size = 0;
		p_other._num_deleted = 0;
	}

	explicit AHashMap(const AHashMap &p_other) {
		_init_from(p_other);
	}

	void operator=(const AHashMap &p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		reset();

		_init_from(p_other);
	}

	AHashMap(uint32_t p_initial_capacity) {
		_capacity_mask = INITIAL_CAPACITY - 1;
		reserve(p_initial_capacity);
	}
	AHashMap() :
			_capacity_mask(INITIAL_CAPACITY - 1) {
	}

	AHashMap(std::initializer_list<KeyValue<TKey, TValue>> p_init) {
		reserve(p_init.size());
		for (const KeyValue<TKey, TValue> &E : p_init) {
			insert(E.key, E.value);
		}
	}

	void reset() {
		if (_elements != nullptr) {
			if constexpr (!(std::is_trivially_destructible_v<TKey> && std::is_trivially_destructible_v<TValue>)) {
				for (uint32_t i = 0; i < _size; i++) {
					_elements[i].key.~TKey();
					_elements[i].value.~TValue();
				}
			}
			Memory::free_static(_elements);
			_elements = nullptr;
		}
		if (_hashes.ptr != nullptr) {
			Memory::free_static(_hashes.ptr);
			_hashes.ptr = nullptr;
		}
		_hash_idx_to_element_idx.reset();
		_capacity_mask = INITIAL_CAPACITY - 1;
		_elements_capacity = 0;
		_size = 0;
		_num_deleted = 0;
	}

	~AHashMap() {
		reset();
	}
};

extern template class AHashMap<int, int>;
extern template class AHashMap<String, int>;
extern template class AHashMap<StringName, StringName>;
extern template class AHashMap<StringName, Variant>;
extern template class AHashMap<StringName, int>;
