#pragma once

#include <limits>

#include "common.h"
#include "mat4f.h"
#include "math_base.h"
#include "vec3f.h"

#if defined(min) || defined(max)
	#error min/max macros are defined
#endif

namespace sge {

struct Box3f {
	Box3f() { setEmpty(); }

	Box3f(const vec3f& min, const vec3f& max)
	    : min(min)
	    , max(max)
	{
	}

	static Box3f getFromHalfDiagonal(const vec3f& halfDiagonal, const vec3f& offset = vec3f(0.f))
	{
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0 && halfDiagonal.z >= 0);

		Box3f retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	static Box3f getFromHalfDiagonalWithOffset(const vec3f& halfDiagonal, const vec3f& offset)
	{
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0 && halfDiagonal.z >= 0);

		Box3f retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	static Box3f getFromXZBaseAndSize(const vec3f& base, const vec3f& sizes)
	{
		vec3f min = base;
		min.x -= sizes.x * 0.5f;
		min.z -= sizes.z * 0.5f;

		vec3f max = base;
		max.x += sizes.x * 0.5f;
		max.y += sizes.y;
		max.z += sizes.z * 0.5f;

		return Box3f(min, max);
	}

	void move(const vec3f& offset)
	{
		min += offset;
		max += offset;
	}

	vec3f center() const { return (max + min) * 0.5f; }

	vec3f halfDiagonal() const { return (max - min) * 0.5f; }

	vec3f diagonal() const { return (max - min); }

	vec3f bottomMiddle() const
	{
		vec3f ret = center() - halfDiagonal().yOnly();
		return ret;
	}

	vec3f topMiddle() const
	{
		vec3f ret = center() + halfDiagonal().yOnly();
		return ret;
	}

	vec3f size() const { return max - min; }

	void setEmpty()
	{
		min = vec3f(std::numeric_limits<float>::max());
		max = vec3f(std::numeric_limits<float>::lowest());
	}

	bool IsEmpty() const { return min == vec3f(std::numeric_limits<float>::max()) && max == vec3f(std::numeric_limits<float>::lowest()); }

	void expand(const vec3f& point)
	{
		min = min.pickMin(point);
		max = max.pickMax(point);
	}

	void expandEdgesByCoefficient(float value)
	{
		vec3f exp = halfDiagonal() * value;
		min -= exp;
		max += exp;
	}

	void scale(const float& s)
	{
		min *= s;
		max *= s;
	}

	Box3f getScaledCentered(const vec3f& s) const
	{
		const vec3f c = center();
		vec3f h = halfDiagonal();
		Box3f result;
		result.expand(c - h * s);
		result.expand(c + h * s);
		return result;
	}

	void scaleAroundCenter(const vec3f& s)
	{
		const vec3f oldCenter = center();
		const vec3f oldSize = size();

		const vec3f newSize = oldSize * s;

		setEmpty();
		expand(oldCenter + newSize * 0.5f);
		expand(oldCenter - newSize * 0.5f);
	}

	void expand(const Box3f& other)
	{
		if (other.IsEmpty() == false) {
			min = min.pickMin(other.min);
			min = min.pickMin(other.max);

			max = max.pickMax(other.min);
			max = max.pickMax(other.max);
		}
	}

	bool isInside(const vec3f& point) const
	{
		bool res = true;

		res &= point.data[0] >= min.data[0] && point.data[0] <= max.data[0];
		res &= point.data[1] >= min.data[1] && point.data[1] <= max.data[1];
		res &= point.data[2] >= min.data[2] && point.data[2] <= max.data[2];

		return res;
	}

	bool overlaps(const Box3f& other) const
	{
		const bool xOverlap = doesOverlap1D(min.x, max.x, other.min.x, other.max.x);
		const bool yOverlap = doesOverlap1D(min.y, max.y, other.min.y, other.max.y);
		const bool zOverlap = doesOverlap1D(min.z, max.z, other.min.z, other.max.z);
		return xOverlap && yOverlap && zOverlap;
	}

