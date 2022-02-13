#pragma once

namespace sge {

template <typename T>
struct span {
	typedef T ElementType;

	span() = default;

	span(T* const elements, const size_t usedElems)
	    : elements(elements)
	    , usedElems(usedElems)
	{
		if (usedElems > 0) {
			sgeAssert(elements != nullptr);
		}
	}


	ElementType& operator[](int idx) { return elements[idx]; }
	const ElementType& operator[](int idx) const { return elements[idx]; }

	ElementType& operator[](size_t idx) { return elements[idx]; }
	const ElementType& operator[](size_t idx) const { return elements[idx]; }


	bool empty() const { return elements == nullptr || usedElems == 0; }

	T* data() { return elements; }

	const T* data() const { return elements; }

	size_t size() const { return usedElems; }

	ElementType* begin() { return elements; }
	ElementType* end() { return elements + usedElems; }

	const ElementType* begin() const { return elements; }
	const ElementType* end() const { return elements + usedElems; }

	ElementType& back() { return *(elements + usedElems - 1); }
	const ElementType& back() const { return *(elements + usedElems - 1); }

  private:
	T* elements = nullptr;
	size_t usedElems = 0;
};

} // namespace sge
