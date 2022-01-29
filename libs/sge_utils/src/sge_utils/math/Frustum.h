#pragma once

#include "Box.h"
#include "mat4.h"
#include "primitives.h"
#include "sge_utils/containers/Optional.h"

namespace sge {

struct Frustum {
	Plane l, r, b, t, n, f;

  public:
	const Plane& plane(const int i) const {
		sgeAssert(i >= 0 && i < 6);
		if (i == 0)
			return l;
		if (i == 1)
			return r;
		if (i == 2)
			return b;

		if (i == 3)
			return t;
		if (i == 4)
			return n;

		return f; // 5
	}

	// Returns true if the sphere is outside of the frustum.
	bool isSphereOutside(const vec3f& pos, float const radius) const;
	bool is8PointConvexHullOutside(const vec3f points[8]) const;
	bool isBoxOutside(const AABox3f& aabbWs) const;
	bool isObjectOrientedBoxOutside(const AABox3f& aabbOs, const mat4f& objToFrustumSpace) const;

	void getCorners(vec3f result[8], Optional<float> overrideNearToFarDistance = NullOptional()) const;

	/// Extracts the clipping planes based on the specified matrix.
	/// Depending on the exact matrix the clip planes could be in different space.
	/// If the matrix is the projView than the clipping planes are in world space.
	static Frustum extractClippingPlanes(mat4f mtx, bool d3dStyle);
};

} // namespace sge
