#pragma once

#include "sge_utils/sge_utils.h"
#include <new>

namespace sge {

/// Static array is an fixed capacity class like std::vecotr.
/// It can grow its size up-to the specified @TSize.
/// The type class will be default constructed.
template <typename TType, int TSize>
class StaticArray {
  public:
	typedef TType value_type;
	typedef value_type* iterator;
	typedef const value_type* const_iterator;

	static const int SIZE = TSize;
	typedef TType ElementType;

	StaticArray()
	    : usedElems(0)
	{
	}

	~StaticArray() { resize(0); }

	StaticArray(const StaticArray& other)
	    : usedElems(0)
	{
		*this = other;
	}

	StaticArray& operator=(const StaticArray& other)
	{
		resize(0);
		for (const auto& v : other) {
			push_back(v);
		}

		return *this;
	}

	int size() const { return usedElems; }
	int static_size() const { return SIZE; }

	// Fills all alocated elements with a particlar value.
	void fill(int newSize, const TType& val)
	{
		resize(newSize);
		for (int t = 0; t < size(); ++t) {
			(*this)[t] = val;
		}
	}

	bool push_back(const ElementType& value)
	{
		if (usedElems >= SIZE)
			return false;
		elements[usedElems++] = value;
		return true;
	}

	bool pop_back()
	{
		if (!usedElems)
			return false;
		elements[--usedElems].~ElementType();
		return true;
	}

	ElementType& operator[](int idx) { return elements[idx]; }
	const ElementType& operator[](int idx) const { return elements[idx]; }

	const ElementType* data() const { return elements; }
	ElementType* data() { return elements; }

	bool isFull() { return SIZE == usedElems; }

	void clear() { resize(); }

	void resize(const int newSize)
	{
		sgeAssert(newSize >= 0);

		if (newSize > SIZE)
			return;

		for (int t = usedElems; t < newSize; ++t) {
			new (&elements[t]) ElementType();
		}

		for (int t = newSize; t < usedElems; ++t) {
			elements[t].~ElementType();
		}

		usedElems = newSize;
	}

	ElementType* begin() { return elements; }
	ElementType* end() { return elements + usedElems; }

	const ElementType* begin() const { return elements; }
	const ElementType* end() const { return elements + usedElems; }

	ElementType& back() { return *(elements + usedElems - 1); }
	const ElementType& back() const { return *(elements + usedElems - 1); }

  protected:
	int usedElems;
	ElementType elements[SIZE];
};

} // namespace sge
