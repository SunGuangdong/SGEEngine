#pragma once

#include <cstddef>

namespace sge {

/// An Aligned pice of memory of the specified type.
template <std::size_t Len, std::size_t Align>
struct AlignedStorage {
	struct type {
		alignas(Align) unsigned char data[Len];
	};
};

/// AlignedStorageTyped is just like @AlignedStorage,
/// however the raw data is in an union with the specified type,
/// this is done to be able to easily inspect the data in the debugger
/// as other wise it will be just a char array which isn't very helpful.
template <typename T>
struct AlignedStorageTyped {
	union type {
		// Prevent calling the default constructor.
		// The owner of this object is responsible for constructing and destructing.
		type() {}

		~type() {}

		T dataTyped;
		alignas(T) unsigned char data[sizeof(T)];
	};
};

} // namespace sge
