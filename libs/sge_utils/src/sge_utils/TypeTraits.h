#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

//------------------------------------------------------------------
// template_if for types
//------------------------------------------------------------------
template <bool boolean, class A, class B>
struct TemplateIf;

template <class A, class B>
struct TemplateIf<true, A, B> {
	typedef A value;
};

template <class A, class B>
struct TemplateIf<false, A, B> {
	typedef B value;
};

//------------------------------------------------------------------
// template if for variables
//------------------------------------------------------------------
template <class T, bool boolean, int A, int B>
struct TemplateIf_values;

template <class T, int A, int B>
struct TemplateIf_values<T, true, A, B> {
	enum : T { value = A };
};

template <class T, int A, int B>
struct TemplateIf_values<T, false, A, B> {
	enum : T { value = B };
};

//------------------------------------------------------------------
// LargestNumber : searches for the largest number at compile time.
//------------------------------------------------------------------
#if 0 // Works on MSVC but not on GCC
template <class T, T... nums>
struct LargestNumber;

template <class T, int num>
struct LargestNumber<T, num>
{
	enum : T 
	{
		value = num
	};
};

template <class T, T a, T b, T... nums>
struct LargestNumber<T, a, b, nums...>
{
	enum : T
	{
		internal_temp = TemplateIf_values<T, (a > b), a, b>::value,
		value = LargestNumber<T, internal_temp, nums...>::value
	};
};
#endif
//------------------------------------------------------------------
// LargestType : searches for the largest type at compile time.
//------------------------------------------------------------------
template <typename... Ts>
struct LargestType;

template <typename T>
struct LargestType<T> {
	using value = T;
};

template <typename T, typename U, typename... Ts>
struct LargestType<T, U, Ts...> {
	using value = typename LargestType<typename TemplateIf<(sizeof(U) <= sizeof(T)), T, U>::value, Ts...>::value;
};

template <class From, class To>
class TCanConvertPtr {
	static ubyte test(...);
	static uint16 test(To const*);

  public:
	enum Type { result = sizeof(test((From*)0)) - 1 };
};

template <class T>
struct RemoveReference {
	typedef T type;
};
template <class T>
struct RemoveReference<T&> {
	typedef T type;
};
template <class T>
struct RemoveReference<T&&> {
	typedef T type;
};

/// A function used to provide an id for earch type that changes with every recompilation of the program.
typedef void (*PerBuildTypeId)();
template <typename T>
struct GetPerBuildTypeId_t {
	static void value() {}
};

#define sgePerBuildTypeId(T) (&GetPerBuildTypeId_t<T>::value)

template <typename T>
struct is_compareable {
	struct yes {
		char a[1];
	};
	struct no {
		char a[2];
	};

	template <typename C>
	static yes test_eq(decltype(&C::operator==));
	template <typename C>
	static no test_eq(...);

	template <typename C>
	static yes test_neq(decltype(&C::operator!=));
	template <typename C>
	static no test_neq(...);

	enum { value = (sizeof(test_eq<T>(0)) == sizeof(yes) && sizeof(test_neq<T>(0)) == sizeof(yes)) };
};

} // namespace sge
