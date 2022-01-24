#pragma once

#if defined(__GNUC__)
#include <signal.h>
#endif

#include <cstddef>

namespace sge {

/// A set of macros used to disable warnings (and therefore warnings as errors) in the
/// code surronded by the macros below. Useful when we want to include 3rd party code
/// that wasn't compiled with the same warnings as errors.
#ifdef WIN32
#define SGE_NO_WARN_BEGIN __pragma(warning(push, 0))
#define SGE_NO_WARN_END __pragma(warning(pop))
#else
#define SGE_NO_WARN_BEGIN
#define SGE_NO_WARN_END
#endif

typedef unsigned char ubyte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char sbyte;
typedef signed short sint16;
typedef signed int sint32;
typedef signed long long sint64;
typedef signed long long int64;
typedef signed long long i64;

template <typename T, size_t N>
char (&SGE_TArrSize_Safe(T (&)[N]))[N];
#define SGE_ARRSZ(A) (sizeof(SGE_TArrSize_Safe(A)))

} // namespace sge

#if defined(SGE_USE_DEBUG)
#if defined(__EMSCRIPTEN__)
#define SGE_BREAK() void(0) // does nothing, because emscripten disables asm.js vailation if we use __builtin_debugtrap()
#elif defined(_WIN32)
#define SGE_BREAK() __debugbreak()
#elif defined(__clang__)
#define SGE_BREAK() __builtin_debugtrap()
#else
#define SGE_BREAK() raise(SIGTRAP)
#endif
#else
#define SGE_BREAK() void(0)
#endif

int assertAskDisable(const char* const file, const int line, const char* expr);

#if defined(SGE_USE_DEBUG)
#define sgeAssert(__exp)                                                     \
	do {                                                                     \
		static int isBrakPointDisabled = 0;                                  \
		if (!isBrakPointDisabled && (false == bool(__exp))) {                \
			int responseCode = assertAskDisable(__FILE__, __LINE__, #__exp); \
			isBrakPointDisabled = !isBrakPointDisabled && responseCode != 0; \
			if (responseCode != 2) {                                         \
				SGE_BREAK();                                                 \
			}                                                                \
		}                                                                    \
	} while (false)
#ifdef __clang__
#define if_checked(expr) if ((expr))
#else
#define if_checked(expr) if ((expr) ? true : (SGE_BREAK(), false))
#endif
#else
#define sgeAssert(__exp) \
	{}
#define if_checked(expr) if (expr)
#endif

#define sgeAssertFalse(msg) sgeAssert(false && msg)

/// Expands the value of the specified expression or a macro and converts it to a string.
#define SGE_MACRO_STR_IMPL(m) #m
#define SGE_MACRO_STR(m) SGE_MACRO_STR_IMPL(m)

/// A macro aimed to be used around names of function arguments that aren't used in the function.
/// As we have unused variables to be treated as errors, we need a solution for the cases where
/// a function does not need an input argumnet. Just wrap the name of the unused variable with
/// this macro to solve the error.
#define UNUSED(x)
#define MAYBE_UNUSED [[maybe_unused]]

#define SGE_CAT_IMPL(s1, s2) s1##s2
#define SGE_CAT(s1, s2) SGE_CAT_IMPL(s1, s2)
#define SGE_ANONYMOUS(x) SGE_CAT(x, __COUNTER__)
