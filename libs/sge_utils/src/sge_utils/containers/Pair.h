#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

template <class TFirst, class TSecond>
struct Pair {
	Pair() {}

	Pair(const TFirst& a, const TSecond& b)
	    : first(a)
	    , second(b) {}

	TFirst first;
	TSecond second;
};

} // namespace sge
