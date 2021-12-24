#pragma once

#include <cstddef>

namespace sge {

template <std::size_t Len, std::size_t Align>
struct AlignedStorage {
	struct type {
		alignas(Align) unsigned char data[Len];
	};
};

template <typename T>
struct AlignedStorageTyped {
	union type {
		// Prevent calling the default constructor.
		// The owner of this object is responsible for constructing and destructing.
		type() {
		}

		~type() {
		}

		T dataTyped;
		alignas(T) unsigned char data[sizeof(T)];
	};
};

} // namespace sge
