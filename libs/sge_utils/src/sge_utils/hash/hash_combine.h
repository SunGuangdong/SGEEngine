#pragma once

namespace sge {

inline unsigned int hashCString_djb2(const char* str)
{
	if (!str) {
		return 0;
	}

	unsigned int hash = 5381;

	size_t iOffset = 0;
	while (str[iOffset] != '\0') {
		hash = ((hash << 5) + hash) + (str[iOffset]); /* hash * 33 + c */
		iOffset++;
	}

	return hash;
}

inline unsigned int hash_djb2(const char* const mem, const size_t numBytes)
{
	if (!mem) {
		return 0;
	}

	unsigned int hash = 5381;
	for (size_t iByte = 0; iByte < numBytes; ++iByte) {
		int c = int(mem[iByte]);
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

template <typename T>
inline T hash_combine(T seed, T value)
{
	return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

} // namespace sge
