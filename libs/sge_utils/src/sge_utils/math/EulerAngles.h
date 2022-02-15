#pragma once

#include "sge_utils/math/common.h"
#include "sge_utils/math/quatf.h"
#include "sge_utils/math/vec3f.h"

namespace sge {

inline quatf eulerToQuaternion(vec3f euler)
{
	// euler.x = normalizeAnglePiRange(euler.x);
	// sgeAssert(euler.x >= -half_pi() && euler.x <= half_pi());
	float ex = euler.x * 0.5f;
	float ey = euler.y * 0.5f;
	float ez = euler.z * 0.5f;
	float sinX = sinf(ex);
	float cosX = cosf(ex);
	float sinY = sinf(ey);
	float cosY = cosf(ey);
	float sinZ = sinf(ez);
	float cosZ = cosf(ez);

	quatf q;

	q.w = cosY * cosX * cosZ + sinY * sinX * sinZ;
	q.x = cosY * sinX * cosZ + sinY * cosX * sinZ;
	q.y = sinY * cosX * cosZ - cosY * sinX * sinZ;
	q.z = cosY * cosX * sinZ - sinY * sinX * cosZ;
	return q;
}

inline quatf eulerToQuaternionDegrees(float x, float y, float z)
{
	return eulerToQuaternion(vec3f(deg2rad(x), deg2rad(y), deg2rad(z)));
}

inline vec3f quaternionToEuler(const quatf& q)
{
	const float x = q.x;
	const float y = q.y;
	const float z = q.z;
	const float w = q.w;

	const float check = 2.0f * (-y * z + w * x);

	if (check < -0.995f) {
		return vec3f(-sgePi * 0.5f, 0.0f, -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)));
	}
	if (check > 0.995f) {
		return vec3f(sgePi * 0.5f, 0.0f, atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)));
	}
	return vec3f(
	    asinf(check),
	    atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)),
	    atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)));
}

} // namespace sge
