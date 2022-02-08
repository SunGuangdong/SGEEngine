#pragma once

#include "common.h"
#include "sge_utils/math/quatf.h"
#include "sge_utils/math/vec4f.h"

namespace sge {

/// mat4f is a column major matrix used for transformations.
struct mat4f {
	union {
		vec4f data[4];
		struct {
			vec4f c0;
			vec4f c1;
			vec4f c2;
			vec4f c3;
		};
	};

	bool operator==(const mat4f& other) const
	{
		bool v = true;

		v &= data[0] == other.data[0];
		v &= data[1] == other.data[1];
		v &= data[2] == other.data[2];
		v &= data[3] == other.data[3];


		return v;
	}

	bool operator!=(const mat4f& other) const
	{
		bool v = true;

		v &= data[0] != other.data[0];
		v &= data[1] != other.data[1];
		v &= data[2] != other.data[2];
		v &= data[3] != other.data[3];


		return v;
	}

	static mat4f getDiagonal(const float s)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0, s);
		result.data[1] = vec4f::getAxis(1, s);
		result.data[2] = vec4f::getAxis(2, s);
		result.data[3] = vec4f::getAxis(3, s);

		return result;
	}

	static mat4f getIdentity() { return getDiagonal(1.f); }

	static mat4f getZero() { return getDiagonal(0.f); }

	vec4f getRow(int iRow) const
	{
		sgeAssert(iRow >= 0 && iRow < 4);
		vec4f result;


		result.data[0] = data[0][iRow];
		result.data[1] = data[1][iRow];
		result.data[2] = data[2][iRow];
		result.data[3] = data[3][iRow];


		return result;
	}

	float& at(unsigned const int r, const unsigned int c) { return data[c][r]; }
	const float at(unsigned const int r, const unsigned int c) const { return data[c][r]; }

	mat4f& operator*=(const float s)
	{
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		data[3] *= s;

		return *this;
	}

	mat4f operator*(const float s) const
	{
		mat4f result(*this);
		result *= s;
		return result;
	}

	friend mat4f operator*(const float s, const mat4f& m) { return m * s; }

	//---------------------------------------------------
	// operator+ and +=
	//---------------------------------------------------
	mat4f& operator+=(const mat4f& m)
	{
		data[0] += m.data[0];
		data[1] += m.data[1];
		data[2] += m.data[2];
		data[3] += m.data[3];

		return *this;
	}

	mat4f operator+(const mat4f& m) const
	{
		mat4f result(*this);
		result += m;
		return result;
	}

	//---------------------------------------------------
	// operator- and -=
	//---------------------------------------------------
	mat4f& operator-=(const mat4f& m)
	{
		data[0] -= m.data[0];
		data[1] -= m.data[1];
		data[2] -= m.data[2];
		data[3] -= m.data[3];

		return *this;
	}

	mat4f operator-(const mat4f& m) const
	{
		mat4f result(*this);
		result -= m;
		return result;
	}

	//---------------------------------------------------
	// Vector * Matrix  -> (1, NUM_COL) (a ROW vector)
	//---------------------------------------------------
	friend vec4f operator*(const vec4f& v, const mat4f& m)
	{
		vec4f result;

		result[0] = dot(v, m.data[0]);
		result[1] = dot(v, m.data[1]);
		result[2] = dot(v, m.data[2]);
		result[3] = dot(v, m.data[3]);

		return result;
	}

	//---------------------------------------------------
	// Matrix * Vector -> (NUM_ROW, 1) (a COLUMN vector)
	//---------------------------------------------------
	friend vec4f operator*(const mat4f& m, const vec4f& v)
	{
		vec4f result = vec4f::getZero();

		result += m.data[0] * v[0];
		result += m.data[1] * v[1];
		result += m.data[2] * v[2];
		result += m.data[3] * v[3];

		return result;
	}

	//---------------------------------------------------
	// Matrix * Matrix mathematical multiplication
	//---------------------------------------------------
	friend mat4f operator*(const mat4f& a, const mat4f& b)
	{
		mat4f r;

		r.data[0] = a * b.data[0];
		r.data[1] = a * b.data[1];
		r.data[2] = a * b.data[2];
		r.data[3] = a * b.data[3];


		return r;
	}

	//////---------------------------------------------------
	////// Matrix * Matrix componentwise multiplication
	//////---------------------------------------------------
	////friend mat4f cmul(const mat4f& a, const mat4f& b)
	////{
	////	mat4f r;

	////	r.data[0] = cmul(a.data[0], b.data[0]);
	////	r.data[1] = cmul(a.data[1], b.data[1]);
	////	r.data[2] = cmul(a.data[2], b.data[2]);
	////	r.data[3] = cmul(a.data[3], b.data[3]);

	////	return r;
	////}

	//---------------------------------------------------
	// transpose
	//---------------------------------------------------
	mat4f transposed() const
	{
		mat4f result;

		for (int t = 0; t < 4; ++t)
			for (int s = 0; s < 4; ++s) {
				result.at(t, s) = at(s, t);
			}

		return result;
	}

	friend mat4f transposed(const mat4f& m) { return m.transposed(); }

	friend vec4f mat_mul_vec(const mat4f& m, const vec4f& v)
	{
		vec4f result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];
		result[0] += m.data[3][0] * v[3];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];
		result[1] += m.data[3][1] * v[3];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];
		result[2] += m.data[3][2] * v[3];

		result[3] = m.data[0][3] * v[0];
		result[3] += m.data[1][3] * v[1];
		result[3] += m.data[2][3] * v[2];
		result[3] += m.data[3][3] * v[3];

		return result;
	}

	friend vec3f mat_mul_pos(const mat4f& m, const vec3f& v)
	{
		vec3f result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];
		result[0] += m.data[3][0];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];
		result[1] += m.data[3][1];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];
		result[2] += m.data[3][2];

		return result;
	}

	vec3f transfPos(const vec3f& pos) const { return mat_mul_pos(*this, pos); }

	friend vec3f mat_mul_dir(const mat4f& m, const vec3f& v)
	{
		vec3f result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];

		return result;
	}

	//---------------------------------------------------
	//
	//---------------------------------------------------
	float determinant() const
	{
		const float s[6] = {
		    data[0][0] * data[1][1] - data[1][0] * data[0][1], data[0][0] * data[1][2] - data[1][0] * data[0][2],
		    data[0][0] * data[1][3] - data[1][0] * data[0][3], data[0][1] * data[1][2] - data[1][1] * data[0][2],
		    data[0][1] * data[1][3] - data[1][1] * data[0][3], data[0][2] * data[1][3] - data[1][2] * data[0][3],
		};

		const float c[6] = {
		    data[2][0] * data[3][1] - data[3][0] * data[2][1], data[2][0] * data[3][2] - data[3][0] * data[2][2],
		    data[2][0] * data[3][3] - data[3][0] * data[2][3], data[2][1] * data[3][2] - data[3][1] * data[2][2],
		    data[2][1] * data[3][3] - data[3][1] * data[2][3], data[2][2] * data[3][3] - data[3][2] * data[2][3],
		};

		const float det = s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0];
		return det;
	}

	friend float determinant(const mat4f& m) { return m.determinant(); }


	//---------------------------------------------------
	// inverse
	//---------------------------------------------------
	mat4f inverse() const
	{
		const float s[6] = {
		    data[0][0] * data[1][1] - data[1][0] * data[0][1], data[0][0] * data[1][2] - data[1][0] * data[0][2],
		    data[0][0] * data[1][3] - data[1][0] * data[0][3], data[0][1] * data[1][2] - data[1][1] * data[0][2],
		    data[0][1] * data[1][3] - data[1][1] * data[0][3], data[0][2] * data[1][3] - data[1][2] * data[0][3],
		};

		const float c[6] = {
		    data[2][0] * data[3][1] - data[3][0] * data[2][1], data[2][0] * data[3][2] - data[3][0] * data[2][2],
		    data[2][0] * data[3][3] - data[3][0] * data[2][3], data[2][1] * data[3][2] - data[3][1] * data[2][2],
		    data[2][1] * data[3][3] - data[3][1] * data[2][3], data[2][2] * data[3][3] - data[3][2] * data[2][3],
		};

		const float invDet = 1.f / (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

		mat4f result;

		result.data[0][0] = (data[1][1] * c[5] - data[1][2] * c[4] + data[1][3] * c[3]) * invDet;
		result.data[0][1] = (-data[0][1] * c[5] + data[0][2] * c[4] - data[0][3] * c[3]) * invDet;
		result.data[0][2] = (data[3][1] * s[5] - data[3][2] * s[4] + data[3][3] * s[3]) * invDet;
		result.data[0][3] = (-data[2][1] * s[5] + data[2][2] * s[4] - data[2][3] * s[3]) * invDet;

		result.data[1][0] = (-data[1][0] * c[5] + data[1][2] * c[2] - data[1][3] * c[1]) * invDet;
		result.data[1][1] = (data[0][0] * c[5] - data[0][2] * c[2] + data[0][3] * c[1]) * invDet;
		result.data[1][2] = (-data[3][0] * s[5] + data[3][2] * s[2] - data[3][3] * s[1]) * invDet;
		result.data[1][3] = (data[2][0] * s[5] - data[2][2] * s[2] + data[2][3] * s[1]) * invDet;

		result.data[2][0] = (data[1][0] * c[4] - data[1][1] * c[2] + data[1][3] * c[0]) * invDet;
		result.data[2][1] = (-data[0][0] * c[4] + data[0][1] * c[2] - data[0][3] * c[0]) * invDet;
		result.data[2][2] = (data[3][0] * s[4] - data[3][1] * s[2] + data[3][3] * s[0]) * invDet;
		result.data[2][3] = (-data[2][0] * s[4] + data[2][1] * s[2] - data[2][3] * s[0]) * invDet;

		result.data[3][0] = (-data[1][0] * c[3] + data[1][1] * c[1] - data[1][2] * c[0]) * invDet;
		result.data[3][1] = (data[0][0] * c[3] - data[0][1] * c[1] + data[0][2] * c[0]) * invDet;
		result.data[3][2] = (-data[3][0] * s[3] + data[3][1] * s[1] - data[3][2] * s[0]) * invDet;
		result.data[3][3] = (data[2][0] * s[3] - data[2][1] * s[1] + data[2][2] * s[0]) * invDet;

		return result;
	}

	friend mat4f inverse(const mat4f& m) { return m.inverse(); }

	//----------------------------------------------------------------
	// Extracting and decompising the matrix
	// Assuming mat*vec multiplication order (column major)
	//----------------------------------------------------------------
	vec3f extractUnsignedScalingVector() const
	{
		vec3f res;
		res.x = c0.xyz().length();
		res.y = c1.xyz().length();
		res.z = c2.xyz().length();
		return res;
	}

	mat4f removedScaling() const
	{
		const float axisXLenSqr = data[0].lengthSqr();
		const float axisYLenSqr = data[1].lengthSqr();
		const float axisZLenSqr = data[2].lengthSqr();
		const float scaleX = 1e-6f >= 0 ? invSqrt(axisXLenSqr) : 1;
		const float scaleY = 1e-6f >= 0 ? invSqrt(axisYLenSqr) : 1;
		const float scaleZ = 1e-6f >= 0 ? invSqrt(axisZLenSqr) : 1;

		mat4f result = *this;

		result.data[0][0] *= scaleX;
		result.data[0][1] *= scaleX;
		result.data[0][2] *= scaleX;

		result.data[1][0] *= scaleY;
		result.data[1][1] *= scaleY;
		result.data[1][2] *= scaleY;

		result.data[2][0] *= scaleZ;
		result.data[2][1] *= scaleZ;
		result.data[2][2] *= scaleZ;

		return result;
	}

	vec3f extractTranslation() const { return c3.xyz(); }

	//----------------------------------------------------------------
	// 3d translation
	//----------------------------------------------------------------
	static mat4f getTranslation(const float tx, const float ty, const float tz)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0);
		result.data[1] = vec4f::getAxis(1);
		result.data[2] = vec4f::getAxis(2);
		result.data[3] = vec4f(tx, ty, tz, 1.f);

		return result;
	}

	static mat4f getTranslation(const vec3f& t)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0);
		result.data[1] = vec4f::getAxis(1);
		result.data[2] = vec4f::getAxis(2);
		result.data[3] = vec4f(t.x, t.y, t.z, 1.f);

		return result;
	}

	static mat4f getScaling(const float s)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0, s);
		result.data[1] = vec4f::getAxis(1, s);
		result.data[2] = vec4f::getAxis(2, s);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	static mat4f getScaling(const float sx, const float sy, const float sz)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0, sx);
		result.data[1] = vec4f::getAxis(1, sy);
		result.data[2] = vec4f::getAxis(2, sz);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	static mat4f getScaling(const vec3f& s)
	{
		mat4f result;

		result.data[0] = vec4f::getAxis(0, s.data[0]);
		result.data[1] = vec4f::getAxis(1, s.data[1]);
		result.data[2] = vec4f::getAxis(2, s.data[2]);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	// Imagine that we are transforming a cube with side size = a
	// Transforming that cube with that matrix with scale X and Z axes equaly, based on the specified Y scaling,
	// so the volume of that cube will remain the same after the scaling.
	static mat4f getSquashyScalingY(const float sy, float sxzScale = 1.f)
	{
		const float sxz = (sy > 1e-6f) ? sqrt(1.f / sy) * sxzScale + (1.f - sxzScale) : 0.f;
		return mat4f::getScaling(sxz, sy, sxz);
	}

	//----------------------------------------------------------------
	// X rotation matrix
	//----------------------------------------------------------------
	static mat4f getRotationX(const float angle)
	{
		mat4f result;

		float s, c;
		SinCos(angle, s, c);

		result.data[0] = vec4f::getAxis(0);
		result.data[1] = vec4f(0, c, s, 0);
		result.data[2] = vec4f(0, -s, c, 0);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Y rotation matrix
	//----------------------------------------------------------------
	static mat4f getRotationY(const float angle)
	{
		mat4f result;

		float s, c;
		SinCos(angle, s, c);

		result.data[0] = vec4f(c, 0, -s, 0);
		result.data[1] = vec4f::getAxis(1);
		result.data[2] = vec4f(s, 0, c, 0);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Z rotation matrix
	//----------------------------------------------------------------
	static mat4f getRotationZ(const float angle)
	{
		mat4f result;

		float s, c;
		SinCos(angle, s, c);

		result.data[0] = vec4f(c, s, 0, 0);
		result.data[1] = vec4f(-s, c, 0, 0);
		result.data[2] = vec4f::getAxis(2);
		result.data[3] = vec4f::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Quaternion rotation
	// http://renderfeather.googlecode.com/hg-history/034a1900d6e8b6c92440382658d2b01fc732c5de/Doc/optimized%20Matrix%20quaternion%20conversion.pdf
	//----------------------------------------------------------------
	static mat4f getRotationQuat(const quatf& q)
	{
		mat4f result;

		const float x = q[0];
		const float y = q[1];
		const float z = q[2];
		const float w = q[3];

#if 0
		result.data[0] = vec4f(1 - 2.0*(y*y + z*z), 2.0*(x*y + w*z), 2.0*(x*z - w*y), 0);
		result.data[1] = vec4f(2.0*(x*y - w*z), 1 - 2.0*(x*x + z*z), 2.0*(y*z + w*x), 0);
		result.data[2] = vec4f(2.0*(x*z + w*y), 2.0*(y*z - w*x), 1 - 2.0*(x*x + y*y), 0);
		result.data[3] = vec4f(0, 0, 0, 1);
#else
		const float s = 2.f / q.length();

		result.data[0] = vec4f(1 - s * (y * y + z * z), s * (x * y + w * z), s * (x * z - w * y), 0);
		result.data[1] = vec4f(s * (x * y - w * z), 1 - s * (x * x + z * z), s * (y * z + w * x), 0);
		result.data[2] = vec4f(s * (x * z + w * y), s * (y * z - w * x), 1 - s * (x * x + y * y), 0);
		result.data[3] = vec4f(0, 0, 0, 1);
#endif

		return result;
	}

	// Todo: Propery implement this as it is commonly used thing!
	static mat4f getAxisRotation(const vec3f& axis, const float angle) { return getRotationQuat(quatf::getAxisAngle(axis, angle)); }

	// [TODO] Optimize this.
	static mat4f getTRS(const vec3f& translation, const quatf& rotQuat, const vec3f& scaling)
	{
		return getTranslation(translation) * getRotationQuat(rotQuat) * getScaling(scaling);
	}

	//----------------------------------------------------------------
	// Orthographic projection
	//----------------------------------------------------------------
	static mat4f getOrthoRH(
	    const float left, const float right, const float top, const float bottom, const float nearZ, const float farZ, const bool d3dStyle)
	{
		if (d3dStyle) {
			mat4f result;
			result.data[0] = vec4f(2.f / (right - left), 0.f, 0.f, 0.f);
			result.data[1] = vec4f(0.f, 2.f / (top - bottom), 0.f, 0.f);
			result.data[2] = vec4f(0.f, 0.f, 1.f / (nearZ - farZ), 0.f);
			result.data[3] = vec4f((left + right) / (left - right), (top + bottom) / (bottom - top), nearZ / (nearZ - farZ), 1.f);

			return result;
		}
		else {
			mat4f result;

			result.data[0] = vec4f(2.f / (right - left), 0.f, 0.f, 0.f);
			result.data[1] = vec4f(0.f, 2.f / (top - bottom), 0.f, 0.f);
			result.data[2] = vec4f(0.f, 0.f, -2.f / (farZ - nearZ), 0.f);
			result.data[3] =
			    vec4f(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(farZ + nearZ) / (farZ - nearZ), 1.f);

			return result;
		}
	}

	static mat4f getOrthoRH(float width, float height, float nearZ, float farZ, const bool d3dStyle)
	{
		return getOrthoRH(0, width, 0, height, nearZ, farZ, d3dStyle);
	}

	static mat4f getOrthoRHCentered(float width, float height, float nearZ, float farZ, const bool d3dStyle)
	{
		width *= 0.5f;
		height *= 0.5f;
		return getOrthoRH(-width, width, height, -height, nearZ, farZ, d3dStyle);
	}

	//----------------------------------------------------------------
	// Perspective projection
	//----------------------------------------------------------------
	static mat4f getPerspectiveOffCenterRH_DirectX(float l, float r, float b, float t, float zn, float zf)
	{
		mat4f result = mat4f::getZero();

		result.data[0][0] = 2.f * zn / (r - l);

		result.data[1][1] = 2.f * zn / (t - b);

		result.data[2][0] = (l + r) / (r - l);
		result.data[2][1] = (t + b) / (t - b);
		result.data[2][2] = zf / (zn - zf);
		result.data[2][3] = -1.f;

		result.data[3][2] = zn * zf / (zn - zf);

		return result;
	}

	static mat4f getPerspectiveOffCenterRH_OpenGL(float l, float r, float b, float t, float zn, float zf)
	{
		mat4f result = mat4f::getZero();

		result.data[0][0] = 2.f * zn / (r - l);

		result.data[1][1] = 2.f * zn / (t - b);

		result.data[2][0] = (l + r) / (r - l);
		result.data[2][1] = (t + b) / (t - b);
		result.data[2][2] = -(zf + zn) / (zf - zn);
		result.data[2][3] = -1.f;

		result.data[3][2] = -(2.f * zf * zn) / (zf - zn);

		return result;
	}

	static mat4f getPerspectiveOffCenterRH(float left, float right, float bottom, float top, float nearZ, float farZ, const bool d3dStyle)
	{
		if (d3dStyle)
			return getPerspectiveOffCenterRH_DirectX(left, right, bottom, top, nearZ, farZ);
		else
			return getPerspectiveOffCenterRH_OpenGL(left, right, bottom, top, nearZ, farZ);
	}

	static mat4f getPerspectiveFovRH_DirectX(
	    const float verticalFieldOfView, const float ratioWidthByHeight, const float nearZ, const float farZ, const float heightShift)
	{
		float SinFov;
		float CosFov;
		SinCos(0.5f * verticalFieldOfView, SinFov, CosFov);

		float Height = (SinFov * nearZ) / CosFov;
		float Width = ratioWidthByHeight * Height;

		return getPerspectiveOffCenterRH_DirectX(-Width, Width, -Height + heightShift, Height + heightShift, nearZ, farZ);
	}

	static mat4f getPerspectiveFovRH_OpenGL(
	    const float verticalFieldOfView, const float ratioWidthByHeight, const float nearZ, const float farZ, const float heightShift)
	{
		float SinFov;
		float CosFov;
		SinCos(0.5f * verticalFieldOfView, SinFov, CosFov);

		float Height = (SinFov * nearZ) / CosFov;
		float Width = ratioWidthByHeight * Height;

		return getPerspectiveOffCenterRH_OpenGL(-Width, Width, -Height + heightShift, Height + heightShift, nearZ, farZ);
	}

	static mat4f getPerspectiveFovRH(
	    const float fov, const float aspect, const float nearZ, const float farZ, const float heightShift, const bool d3dStyle)
	{
		if (d3dStyle)
			return getPerspectiveFovRH_DirectX(fov, aspect, nearZ, farZ, heightShift);
		else
			return getPerspectiveFovRH_OpenGL(fov, aspect, nearZ, farZ, heightShift);
	}

	//----------------------------------------------------------------
	// look at view d3d right-handed
	//----------------------------------------------------------------
	static mat4f getLookAtRH(const vec3f& eye, const vec3f& at, const vec3f& up)
	{
		mat4f result = mat4f::getIdentity();

		vec3f f = normalized(at - eye);
		vec3f u = normalized(up);
		vec3f s = normalized(cross(f, u));
		u = normalized(cross(s, f));

		result.at(0, 0) = s.x;
		result.at(0, 1) = s.y;
		result.at(0, 2) = s.z;

		result.at(1, 0) = u.x;
		result.at(1, 1) = u.y;
		result.at(1, 2) = u.z;

		result.at(2, 0) = -f.x;
		result.at(2, 1) = -f.y;
		result.at(2, 2) = -f.z;

		result.at(0, 3) = -dot(s, eye);
		result.at(1, 3) = -dot(u, eye);
		result.at(2, 3) = dot(f, eye);

		return result;
	}

	/// Converts an assumed orthogonal matrix with no scaling to a quaternion.
	quatf toQuat() const
	{
#if 0
		// See for more details :
		// https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

		const bool isZeroRotationMatrix = data[0].lengthSqr() < 1e-6f || data[1].lengthSqr() < 1e-6f || data[2].lengthSqr() < 1e-6f;

		if (isZeroRotationMatrix) {
			return quatf::getIdentity();
		}

		const DATA_TYPE mtxTrace = data[0][0] + data[1][1] + data[2][2];

		if (mtxTrace > 0) {
			quatf result;

			const DATA_TYPE invSqrtScaling = DATA_TYPE(1.0) / sqrt(mtxTrace + DATA_TYPE(1.0));
			const DATA_TYPE s = DATA_TYPE(0.5) * invSqrtScaling;

			result.x = (data[1][2] - data[2][1]) * s;
			result.y = (data[2][0] - data[0][2]) * s;
			result.z = (data[0][1] - data[1][0]) * s;
			result.w = 0.5f * (DATA_TYPE(1.0) / invSqrtScaling);

			return result;
		} else {
			quatf result;

			int i = 0;

			if (data[1][1] > data[0][0]) {
				i = 1;
			}

			if (data[2][2] > data[i][i]) {
				i = 2;
			}

			const int nextAxis[3] = {1, 2, 0};
			const int j = nextAxis[i];
			const int k = nextAxis[j];

			DATA_TYPE s = data[i][i] - data[j][j] - data[k][k] + DATA_TYPE(1.0);

			const DATA_TYPE invSqrtScaling = DATA_TYPE(1.0) / sqrt(s);

			DATA_TYPE qt[4];
			qt[3] = DATA_TYPE(0.5) * (DATA_TYPE(1.0) / invSqrtScaling);

			s = DATA_TYPE(0.5) * invSqrtScaling;

			qt[i] = (data[j][k] - data[k][j]) * s;
			qt[j] = (data[i][j] + data[j][i]) * s;
			qt[k] = (data[i][k] + data[k][i]) * s;

			result.x = qt[0];
			result.y = qt[1];
			result.z = qt[2];
			result.w = qt[3];

			return result;
		}
#else
		quatf result;

		result.w = sqrt(maxOf<float>(0.f, 1.f + data[0][0] + data[1][1] + data[2][2])) * 0.5f;
		result.x = sqrt(maxOf<float>(0.f, 1.f + data[0][0] - data[1][1] - data[2][2])) * 0.5f;
		result.y = sqrt(maxOf<float>(0.f, 1.f - data[0][0] + data[1][1] - data[2][2])) * 0.5f;
		result.z = sqrt(maxOf<float>(0.f, 1.f - data[0][0] - data[1][1] + data[2][2])) * 0.5f;
		result.x = std::copysign(result.x, data[1][2] - data[2][1]);
		result.y = std::copysign(result.y, data[2][0] - data[0][2]);
		result.z = std::copysign(result.z, data[0][1] - data[1][0]);

		return result;
#endif
	}
};

} // namespace sge
