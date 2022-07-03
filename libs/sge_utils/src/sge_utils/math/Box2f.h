#pragma once

#include "common.h"
#include "math_base.h"
#include "vec2f.h"
#include <float.h>

#if defined(min) || defined(max)
	#error min/max macros are defined
#endif

namespace sge {
struct Box2f {
	Box2f() { setEmpty(); }

	Box2f(const vec2f& min, const vec2f& max)
	    : min(min)
	    , max(max)
	{
	}

	static Box2f getFromHalfDiagonal(const vec2f& halfDiagonal, const vec2f& offset = vec2f(0.f))
	{
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0);

		Box2f retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	static Box2f getFromHalfDiagonalWithOffset(const vec2f& halfDiagonal, const vec2f& offset)
	{
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0);

		Box2f retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	void move(const vec2f& offset)
	{
		min += offset;
		max += offset;
	}

	vec2f center() const { return (max + min) * 0.5f; }
	vec2f halfDiagonal() const { return (max - min) * 0.5f; }
	vec2f diagonal() const { return (max - min); }

	vec2f bottomMiddle() const
	{
		vec2f ret = vec2f((max.x - min.x) * 0.5f, min.y);
		return ret;
	}

	vec2f topMiddle() const
	{
		vec2f ret = vec2f((max.x - min.x) * 0.5f, max.y);
		return ret;
	}

	vec2f size() const { return max - min; }

	void setEmpty()
	{
		min = vec2f(FLT_MAX);
		max = vec2f(-FLT_MAX);
	}

	bool isEmpty() const { return min == vec2f(FLT_MAX) && max == vec2f(-FLT_MAX); }

	void expand(const vec2f& point)
	{
		min = min.pickMin(point);
		max = max.pickMax(point);
	}

	void expandEdgesByCoefficient(float value)
	{
		vec2f exp = halfDiagonal() * value;
		min -= exp;
		max += exp;
	}

	void scale(const float s)
	{
		min *= s;
		max *= s;
	}

	void expand(const Box2f& other)
	{
		if (other.isEmpty() == false) {
			min = min.pickMin(other.min);
			min = min.pickMin(other.max);

			max = max.pickMax(other.min);
			max = max.pickMax(other.max);
		}
	}

	bool isInside(const vec2f& point) const
	{
		bool res = point.data[0] >= min.data[0] && point.data[0] <= max.data[0] && point.data[1] >= min.data[1] &&
		           point.data[1] <= max.data[1];

		return res;
	}

	bool overlaps(const Box2f& other) const
	{
		const bool xOverlap = doesOverlap1D(min.x, max.x, other.min.x, other.max.x);
		const bool yOverlap = doesOverlap1D(min.y, max.y, other.min.y, other.max.y);
		return xOverlap && yOverlap;
	}

	Box2f getOverlapBox(const Box2f& other) const
	{
		if (this->isEmpty() || other.isEmpty()) {
			return Box2f();
		}

		Box2f result;

		for (unsigned int t = 0; t < 2; ++t) {
			float omin, omax;
			overlapSegment1D(min.data[t], max.data[t], other.min.data[t], other.max.data[t], omin, omax);

			// If the overlap is negative, that means that the boxes do not overlap.
			// In this case just return an empty box.
			if (omin > omax) {
				return Box2f();
			}

			result.min[t] = omin;
			result.max[t] = omax;
		}

		return result;
	}

	bool operator==(const Box2f& other) const { return (min == other.min) && (max == other.max); }

	bool operator!=(const Box2f& other) const { return !(*this == other); }

  public:
	vec2f min;
	vec2f max;
};


} // namespace sge
