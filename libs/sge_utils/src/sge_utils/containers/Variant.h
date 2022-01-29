#pragma once

#include "sge_utils/TypeTraits.h"

namespace sge {

//-------------------------------------------------------------------------------------------------------------------
// Variant is an extented union caplable of holding non-trivial types.
// Note: that the idea behind moving "Variant" objects, is actually moving
// the "Variant" itself, and NOT MOVING THE OBJECT(despite the fact that move constr/assing could get called).
//-------------------------------------------------------------------------------------------------------------------
template <int Nbytes>
struct Variant {
	Variant() = default;

	~Variant() { Destroy(); }

	Variant(const Variant& ref) {
		if (ref.objectValid) {
			objectValid = true;
			copyTypeHolder(ref);

			GetTypeHolder()->copy_constr(object, ref.object);
		}
	}

	Variant(Variant&& ref) {
		if (ref.objectValid && ref.GetTypeHolder()->getCompileTimeClassId()) {
			objectValid = true;
			copyTypeHolder(ref);

			GetTypeHolder()->move_constr(object, ref.object);

			ref.objectValid = 0;
		}
	}

	Variant& operator=(const Variant& ref) {
		if (ref.objectValid == false) {
			Destroy();
			return *this;
		}

		if (objectValid && GetTypeHolder()->getCompileTimeClassId() == ref.GetTypeHolder()->getCompileTimeClassId()) {
			GetTypeHolder()->assign(object, ref.object);
			return *this;
		} else {
			Destroy();

			objectValid = true;
			copyTypeHolder(ref);

			GetTypeHolder()->constr(&object[0]);
			GetTypeHolder()->assign(object, ref.object);

			return *this;
		}

		sgeAssert(false);
		return *this;
	}

	Variant& operator=(Variant&& ref) {
		if (ref.objectValid == false) {
			Destroy();
			return *this;
		}

		if (objectValid && GetTypeHolder()->getCompileTimeClassId() && ref.GetTypeHolder()->getCompileTimeClassId()) {
			GetTypeHolder()->move_assign(object, ref.object);
			ref.objectValid = false;
			return *this;
		} else {
			Destroy();

			objectValid = true;
			copyTypeHolder(ref);

			GetTypeHolder()->constr(&object[0]);
			GetTypeHolder()->move_assign(object, ref.object);

			ref.objectValid = false;

			return *this;
		}
	}

	void Destroy() {
		if (objectValid != 0) {
			GetTypeHolder()->destruct(object);
			objectValid = false;
		}
	}

	template <typename T>
	T* setVariant() {
		static_assert(sizeof(T) <= Nbytes, "The preallocated buffer isn't large enough");

		// Destroy the current object (if any).
		Destroy();

		// Create a new typeholder and create the object.
		new (typeHolder) TypeHolder<T>();
		GetTypeHolder()->constr(&object[0]);
		objectValid = true;

		return (T*)&object[0];
	}

	template <typename T>
	bool resetVariantToValue(const T& newValue) {
		if (setVariant<T>()) {
			*get<T>() = newValue;
			return true;
		}

		return false;
	}

	template <typename T>
	T* get() {
		if (isVariantSetTo<T>())
			return reinterpret_cast<T*>(&object[0]);
		return nullptr;
	}

	template <typename T>
	const T* get() const {
		if (isVariantSetTo<T>())
			return reinterpret_cast<const T*>(&object[0]);
		return nullptr;
	}

	template <typename T>
	T& as() {
		return *reinterpret_cast<T*>(&object[0]);
	}

	// Checks if there is inintialized object.
	bool hasVariant() const { return objectValid != 0; }

	// Check if the currently created object is type "T".
	template <typename T>
	bool isVariantSetTo() const {
		return objectValid && (sgePerBuildTypeId(T) == GetTypeHolder()->getCompileTimeClassId());
	}

  private:
	// TypeInterface is used do provide information about the working type <T>.
	struct TypeInterface {
		virtual void constr(void*) const = 0; // This function know
		virtual void copy_constr(void* p, const void* ref) const = 0;
		virtual void move_constr(void* p, void* ref) const = 0;
		virtual void assign(void* p, const void* ref) const = 0;
		virtual void move_assign(void* p, const void* ref) const = 0;
		virtual void destruct(void*) const = 0;
		virtual void* getCompileTimeClassId() const = 0;
	};

	template <typename T>
	struct TypeHolder : TypeInterface {
		void constr(void* p) const final { new (p) T; }

		void copy_constr(void* p, const void* ref) const final { new (p) T(*(const T*)ref); }

		void move_constr(void* p, void* ref) const final { new (p) T(std::move(*(T*)ref)); }

		void assign(void* p, const void* ref) const final { *(T*)p = *(const T*)ref; }

		void move_assign(void* p, const void* ref) const final { *(T*)p = std::move(*(const T*)ref); }

		void destruct(void* p) const final { ((T*)(p))->~T(); }

		void* getCompileTimeClassId() const final { return (void*)(sgePerBuildTypeId(T)); }
	};

	void copyTypeHolder(const Variant& ref) {
		for (int iByte = 0; iByte < SGE_ARRSZ(ref.typeHolder); ++iByte) {
			typeHolder[iByte] = ref.typeHolder[iByte];
		}
	}

	struct Dummy {};

	TypeInterface* GetTypeHolder() { return (TypeInterface*)typeHolder; }
	const TypeInterface* GetTypeHolder() const { return (const TypeInterface*)typeHolder; }

	// Class data
	char object[Nbytes];
	char typeHolder[sizeof(TypeHolder<Dummy>)];
	unsigned char objectValid = 0;
};

//------------------------------------------------------------------
// A helper alias of Variant that automatically computes the size of
// the storage by a list of types.
// [CAUTION][NOTE] This class does not restrict the Variant to those types!
//------------------------------------------------------------------
template <typename... Ts>
using VariantT = Variant<sizeof(LargestType<Ts...>::value)>;

} // namespace sge
