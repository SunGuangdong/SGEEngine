#include "Frustum.h"

namespace sge {

// Returns true if the sphere is outside of the frustum.
bool Frustum::isSphereOutside(const vec3f& pos, float const radius) const
{
	for (int t = 0; t < 6; ++t) {
		float const dist = plane(t).Distance(pos);
		if (dist + radius < 0.f)
			return true;
	}

	return false;
}

bool Frustum::is8PointConvexHullOutside(const vec3f points[8]) const
{
	bool isInsideOrIntersects = true;
	for (int iPlane = 0; iPlane < 6; ++iPlane) {
		int in = 0;
		int out = 0;
		for (int iPt = 0; iPt < 8 && (in == 0 || out == 0); ++iPt) {
			float const dist = plane(iPlane).Distance(points[iPt]);
			if (dist < 0.f)
				out++;
			else
				in++;
		}

		if (in == 0)
			return true;
		else if (out != 0) {
			isInsideOrIntersects = true;
		}
	}

	return !isInsideOrIntersects;
}

bool Frustum::isBoxOutside(const Box3f& aabbWs) const
{
	const vec3f points[8] = {
	    aabbWs.getPoint(0), aabbWs.getPoint(1), aabbWs.getPoint(2), aabbWs.getPoint(3),
	    aabbWs.getPoint(4), aabbWs.getPoint(5), aabbWs.getPoint(6), aabbWs.getPoint(7),
	};

	const bool result = is8PointConvexHullOutside(points);
	return result;
}

bool Frustum::isObjectOrientedBoxOutside(const Box3f& aabbOs, const mat4f& objToFrustumSpace) const
{
	return isBoxOutside(aabbOs.getTransformed(objToFrustumSpace));
}

void Frustum::getCorners(vec3f result[8], Optional<float> overrideNearToFarDistance) const
{
	const auto intersectPlanes = [](const Plane& p0, const Plane& p1, const Plane& p2) -> vec3f {
		// http://www.ambrsoft.com/TrigoCalc/Plan3D/3PlanesIntersection_.htm
		float const det = -triple(p0.norm(), p1.norm(), p2.norm()); // Caution: I'm not sure about that minus...

		float const x = triple(p0.v4.wyz(), p1.v4.wyz(), p2.v4.wyz()) / det;
		float const y = triple(p0.v4.xwz(), p1.v4.xwz(), p2.v4.xwz()) / det;
		float const z = triple(p0.v4.xyw(), p1.v4.xyw(), p2.v4.xyw()) / det;

		return vec3f(x, y, z);
	};

	Plane fPlane = f;
	if (overrideNearToFarDistance.isValid()) {
		fPlane = n;
		fPlane.v4.setXyz(-n.v4.xyz());
		fPlane.v4.w = -fPlane.v4.w + overrideNearToFarDistance.get();

		fPlane = fPlane.Normalized();
	}

	result[0] = intersectPlanes(t, r, n);
	result[1] = intersectPlanes(t, l, n);

	result[2] = intersectPlanes(b, l, n);
	result[3] = intersectPlanes(b, r, n);

	result[4] = intersectPlanes(t, r, fPlane);
	result[5] = intersectPlanes(t, l, fPlane);

	result[6] = intersectPlanes(b, l, fPlane);
	result[7] = intersectPlanes(b, r, fPlane);
}

Frustum Frustum ::extractClippingPlanes(mat4f mtx, bool d3dStyle)
{
	mtx = mtx.transposed();

	Frustum f;

	f.l.v4 = mtx.data[3] + mtx.data[0];
	f.r.v4 = mtx.data[3] - mtx.data[0];
	f.b.v4 = mtx.data[3] + mtx.data[1];
	f.t.v4 = mtx.data[3] - mtx.data[1];
	if (d3dStyle)
		f.n.v4 = mtx.data[2];
	else
		f.n.v4 = mtx.data[3] + mtx.data[2];

	f.f.v4 = mtx.data[3] - mtx.data[2];

	f.l = f.l.Normalized();
	f.r = f.r.Normalized();
	f.b = f.b.Normalized();
	f.t = f.t.Normalized();
	f.n = f.n.Normalized();
	f.f = f.f.Normalized();

	return f;
}

} // namespace sge
