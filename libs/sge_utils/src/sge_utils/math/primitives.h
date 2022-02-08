#pragma once
#include "math_base.h"
#include "vec4f.h"
#include <cfloat>

namespace sge {

// Explicit Ray: pt = p + t*d
struct Ray {
	Ray() = default;

	Ray(const vec3f& pos, const vec3f& dir)
	    : pos(pos)
	    , dir(dir) {
	}

	vec3f Sample(const float t) const {
		return pos + (t * dir);
	}

	vec3f pos; // origin
	vec3f dir; // dir
};

// returns FLT_MAX if no intersection was found.
inline float intersectRayPlane(const vec3f& pos, const vec3f& dir, const vec3f& planeNormal, float const planeD) {
	float const denom = dot(dir, planeNormal);

	if (denom < 1e-6f && denom > -1e-6f)
		return FLT_MAX;
	float t = -(planeD + dot(pos, planeNormal)) / denom;
	return t;
}

// Implicit plane: N(A,B,C), |N| = 1, Ax + By + Cz + D = 0
// [TODO] Remove that |N| = 1, as nobody will ever be able to guarantee it!
// (And it's kind of stupid, what was i thinking!).
struct Plane {
	Plane() = default;

	Plane(const vec3f& norm, const float& d)
	    : v4(norm, d) {
	}

	vec4f v4;

	vec3f norm() const {
		return v4.xyz();
	}

	float d() const {
		return v4.w;
	}

	void setNormal(const vec3f& normal) {
		v4 = vec4f(normal, v4.w);
	}

	void setDistance(const float distance) {
		v4.w = distance;
	}


	float Distance(const vec3f pt) const {
		return dot(norm(), pt) + d();
	}

	Plane Normalized() const {
		float l = norm().length();

		if (l < 1e-6f) {
			return *this;
		}

		float invL = 1.f / l;
		return Plane(norm() * invL, d() * invL);
	}

	friend Plane normalized(const Plane& v) {
		return v.Normalized();
	}

	vec3f Project(const vec3f& pt) const {
		return pt - Distance(pt) * norm();
	}

	/// Creates a normalized plane form Counter Clockwise Triangle Winding.
	static Plane FromTriangle_CCW(const vec3f p[3]) {
		const vec3f e1 = p[1] - p[0];
		const vec3f e2 = p[2] - p[0];
		const vec3f N = normalized(cross(e1, e2));
		const float planeConstant = -dot(N, p[0]);

		return Plane(N, planeConstant);
	}

	/// Creates a plane defined by the specified position and the normal direction.
	static Plane FromPosAndDir(const vec3f& pos, const vec3f dir) {
		Plane plane;
		plane.v4 = vec4f(normalized(dir), -dot(pos, dir));
		return plane;
	}

	/// Returns the distance along the ray where the intersection happened.
	/// FLT_MAX is returned if no intersection was found.
	float intersectRay(const Ray& ray) const {
		return intersectRayPlane(ray.pos, ray.dir, v4.xyz(), v4.w);
	}
};

struct Sphere {
	Sphere(const vec3f& pos, const float radius)
	    : pos(pos)
	    , radius(radius) {
	}

