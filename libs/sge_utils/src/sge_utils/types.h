#pragma once

#include <cstddef>

namespace sge {
	
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
	
/// Inheriting @NoCopyNoMove is an easy way to enforce a class to be non-copyable and non-movable.
struct NoCopyNoMove {
  public:
	NoCopyNoMove() = default;
	NoCopyNoMove(const NoCopyNoMove&) = delete;
	NoCopyNoMove& operator=(const NoCopyNoMove&) = delete;
};

/// Inheriting @NoCopy is an easy way to enforce a class to be no-copyable, but still moveable.
struct NoCopy {
  public:
	NoCopy() = default;
	
	NoCopy(const NoCopy&) = delete;
	NoCopy& operator=(const NoCopy&) = delete;

	inline NoCopy(NoCopy&&) = default;
	inline NoCopy& operator=(NoCopy&&) = default;
};

/// An overengineered thing that is never used.
/// Basically, identifies that the class is inteded to be an interface of some sort
/// that will have virtual destructor.
struct Polymorphic {
  public:
	Polymorphic() = default;
	virtual ~Polymorphic() = default;
};
	
} // namespace sge