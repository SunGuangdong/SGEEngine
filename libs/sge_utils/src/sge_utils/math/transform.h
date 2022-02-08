#pragma once

#include "mat4f.h"
#include "quatf.h"

namespace sge {

/// A set of {Scaling, Rotation, Translation} transforms applied in that order.
/// Caution:
/// Unlike matrices, when combined with other transforms (multiplication basically) the
/// translation is applied in the parent-frame space,
/// oppused to the basis-space (world of example) which happens with regular matrices.
/// Because of this t1 * t0 != t1.toMatrix() * t0.toMatrix().
/// Additionally because of this, inversing a transform doesn't really make sense.
struct transf3d {
	vec3f p = vec3f(0.f);           // position.
	quatf r = quatf::getIdentity(); // rotation.
	vec3f s = vec3f(1.f);           // scaling.

	transf3d() = default;

	explicit transf3d(const vec3f& p, const quatf& r = quatf::getIdentity(), const vec3f& s = vec3f(1.f))
	    : p(p)
	    , r(r)
	    , s(s) {}

	static transf3d getIdentity() { return transf3d(vec3f(0.f), quatf::getIdentity(), vec3f(1.f)); }

	static transf3d fromMatrixMultWithScaling(const mat4f& AMtx, const mat4f& BMtx, const vec3f& targetScaling);

	void set(const vec3f& p_arg, const quatf& r_arg, const vec3f& s_arg) {
		p = p_arg;
		r = r_arg;
		s = s_arg;
	}

	transf3d getSelfNoScaling() const { return transf3d(p, r, vec3f(1.f)); }

	transf3d getSelfNoRotation() const { return transf3d(p, quatf::getIdentity(), s); }

	mat4f toMatrix() const;

	/// Returns the inverse transform of the current transform.
	/// However unlike matrices combining transforms with non-uniform scaling
	/// cannot really happen as transforms cannot perserve the orientation of the
	/// scaling. Instead use:
	transf3d inverseSimple() const;

	transf3d computeBindingTransform(const transf3d& parent) const;
	static transf3d applyBindingTransform(const transf3d& bindTransform, const transf3d& parentTrnasform);

	bool operator==(const transf3d& ref) const { return p == ref.p && r == ref.r && s == ref.s; }

	bool operator!=(const transf3d& ref) const { return !((*this) == ref); }
};

transf3d operator*(const transf3d& A, const transf3d& B);

template <>
inline transf3d lerp(const transf3d& a, const transf3d& b, const float t) {
	transf3d result;
	result.s = lerp(a.s, b.s, t);
	result.r = slerp(a.r, b.r, t);
	result.p = lerp(a.p, b.p, t);

	return result;
}



} // namespace sge
