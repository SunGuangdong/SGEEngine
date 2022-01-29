#pragma once

#include "sge_utils/sge_utils.h"
#include <algorithm>
#include <vector>

namespace sge {

template <typename K>
struct vector_set {
	typedef typename std::vector<K>::iterator iterator;
	typedef typename std::vector<K>::const_iterator const_iterator;

	iterator begin() { return m_data.begin(); }
	const_iterator begin() const { return m_data.begin(); }
	const_iterator cbegin() const { return m_data.cbegin(); }

	iterator end() { return m_data.end(); }
	const_iterator end() const { return m_data.end(); }
	const_iterator cend() const { return m_data.cend(); }

	K* data() { return m_data.data(); }
	const K* data() const { return m_data.data(); }
	int size() const { return (int)m_data.size(); }
	bool empty() const { return size() == 0; }
	void clear() { m_data.clear(); }

	void reserve(size_t n) { m_data.reserve(n); }

	int find_index_of(const K& key) const {
		auto itr = std::lower_bound(std::begin(m_data), std::end(m_data), key);

		const bool found = itr != std::end(m_data) && !(key < *itr);
		if (!found) {
			return -1;
		}

		size_t const index = itr - std::begin(m_data);
		return (int)index;
	}

	int count(const K& key) const { return find_index_of(key) == -1 ? 0 : 1; }

	void add(const K& key) {
		auto itr = std::lower_bound(std::begin(m_data), std::end(m_data), key);

		// Check if the element already exists.
		if (itr != m_data.end() && *itr == key) {
			return;
		}

		// The element doesn't exist.
		m_data.insert(itr, key);
	}

	void insert(const K& key) { add(key); }

	void eraseKey(const K& key) {
		auto itr = std::lower_bound(std::begin(m_data), std::end(m_data), key);

		if ((itr != std::end(m_data)) && (key == *itr)) {
			m_data.erase(itr);
		}
	}

	const std::vector<K>& getRawVector() const { return m_data; }

	const K& getNth(const size_t n) const { return m_data[size_t(n)]; }

  private:
	std::vector<K> m_data;
};

} // namespace sge
