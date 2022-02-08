#pragma once

#include "vec3f.h"
#include "mat4f.h"

namespace sge {

vec3f rotateTowards(const vec3f& from, const vec3f& to, const float maxRotationAmount, const vec3f& fallbackAxis)
{
	float const angle = acos(clamp(from.dot(to) / (from.length() * to.length()), -1.f, 1.f));

	if (angle <= maxRotationAmount) {
		return to;
	}

	vec3f c = from.cross(to);
	const float cLengthSqr = c.lengthSqr();
	if (c.lengthSqr() < 1e-5f) {
		c = fallbackAxis;
	}
	else {
		// Normalize the rotation axis.
		c = c / sqrtf(cLengthSqr);
	}

	mat4f const rMtx = mat4f::getAxisRotation(c, maxRotationAmount);
	vec3f const result = mat_mul_dir(rMtx, from) * to.length();
	return result;
}

} // namespace sge
