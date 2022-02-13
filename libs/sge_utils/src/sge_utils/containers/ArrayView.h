#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

template <typename T>
struct ArrayView {
	typedef T value_type;
	typedef value_type* iterator;
	typedef const value_type* const_iterator;

  public:
	ArrayView() = default;

	ArrayView(value_type* const ptr, const size_t numElems)
	    : ptr(ptr)
	    , numElems(numElems)
	{
	}

	iterator begin() { return ptr; }
	iterator end() { return ptr + numElems; }

	const_iterator begin() const { return ptr; }
	const_iterator end() const { return ptr + numElems; }

	size_t size() const { return numElems; }
	bool empty() const { return numElems <= 0; }

	value_type& operator[](size_t index) { return ptr[index]; }

	const value_type& operator[](size_t index) const { return ptr[index]; }

  private:
	value_type* const ptr = nullptr;
	const size_t numElems = 0;
};

} // namespace sge