	Box3f getOverlapBox(const Box3f& other) const
	{
		if (this->IsEmpty() || other.IsEmpty()) {
			return Box3f();
		}

		Box3f result;

		for (unsigned int t = 0; t < 3; ++t) {
			float omin, omax;
			overlapSegment1D(min.data[t], max.data[t], other.min.data[t], other.max.data[t], omin, omax);

			// If the overlap is negative, that means that the boxes do not overlap.
			// In this case just return an empty box.
			if (omin > omax) {
				return Box3f();
			}

			result.min[t] = omin;
			result.max[t] = omax;
		}

		return result;
	}

	/// Retireves the AABB corner points by index (0-7).
	vec3f getPoint(const int idx) const
	{
		if (idx == 0)
			return min;
		else if (idx == 1)
			return vec3f(min.x, min.y, max.z);
		else if (idx == 2)
			return vec3f(max.x, min.y, max.z);
		else if (idx == 3)
			return vec3f(max.x, min.y, min.z);
		else if (idx == 4)
			return vec3f(min.x, max.y, min.z);
		else if (idx == 5)
			return vec3f(min.x, max.y, max.z);
		else if (idx == 6)
			return max;
		else if (idx == 7)
			return vec3f(max.x, max.y, min.z);

		sgeAssert(false && "idx out of bounds! Needs to be in [0;7]");
		return vec3f(0.f);
	}

	/// Returns the transformed AXIS ALIGNED Bounding box.
	/// Performs a column-major matrix multiplication for every point.
	Box3f getTransformed(const mat4f& transform) const
	{
		if (this->IsEmpty())
			return Box3f();

		Box3f retval;

		retval.expand(mat_mul_pos(transform, getPoint(0)));
		retval.expand(mat_mul_pos(transform, getPoint(1)));
		retval.expand(mat_mul_pos(transform, getPoint(2)));
		retval.expand(mat_mul_pos(transform, getPoint(3)));
		retval.expand(mat_mul_pos(transform, getPoint(4)));
		retval.expand(mat_mul_pos(transform, getPoint(5)));
		retval.expand(mat_mul_pos(transform, getPoint(6)));
		retval.expand(mat_mul_pos(transform, getPoint(7)));


		return retval;
	}

	bool intersectFast(const vec3f& origin, const vec3f& invDir, float& t) const
	{
		const float t1 = (min.x - origin.x) * invDir.x;
		const float t2 = (max.x - origin.x) * invDir.x;
		const float t3 = (min.y - origin.y) * invDir.y;
		const float t4 = (max.y - origin.y) * invDir.y;
		const float t5 = (min.z - origin.z) * invDir.z;
		const float t6 = (max.z - origin.z) * invDir.z;

		float tmin = maxOf(maxOf(minOf(t1, t2), minOf(t3, t4)), minOf(t5, t6));
		float tmax = minOf(minOf(maxOf(t1, t2), maxOf(t3, t4)), maxOf(t5, t6));

		// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us.
		if (tmax < 0) {
			t = tmax;
			return false;
		}

		// if tmin > tmax, ray doesn't intersect AABB
		if (tmin > tmax) {
			t = tmax;
			return false;
		}

		t = tmin;
		return true;
	}

	bool intersect(const vec3f& origin, const vec3f& dir, float& t0, float& t1) const
	{
		float tmin, tmax, tymin, tymax, tzmin, tzmax;

		if (dir.x >= 0.f) {
			tmin = (min.x - origin.x) / dir.x;
			tmax = (max.x - origin.x) / dir.x;
		}
		else {
			tmin = (max.x - origin.x) / dir.x;
			tmax = (min.x - origin.x) / dir.x;
		}

		if (dir.y >= 0.f) {
			tymin = (min.y - origin.y) / dir.y;
			tymax = (max.y - origin.y) / dir.y;
		}
		else {
			tymin = (max.y - origin.y) / dir.y;
			tymax = (min.y - origin.y) / dir.y;
		}

		if ((tmin > tymax) || (tymin > tmax)) {
			return false;
		}

		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;

		if (dir.z >= 0.f) {
			tzmin = (min.z - origin.z) / dir.z;
			tzmax = (max.z - origin.z) / dir.z;
		}
		else {
			tzmin = (max.z - origin.z) / dir.z;
			tzmax = (min.z - origin.z) / dir.z;
		}

		if ((tmin > tzmax) || (tzmin > tmax)) {
			return false;
		}

		if (tzmin > tmin)
			tmin = tzmin;
		if (tzmax < tmax)
			tmax = tzmax;

		t0 = tmin;
		t1 = tmax;

		return true;
		// return ((tmin < t1) && (tmax > t0));
	}

