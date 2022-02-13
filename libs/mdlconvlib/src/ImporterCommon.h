#pragma once

#include <stdexcept>

namespace sge {

enum : int { kMaxBonesPerVertex = 4 };

struct ImportExcept : public std::logic_error {
	ImportExcept(const char* const error = "Unknown ImportExcept")
	    : std::logic_error(error)
	{
	}
};

} // namespace sge