	vec3f pos;
	float radius;
};



// Intersects a Ray with a Sphere :
// Returns the number of intersection points, and the coeff in t0, t1. The points aren't sorted!
inline int intersectRaySphere(const Ray& ray, const Sphere& sphere, float& t0, float& t1) {
	// sphere is defined by dot((p - center), (p - center)) = radius^2
	// line is defined by p = origin + t * direction

	const vec3f originDiff = ray.pos - sphere.pos;

	const float a = ray.dir.lengthSqr();
	const float b = 2 * dot(ray.dir, originDiff);
	const float c = originDiff.lengthSqr() - (sphere.radius * sphere.radius);

	const float discriminant = b * b - (4.f * a * c);

	if (discriminant < 0.f)
		return 0;

	// That case is not that important I guess...?
	if (discriminant == 0.f) {
		const float t = (-b) / (2.f * a);

		if (t < 0.f)
			return false;

		t0 = t;
		return 1;
	}

	// [TODO] Find a better, numerically stable way of finding the min and max solution of the quadratic equation.
	const float sqrtD = sqrtf(discriminant);
	const float x1 = (-b - sqrtD) / (2.f * a);
	const float x2 = (-b + sqrtD) / (2.f * a);

	int numIntersections = 0;

	if (!(x1 < 0.f)) {
		t0 = x1;
		numIntersections += 1;
	}

	if (!(x2 < 0.f)) {
		((numIntersections == 0) ? t0 : t1) = x2;
		numIntersections += 1;
	}

	return numIntersections;
}

inline float projectPointOnLine(const vec3f& o1, const vec3f& d1, const vec3f pt) {
	const vec3f p = pt - o1;
	const float proj = dot(d1, p) / sqr(d1.length());
	return proj;
}

inline float IntersectRayTriangle(const Ray& ray, const vec3f p[3]) {
	const vec3f e[2] = {
	    p[1] - p[0],
	    p[2] - p[0],
	};

	const vec3f P = cross(ray.dir, e[1]);
	const float det = dot(e[0], P);

	if (det > -FLT_EPSILON && det < FLT_EPSILON)
		return FLT_MAX;
	const float inv_det = 1.f / det;

	const vec3f T = ray.pos - p[0];

	const float u = dot(T, P) * inv_det;
	if (u < 0.f || u > 1.f)
		return FLT_MAX;

	const vec3f Q = cross(T, e[0]);
	const float v = dot(ray.dir, Q) * inv_det;

	if (v < 0.f || u + v > 1.f)
		return FLT_MAX;

	const float t = dot(e[1], Q) * inv_det;

	return t;
}

inline float IntersectRayQuad(const Ray& ray, const vec3f o, const vec3f e1, const vec3f e2) {
	const vec3f P = cross(ray.dir, e2);
	const float det = dot(e1, P);

	if (det > -FLT_EPSILON && det < FLT_EPSILON)
		return FLT_MAX;
	const float inv_det = 1.f / det;

	const vec3f T = ray.pos - o;

	const float u = dot(T, P) * inv_det;
	if (u < 0.f || u > 1.f)
		return FLT_MAX;

	const vec3f Q = cross(T, e1);
	const float v = dot(ray.dir, Q) * inv_det;

	if (v < 0.f || v > 1.f)
		return FLT_MAX;

	const float t = dot(e2, Q) * inv_det;

	return t;
}

// https://en.wikipedia.org/wiki/Skew_lines
inline float getLineDistance(const vec3f& o1, const vec3f& d1, const vec3f& o2, const vec3f& d2) {
	vec3f n = cross(d1, d2);
	const float n_ln = n.length();

	// Check if the lines are perpendicullar.
	if (n_ln < 1e-6f) {
		// In that case project a random point form l2 on l1 and measure the distance.
		const float proj = projectPointOnLine(o1, d1, o2);
		const vec3f o2_on_l1 = (o1 + d1 * proj);
		return length(o2_on_l1 - o2);
	}

	const float inv_n_ln = 1 / n_ln;
	n *= inv_n_ln;

	const float distance = fabsf(dot(n, o1 - o2));
	return distance;
}

// https://en.wikipedia.org/wiki/Skew_lines
inline float getRaySegmentDistance(const vec3f& o1, const vec3f& d1, const vec3f& a, const vec3f& b, float* pRayLerp = nullptr) {
	const vec3f d2 = b - a;

	vec3f n = cross(d1, d2);
	const float n_ln = n.length();

	// Check if the lines are parallel.
	if (n_ln < 1e-6f) {
		const float proj_a = projectPointOnLine(o1, d1, a);
		const float proj_b = projectPointOnLine(o1, d1, a);

		// Find the "nearest" point to o1. The worst case is "a" and "b" are behind o1. Otherwise it doesn't matter.
		const vec3f ptAB = proj_a > proj_b ? a : b;
		const float proj = proj_a < proj_b ? proj_a : proj_b;

		if (pRayLerp != nullptr) {
			*pRayLerp = proj;
		}

		const vec3f pt = o1 + d1 * proj;
		return length(pt - ptAB);
	}

	const vec3f n1 = cross(n, d1);
	const vec3f n2 = cross(n, d2);

	const float t1 = clamp(dot(a - o1, n2) / dot(d1, n2), 0.f, FLT_MAX);
	const float t2 = clamp(dot(o1 - a, n1) / dot(d2, n1), 0.f, 1.f);

	if (pRayLerp != nullptr) {
		*pRayLerp = t1;
	}

	const vec3f pt1 = o1 + d1 * t1;
	const vec3f pt2 = a + d2 * t2;

	return length(pt1 - pt2);
}

} // namespace sge
