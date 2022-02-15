#pragma once

#include "transform.h"

namespace sge {

transf3d transf3d::fromMatrixMultWithScaling(const mat4f& AMtx, const mat4f& BMtx, const vec3f& targetScaling)
{
	// The idea of this function is to compute the orientation of the matrix
	// and then extract the scaling of the matrix.
	mat4f m = (AMtx * BMtx);
	m = m.removedScaling();

	const vec3f targetScalingSigns(
	    targetScaling.x >= 0.f ? +1.f : -1.f,
	    targetScaling.y >= 0.f ? +1.f : -1.f,
	    targetScaling.z >= 0.f ? +1.f : -1.f);

	// apply negative scale back to axes
	m.data[0].setXyz(m.data[0].xyz() * targetScalingSigns.x);
	m.data[1].setXyz(m.data[1].xyz() * targetScalingSigns.y);
	m.data[2].setXyz(m.data[2].xyz() * targetScalingSigns.z);

	const quatf rotation = m.toQuat().normalized();

	// set values back to output
	transf3d result;
	result.s = targetScaling;
	result.r = rotation;

	result.p = m.data[3].xyz();

	return result;
}

mat4f transf3d::toMatrix() const
{
#if 0
	const mat4f translation = mat4f::getTranslation(p);
	const mat4f rotation = mat4f::getRotationQuat(r);
	const mat4f scale = mat4f::getScaling(s);

	return translation * rotation * scale;
#else

	mat4f result;

	// Apply the translation.
	result.data[3][0] = p.x;
	result.data[3][1] = p.y;
	result.data[3][2] = p.z;

	// Apply the rotation and scaling (basically a combied version of quternion to matrix multiplied by scaling).
	const float x2 = r.x + r.x;
	const float y2 = r.y + r.y;
	const float z2 = r.z + r.z;

	{
		const float xx2 = r.x * x2;
		const float yy2 = r.y * y2;
		const float zz2 = r.z * z2;

		result.data[0][0] = (1.0f - (yy2 + zz2)) * s.x;
		result.data[1][1] = (1.0f - (xx2 + zz2)) * s.y;
		result.data[2][2] = (1.0f - (xx2 + yy2)) * s.z;
	}

	{
		const float yz2 = r.y * z2;
		const float wx2 = r.w * x2;

		result.data[2][1] = (yz2 - wx2) * s.z;
		result.data[1][2] = (yz2 + wx2) * s.y;
	}

	{
		const float xy2 = r.x * y2;
		const float wz2 = r.w * z2;

		result.data[1][0] = (xy2 - wz2) * s.y;
		result.data[0][1] = (xy2 + wz2) * s.x;
	}

	{
		const float xz2 = r.x * z2;
		const float wy2 = r.w * y2;

		result.data[2][0] = (xz2 + wy2) * s.z;
		result.data[0][2] = (xz2 - wy2) * s.x;
	}

	result.data[0][3] = 0.0f;
	result.data[1][3] = 0.0f;
	result.data[2][3] = 0.0f;
	result.data[3][3] = 1.0f;

	return result;
#endif
}

transf3d transf3d::inverseSimple() const
{
	const quatf invRotation = r.inverse();
	vec3f invScaling;
	invScaling.x = s.x ? 1.f / s.x : 1e-6f;
	invScaling.y = s.y ? 1.f / s.y : 1e-6f;
	invScaling.z = s.z ? 1.f / s.z : 1e-6f;
	const vec3f invTranslation = quat_mul_pos(invRotation, (invScaling * -p));

	transf3d result;
	result.r = invRotation;
	result.s = invScaling;
	result.p = invTranslation;

	return result;
}

transf3d transf3d::computeBindingTransform(const transf3d& parentTrnasform) const
{
	const vec3f parentInvScaling = parentTrnasform.s.reciprocalSafe();
	const quatf parentInvRotation = parentTrnasform.r.inverse();

	transf3d bindTform;
	bindTform.s = parentInvScaling * s;
	bindTform.r = parentInvRotation * r;
	bindTform.p = quat_mul_pos(parentInvRotation, p - parentTrnasform.p) * parentInvScaling;

	return bindTform;
}

transf3d transf3d::applyBindingTransform(const transf3d& bindTransform, const transf3d& parentTrnasform)
{
	transf3d child = parentTrnasform * bindTransform;
	return child;
}

transf3d operator*(const transf3d& A, const transf3d& B)
{
	transf3d result;
	result.r = A.r * B.r;
	result.s = A.s * B.s;
	// Unlike matrix multiplication, the translation here is applied in the A based orientation,
	// not in basis the frame (world space for example).
	result.p = A.p + quat_mul_pos(A.r, B.p * A.s);
	return result;
}

} // namespace sge
