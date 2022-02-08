#pragma once

#include <cmath> // isnan

#include "common.h"
#include "math_base.h"
#include "vec2f.h"

namespace sge {

struct vec3f {
	union {
		struct {
			float x, y, z;
		};
		float data[3];
	};

	vec3f() = default;

	explicit vec3f(const float s)
	{
		for (unsigned int t = 0; t < 3; ++t) {
			data[t] = s;
		}
	}

	vec3f(const float _x, const float _y, const float _z)
	    : x(_x)
	    , y(_y)
	    , z(_z)
	{
	}

	vec3f(const vec2f& xy, const float _z)
	    : x(xy[0])
	    , y(xy[1])
	    , z(_z)
	{
	}

	vec3f(const float _x, const vec2f& yz)
	    : x(_x)
	    , y(yz[0])
	    , z(yz[1])
	{
	}

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec3f getAxis(const unsigned int& axisIndex, const float axisLen = 1.f)
	{
		vec3f result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	static vec3f axis_x(const float axisLen = 1.f) { return vec3f(axisLen, 0, 0); }

	static vec3f axis_y(const float axisLen = 1.f) { return vec3f(0, axisLen, 0); }

	static vec3f axis_z(const float axisLen = 1.f) { return vec3f(0, 0, axisLen); }

	static vec3f getY(const float axisLen = 1.f)
	{
		vec3f result(0);
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec3f getZero() { return vec3f(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const float* const pData)
	{
		data[0] = pData[0];
		data[1] = pData[1];
		data[2] = pData[2];
	}

	// Less and greater operators.
	friend bool operator<(const vec3f& a, const vec3f& b)
	{
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;
		if (a[2] < b[2])
			return true;

		return false;
	}

	friend bool operator<=(const vec3f& a, const vec3f& b)
	{
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;
		if (a[2] <= b[2])
			return true;

		return false;
	}


	friend bool operator>(const vec3f& a, const vec3f& b)
	{
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;
		if (a[2] > b[2])
			return true;

		return false;
	}

	friend bool operator>=(const vec3f& a, const vec3f& b)
	{
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
			return true;
		if (a[2] >= b[2])
			return true;

		return false;
	}

	// Indexing operators.
	float& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 3);
		return data[t];
	}
	const float operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 3);
		return data[t];
	}

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec3f& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]); }

	bool operator!=(const vec3f& v) const { return !((*this) == v); }

	// Unary operators - +
	vec3f operator-() const
	{
		vec3f result;

		result[0] = -data[0];
		result[1] = -data[1];
		result[2] = -data[2];

		return result;
	}

	vec3f operator+() const { return *this; }

	// vec + vec
	vec3f& operator+=(const vec3f& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		return *this;
	}

	vec3f operator+(const vec3f& v) const
	{
		vec3f r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec3f& operator-=(const vec3f& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		return *this;
	}


	vec3f operator-(const vec3f& v) const
	{
		vec3f r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec3f& operator*=(const float s)
	{
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		return *this;
	}

	vec3f operator*(const float s) const
	{
		vec3f r(*this);
		r *= s;
		return r;
	}

	friend vec3f operator*(const float s, const vec3f& v) { return v * s; }


	// Vector / Scalar
	vec3f& operator/=(const float s)
	{
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		return *this;
	}

	vec3f operator/(const float s) const
	{
		vec3f r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec3f& operator*=(const vec3f& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		return *this;
	}

	vec3f operator*(const vec3f& s) const
	{
		vec3f r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec3f& operator/=(const vec3f& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		return *this;
	}

	vec3f operator/(const vec3f& s) const
	{
		vec3f r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec3f cdiv(const vec3f& a, const vec3f& b)
	{
		vec3f r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	float hsum() const
	{
		float r = data[0];
		r += data[1];
		r += data[2];
		return r;
	}

	friend float hsum(const vec3f& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	float dot(const vec3f& v) const
	{
		float r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];

		return r;
	}

	friend float dot(const vec3f& a, const vec3f& b) { return a.dot(b); }

	/// Computes the length of the vector.
	float lengthSqr() const { return dot(*this); }

	float length() const
	{
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend float length(const vec3f& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	float volume() const
	{
		float r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);

		return r;
	}

	/// Returns the normalized vector (with length 1.f).
	/// Assumes that the vector current size is not 0.
	vec3f normalized() const
	{
		const float invLength = 1.f / length();

		vec3f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;

		return result;
	}

	friend vec3f normalized(const vec3f& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1.f).
	/// If the size of the vector is 0, the zero vector is returned.
	vec3f normalized0() const
	{
		float lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec3f(0);
		}

		const float invLength = 1.f / sqrt(lenSqr);

		vec3f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;

		return result;
	}

	friend vec3f normalized0(const vec3f& v) { return v.normalized0(); }

	vec3f reciprocalSafe(float fallbackIfZero = float(1e-6f)) const
	{
		vec3f v;
		v.data[0] = fabsf(data[0]) > float(1e-6f) ? 1 / data[0] : fallbackIfZero;
		v.data[1] = fabsf(data[1]) > float(1e-6f) ? 1 / data[1] : fallbackIfZero;
		v.data[2] = fabsf(data[2]) > float(1e-6f) ? 1 / data[2] : fallbackIfZero;

		return v;
	}

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec3f speedLerp(const vec3f& a, const vec3f& b, const float speed, const float epsilon = 1e-6f)
	{
		vec3f const diff = b - a;
		float const diffLen = diff.length();

		// if the two points are too close together just return the target point.
		if (diffLen < epsilon) {
			return b;
		}

		// TODO: handle double here.
		float k = float(speed) / float(diffLen);

		if (k > 1.f) {
			k = 1.f;
		}

		return a + float(k) * diff;
	}

	/// Computes the distance between two vectors.
	float distance(const vec3f& other) const { return (*this - other).length(); }

	friend float distance(const vec3f& a, const vec3f& b) { return a.distance(b); }

	/// Retrieves the minimum of each pairs of components by picking the smallest one.
	vec3f pickMin(const vec3f& other) const
	{
		vec3f result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);

		return result;
	}

	/// Retrieves the minimum of each pairs of components by picking the biggest one.
	vec3f pickMax(const vec3f& other) const
	{
		vec3f result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);

		return result;
	}

	/// Returns the min component in the vector.
	float componentMin() const
	{
		float retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		if (data[2] < retval) {
			retval = data[2];
		}

		return retval;
	}

	/// @brief Computes the absolute values of each compoents in the vector and returns the smallest one.
	float componentMinAbs() const
	{
		float retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs < retval) {
				retval = abs;
			}
		}

		{
			float abs = std::abs(data[2]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	/// Returns the max component in the vector.
	float componentMax() const
	{
		float retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
		}

		if (data[2] > retval) {
			retval = data[2];
		}

		return retval;
	}

	float componentMaxAbs() const
	{
		float retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs > retval) {
				retval = abs;
			}
		}

		{
			float abs = std::abs(data[2]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec3f getAbs() const
	{
		vec3f retval = *this;
		retval.x = std::abs(retval.x);
		retval.y = std::abs(retval.y);
		retval.z = std::abs(retval.z);

		return retval;
	}

	vec3f getSorted() const
	{
		vec3f retval = *this;

		for (int t = 0; t < 3; ++t)
			for (int k = t + 1; k < 3; ++k) {
				if (retval.data[t] > retval.data[k]) {
					float x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}

	static vec3f getSignedAxis(const SignedAxis sa)
	{
		switch (sa) {
			case axis_x_pos:
				return vec3f(1, 0, 0);
			case axis_y_pos:
				return vec3f(0, 1, 0);
			case axis_z_pos:
				return vec3f(0, 0, 1);
			case axis_x_neg:
				return vec3f(-1, 0, 0);
			case axis_y_neg:
				return vec3f(0, -1, 0);
			case axis_z_neg:
				return vec3f(0, 0, -1);
		}

		sgeAssert(false);
		return vec3f(float(0));
	}

	//---------------------------------------------------
	// cross product
	//---------------------------------------------------
	vec3f cross(const vec3f& v) const
	{
		const float tx = (y * v.z) - (v.y * z);
		const float ty = (v.x * z) - (x * v.z);
		const float tz = (x * v.y) - (v.x * y);

		return vec3f(tx, ty, tz);
	}

	friend vec3f cross(const vec3f& a, const vec3f& b) { return a.cross(b); }

	//---------------------------------------------------
	// tiple product: dot(v, cross(a,b))
	//---------------------------------------------------
	friend float triple(const vec3f& v, const vec3f& a, const vec3f& b) { return v.dot(a.cross(b)); }


	vec3f anyPerpendicular() const
	{
		vec3f r = vec3f(-y, x, z);
		if (!isEpsZero(r.dot(*this))) {
			r = vec3f(-z, 0, x);
		}
		if (!isEpsZero(r.dot(*this))) {
			r = vec3f(0, -z, y);
		}
		if (!isEpsZero(r.dot(*this))) {
			r = vec3f(-y, x, 0);
		}
		sgeAssert(isEpsZero(r.dot(*this)));
		return r;
	}

	vec2f xz() const { return vec2f(data[0], data[2]); }
	vec2f xy() const { return vec2f(data[0], data[1]); }

	vec3f x00() const { return vec3f(data[0], 0.f, 0.f); }
	vec3f x0z() const { return vec3f(data[0], 0.f, data[2]); }
	vec3f xy0() const { return vec3f(data[0], data[1], 0.f); }

	vec3f xOnly() const { return vec3f(data[0], 0.f, 0.f); }
	vec3f yOnly() const { return vec3f(0.f, data[1], 0.f); }
	vec3f zOnly() const { return vec3f(0.f, 0.f, data[2]); }

	vec3f _0yz() const { return vec3f(0.f, data[1], data[2]); }

	bool hasNan() const { return std::isnan(data[0]) || std::isnan(data[1]) || std::isnan(data[1]); }
};


// Rotates a point @pt around @axisNormalized (witch needs to be normalized) with @angle radians.
// https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
inline vec3f rotateAround(const vec3f& axisNormalized, float angle, const vec3f& pt)
{
	float s, c;
	SinCos(angle, s, c);
	vec3f ptRotated = pt * c + cross(axisNormalized, pt) * s + axisNormalized * (dot(axisNormalized, pt)) * (1.f - c);

	return ptRotated;
}

vec3f rotateTowards(const vec3f& from, const vec3f& to, const float maxRotationAmount, const vec3f& fallbackAxis);

} // namespace sge
