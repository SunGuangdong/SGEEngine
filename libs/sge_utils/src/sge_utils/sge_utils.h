#pragma once

#include <cstddef>

#include "sge_utils/assert/assert.h"
#include "sge_utils/types.h"

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

namespace sge {
template <typename T, size_t N>
char (&SGE_TArrSize_Safe(T (&)[N]))[N];
#define SGE_ARRSZ(A) (sizeof(SGE_TArrSize_Safe(A)))
} // namespace sge

/// Expands the value of the specified expression or a macro and converts it to a string.
#define SGE_MACRO_STR_IMPL(m) #m
#define SGE_MACRO_STR(m) SGE_MACRO_STR_IMPL(m)

/// A macro aimed to be used around names of function arguments that aren't used in the function.
/// As we have unused variables to be treated as errors, we need a solution for the cases where
/// a function does not need an input argumnet. Just wrap the name of the unused variable with
/// this macro to solve the error.
#define UNUSED(x)
#define MAYBE_UNUSED [[maybe_unused]]

/// Concatenates two expressions together.
#define SGE_CAT_IMPL(s1, s2) s1##s2
#define SGE_CAT(s1, s2) SGE_CAT_IMPL(s1, s2)

/// Generates a unique identifier for the current file
/// prefixed with @x.
#define SGE_ANONYMOUS(x) SGE_CAT(x, __COUNTER__)
