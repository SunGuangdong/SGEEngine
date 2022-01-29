#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Unknwn.h>

namespace sge {
template <typename TYPE>
class TComPtr {
  public:
	TComPtr()
	    : p(nullptr) {
	}

	TComPtr(TYPE* pPtr)
	    : p(pPtr) {
		AddRef();
	}

	TComPtr(const TComPtr& rOther)
	    : p(rOther.p) {
		AddRef();
	}

	TComPtr& operator=(const TComPtr& rOther) {
		p = rOther.p;
		AddRef();

		return (*this);
	}

	~TComPtr() {
		Release();
	}

	template <typename T>
	operator T() {
		return (T)p;
	}

	void Attach(TYPE* pPtr) {
		Release();
		p = pPtr;
	}

	template <typename U>
	HRESULT As(TComPtr<U>& other) const throw() {
		return p->QueryInterface(__uuidof(U), (void**)&other.p);
	}

	ULONG Release() {
		ULONG refcnt = 0;
		if (p) {
			refcnt = p->Release();
		}
		p = nullptr;
		return refcnt;
	}

	template <typename U>
	operator U*() {
		return static_cast<U*>(p);
	}

	template <typename U>
	operator const U*() const {
		return static_cast<const U*>(p);
	}

	operator bool() const {
		return p != nullptr;
	}

	operator TYPE*() const {
		return p;
	}
	TYPE** operator&() {
		return &p;
	}
	TYPE* const* operator&() const {
		return &p;
	}
	bool operator!() const {
		return (p == nullptr);
	}
	TComPtr& operator=(TYPE* pT) {
		p = pT;
		AddRef();
		return *this;
	}
	TYPE* operator->() {
		return p;
	}
	const TYPE* operator->() const {
		return p;
	}

  protected:
	void AddRef() {
		if (p)
			p->AddRef();
	}

  public:
	mutable TYPE* p;
};

} // namespace sge
