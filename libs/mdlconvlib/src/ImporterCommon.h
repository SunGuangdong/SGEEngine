#pragma once

#include <stdexcept>

namespace sge {

enum : int { kMaxBonesPerVertex = 4 };

struct ImportExpect : public std::logic_error {
	ImportExpect(const char* const error = "Unknown ImportExpect")
	    : std::logic_error(error) {
	}
};

} // namespace sge
