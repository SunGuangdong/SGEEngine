#pragma once

#include "sge_utils/sge_utils.h"
#include <algorithm>
#include <vector>

namespace sge {

template <typename K, typename V, bool TSorted = true>
struct vector_map {
	// Const iterator implementation.
	struct const_iterator {
		const_iterator(const vector_map& map, size_t idx)
		    : map(map)
		    , idx(idx) {}

		const K& key() const { return map.keys[idx]; }
		const V& value() const { return map.values[idx]; }

		bool operator==(const const_iterator& ref) const { return (idx == ref.idx) && (&map == &ref.map); }
		bool operator!=(const const_iterator& ref) const { return !(*this == ref); }

		const_iterator& operator++() {
			++idx;
			return *this;
		}
		const_iterator& operator--() {
			--idx;
			return *this;
		}
		const_iterator& operator*() { return *this; } // In order to support for-raged loops.

		const vector_map& map;
		size_t idx;
	};

	// iterator implementation.
	struct iterator {
		iterator(vector_map& map, size_t idx)
		    : map(map)
		    , idx(idx) {}

		K& key() { return map.keys[idx]; }
		const K& key() const { return map.keys[idx]; }
		V& value() { return map.values[idx]; }
		const V& value() const { return map.values[idx]; }

		bool operator==(const iterator& ref) const { return (idx == ref.idx) && (&map == &ref.map); }
		bool operator!=(const iterator& ref) const { return !(*this == ref); }

		iterator& operator++() {
			++idx;
			return *this;
		}
		iterator& operator--() {
			--idx;
			return *this;
		}
		iterator& operator*() { return *this; } // In order to support for-raged loops.

		vector_map& map;
		size_t idx;
	};

	K& keyAtIdx(const size_t idx) { return keys[idx]; }
	const K& keyAtIdx(const size_t idx) const { return keys[idx]; }

	V& valueAtIdx(const size_t idx) { return values[idx]; }
	const V& valueAtIdx(const size_t idx) const { return values[idx]; }

	void debug_verify() const { sgeAssert(keys.size() == values.size()); }

	size_t size() const { return keys.size(); }

	void clear() {
		keys.clear();
		values.clear();
	}

	void reserve(const size_t elementsCount) {
		keys.reserve(elementsCount);
		values.reserve(elementsCount);

		debug_verify();
	}

	bool empty() const { return size() == 0; }

	V* find_element(const K& key) {
		debug_verify();

		auto itr = TSorted ? std::lower_bound(std::begin(keys), std::end(keys), key) : std::find(std::begin(keys), std::end(keys), key);

		bool found = TSorted ? itr != std::end(keys) && !(key < *itr) : itr != std::end(keys);

		if (!found) {
			return nullptr;
		}

		size_t const index = itr - std::begin(keys);
		return &values[index];
	}

	int find_element_index(const K& key) const {
		debug_verify();

		auto itr = TSorted ? std::lower_bound(std::begin(keys), std::end(keys), key) : std::find(std::begin(keys), std::end(keys), key);

		bool found = TSorted ? itr != std::end(keys) && !(key < *itr) : itr != std::end(keys);

		if (!found) {
			return -1;
		}

		size_t const index = itr - std::begin(keys);
		return (int)index;
	}

	const V* find_element(const K& key) const {
		debug_verify();

		if (TSorted) {
			auto itr = TSorted ? std::lower_bound(std::begin(keys), std::end(keys), key) : std::find(std::begin(keys), std::end(keys), key);

			bool found = TSorted ? itr != std::end(keys) && !(key < *itr) : itr != std::end(keys);

			if (!found) {
				return nullptr;
			}

			size_t const index = itr - std::begin(keys);
			return &values[index];
		} else {
			for (int t = 0; t < keys.size(); ++t) {
				if (keys[t] == key) {
					return &values[t];
				}
			}

			return nullptr;
		}
	}

	V& operator[](const K& key) {
		debug_verify();

		if (TSorted) {
			auto itr = std::lower_bound(std::begin(keys), std::end(keys), key);
			const size_t idx = itr - std::begin(keys);

			// Check if the element already exists.
			if (itr != keys.end() && *itr == key) {
				return values[idx];
			}

			// The element doesn't exist.
			keys.insert(itr, key);
			values.insert(values.begin() + idx, V());

			debug_verify();

			return values[idx];
		} else {
			V* const ptr = find_element(key);
			if (ptr)
				return *ptr;

			keys.emplace_back(key);
			values.emplace_back(V());

			sgeAssert(keys.size() == values.size());

			debug_verify();

			return values.back();
		}
	}

	void eraseAtIndex(const size_t index) {
		sgeAssert(size() > index);
		keys.erase(keys.begin() + index);
		values.erase(values.begin() + index);

		debug_verify();
	}

	void eraseKey(const K& key) {
		auto itr = TSorted ? std::lower_bound(std::begin(keys), std::end(keys), key) : std::find(std::begin(keys), std::end(keys), key);

		bool found = TSorted ? itr != std::end(keys) && !(key < *itr) : itr != std::end(keys);

		if (!found) {
			return;
		}

		size_t const index = itr - std::begin(keys);
		eraseAtIndex(index);
	}

	iterator begin() { return iterator(*this, 0); }
	const_iterator begin() const { return const_iterator(*this, 0); }
	const_iterator cbegin() const { return const_iterator(*this, 0); }

	iterator end() { return iterator(*this, size()); }
	const_iterator end() const { return const_iterator(*this, size()); }
	const_iterator cend() const { return const_iterator(*this, size()); }

	const std::vector<K>& getAllKeys() const { return keys; }
	const std::vector<V>& getAllValues() const { return values; }

	/// A set of function used to help with serialization of vector_maps.
	void serializationSetKeys(const std::vector<K>& k) { keys = k; }
	void serializationSetValues(const std::vector<V>& v) {
		values = v;
		debug_verify();
	}

  public:
	std::vector<K> keys;
	std::vector<V> values;
};

} // namespace sge
