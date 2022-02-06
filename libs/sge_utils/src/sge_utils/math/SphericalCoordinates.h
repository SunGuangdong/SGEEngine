#pragma once

#include "vec3.h"

namespace sge {

inline float toSphericalYUp(float& angleFromY, float& angleAroundY, const vec3f& d)
{
	const float r = d.length();
	if (r > 1e-6f) {
		angleFromY = acosf(d.y / r);
		angleAroundY = atan2(-d.z, d.x);
	}
	else {
		angleFromY = 0.f;
		angleAroundY = 0.f;
	}
	return r;
}

inline vec3f fromSphericalYUp(float angleFromY, float angleAroundY, float radius)
{
	vec3f r;

	float s = sin(angleFromY);

	r.y = radius * cos(angleFromY);
	r.x = radius * cos(angleAroundY) * s;
	r.z = radius * sin(angleAroundY) * s;

	return r;
}

struct SphericalRotation {
	float fromY = 0.f;
	float aroundY = 0.f;

	SphericalRotation() = default;

	SphericalRotation(float fromY, float aroundY)
	    : fromY(fromY)
	    , aroundY(aroundY)
	{
	}

	bool operator==(SphericalRotation& ref) const
	{
		return fromY == ref.fromY && aroundY == ref.aroundY;
	}

	bool operator!=(SphericalRotation& ref) const
	{
		return !(*this == ref);
	}

	vec3f getDirection() const
	{
		return fromSphericalYUp(fromY, aroundY, 1.f);
	}

	vec3f transformPt(const vec3f& pt) const
	{
		return quat_mul_pos(toQuaternion(), pt);
	}

	quatf toQuaternion() const
	{
		vec3f dir = getDirection();
		quatf q = quatf::fromNormalizedVectors(vec3f(0.f, 1.f, 0.f), dir, vec3f(0.f, 1.f, 0.f));
		return q;
	}

	static SphericalRotation axis_y()
	{
		return SphericalRotation();
	}

	static SphericalRotation axis_neg_y()
	{
		return SphericalRotation(two_pi(), 0.f);
	}

	static SphericalRotation fromDirection(const vec3f& dir)
	{
		SphericalRotation res;
		[[maybe_unused]] float radius_unused = toSphericalYUp(res.fromY, res.aroundY, dir);
		return res;
	}
};


struct SphericalCoord {
	float fromY = 0.f;
	float aroundY = 0.f;
	float radius = 0.f;

	SphericalCoord() = default;

	SphericalCoord(float fromY, float aroundY, float radius)
	    : fromY(fromY)
	    , aroundY(aroundY)
	    , radius(radius)
	{
	}

	SphericalCoord(const SphericalRotation& rotation, float radius)
	    : fromY(rotation.fromY)
	    , aroundY(rotation.aroundY)
	    , radius(radius)
	{
	}

	bool operator==(SphericalCoord& ref) const
	{
		return fromY == ref.fromY && aroundY == ref.aroundY && radius == ref.radius;
	}

	bool operator!=(SphericalCoord& ref) const
	{
		return !(*this == ref);
	}


	vec3f getDirection() const
	{
		return fromSphericalYUp(fromY, aroundY, radius);
	}

	vec3f transformPt(const vec3f& pt) const
	{
		vec3f fromYAxisRotationAxis = cross(pt, vec3f(0.f, 1.f, 0.f));
		const float fromYAxisRotationAxisLen = fromYAxisRotationAxis.length();

		// The point is colinear with the Y axis, meaning that all roatations around Y axis
		// will result in the same point, and that the rotation from +Y downwards does not have
		// any defined direction.
		if (fromYAxisRotationAxisLen < 1e-6f) {
			return pt;
		}

		// Normalize the axis.
		fromYAxisRotationAxis *= 1.f / fromYAxisRotationAxisLen;

		// Apply the rotation from Y.
		vec3f ptResult = rotateAround(fromYAxisRotationAxis, fromY, pt);

		// Apply the rotation around Y.
		ptResult = rotateAround(vec3f(0.f, 1.f, 0.f), aroundY, ptResult);

		// Scale the point by the radius.
		ptResult *= radius;

		return ptResult;
	}

	static SphericalCoord fromDirection(const vec3f& dir)
	{
		SphericalCoord res;
		res.radius = toSphericalYUp(res.fromY, res.aroundY, dir);
		return res;
	}
};

} // namespace sge