	/// Moves the specified face of the bounding box along its normal.
	// Box3f movedFaceBy(const SignedAxis axis, float amount) const {
	//	Box3f tmp = *this;
	//	switch (axis) {
	//		case axis_x_pos: {
	//			tmp.max.x += amonut;
	//		} break;
	//		case axis_y_pos: {
	//			tmp.max.y += amonut;
	//		} break;
	//		case axis_z_pos: {
	//			tmp.max.z += amonut;
	//		} break;
	//		case axis_x_neg: {
	//			tmp.min.x -= amonut;
	//		} break;
	//		case axis_y_neg: {
	//			tmp.min.y -= amonut;
	//		} break;
	//		case axis_z_neg: {
	//			tmp.min.z += amonut;
	//		} break;
	//	}

	//	// Handle the situation where the amonut swapped the position of min/max points.
	//	Box3f result;
	//	result.expand(tmp.min);
	//	result.expand(tmp.max);

	//	return result;
	//}

	/// Moves the specified face of the bounding box to the specified value along it's normal.
	Box3f movedFaceTo(const SignedAxis axis, float pos) const
	{
		Box3f tmp = *this;
		switch (axis) {
			case axis_x_pos: {
				tmp.max.x = pos;
			} break;
			case axis_y_pos: {
				tmp.max.y = pos;
			} break;
			case axis_z_pos: {
				tmp.max.z = pos;
			} break;
			case axis_x_neg: {
				tmp.min.x = pos;
			} break;
			case axis_y_neg: {
				tmp.min.y = pos;
			} break;
			case axis_z_neg: {
				tmp.min.z = pos;
			} break;
		}

		// Handle the situation where the amonut swapped the position of min/max points.
		Box3f result;
		result.expand(tmp.min);
		result.expand(tmp.max);

		return result;
	}

	vec3f getFaceCenter(SignedAxis axis) const
	{
		const vec3f c = center();
		const vec3f halfDiag = halfDiagonal();

		if (axis == axis_x_pos)
			return c + halfDiag.xOnly();
		else if (axis == axis_y_pos)
			return c + halfDiag.yOnly();
		else if (axis == axis_z_pos)
			return c + halfDiag.zOnly();
		else if (axis == axis_x_neg)
			return c - halfDiag.xOnly();
		else if (axis == axis_y_neg)
			return c - halfDiag.yOnly();
		else if (axis == axis_z_neg)
			return c - halfDiag.zOnly();

		sgeAssert(false);
		return vec3f(0.f);
	}

	void getFacesCenters(vec3f facesCenters[signedAxis_numElements]) const
	{
		const vec3f c = center();
		const vec3f halfDiag = halfDiagonal();

		facesCenters[axis_x_pos] = c + halfDiag.xOnly();
		facesCenters[axis_y_pos] = c + halfDiag.yOnly();
		facesCenters[axis_z_pos] = c + halfDiag.zOnly();
		facesCenters[axis_x_neg] = c - halfDiag.xOnly();
		facesCenters[axis_y_neg] = c - halfDiag.yOnly();
		facesCenters[axis_z_neg] = c - halfDiag.zOnly();
	}

	void getFacesNormals(vec3f facesCenters[signedAxis_numElements]) const
	{
		facesCenters[axis_x_pos] = vec3f::getAxis(0);
		facesCenters[axis_y_pos] = vec3f::getAxis(1);
		facesCenters[axis_z_pos] = vec3f::getAxis(2);
		facesCenters[axis_x_neg] = -vec3f::getAxis(0);
		facesCenters[axis_y_neg] = -vec3f::getAxis(1);
		facesCenters[axis_z_neg] = -vec3f::getAxis(2);
	}

	bool operator==(const Box3f& other) const { return (min == other.min) && (max == other.max); }

	bool operator!=(const Box3f& other) const { return !(*this == other); }

	vec3f min, max;
};

} // namespace sge
