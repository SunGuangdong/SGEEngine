#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

template <typename T, T invalidValue>
struct MiniOptional {

	MiniOptional() = default;

	void setInvalid() {
		value = invalidValue;
	}

	operator bool() const {
		return value != invalidValue;
	}

	bool hasValue() const {
		return value != invalidValue;
	}

	T& get() {
		sgeAssert(hasValue());
		return value;
	}

	const T& get() const {
		sgeAssert(hasValue());
		return value;
	}

  private:
	T value = invalidValue;
};

} // namespace sge
