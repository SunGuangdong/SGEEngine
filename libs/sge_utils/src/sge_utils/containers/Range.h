#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

/// A stl-style "container" that is useful when we want to iterate in a range [a, b) (notice open-ended).
template <typename T>
struct Range {
	struct const_iterator {
		T i;

		const_iterator(T i)
		    : i(i) {}

		bool operator!=(const const_iterator& ref) const { return i != ref.i; }
		T operator*() const { return i; }
		const_iterator operator++() {
			++i;
			return *this;
		}
	};
public :
	Range() = delete;

	/// the 1st value is assumed 0. the interval is [0, _end)
	Range(const T m_end)
	    : m_begin(0)
	    , m_end(m_end) {}

	// [_begin, _end)
	Range(const T _begin, const T _end)
	    : m_begin(_begin)
	    , m_end(_end) {
		sgeAssert(m_begin <= m_end);
	}

	const_iterator begin() const { return m_begin; }
	const_iterator end() const { return m_end; }

	T m_begin;
	T m_end;
};
typedef Range<int> RangeInt;
typedef Range<unsigned int> RangeUint;
typedef Range<size_t> RangeSizet;

} // namespace sge
