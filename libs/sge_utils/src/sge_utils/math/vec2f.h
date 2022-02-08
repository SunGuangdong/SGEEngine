#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan

namespace sge {


struct vec2f {
	union {
		struct {
			float x, y;
		};
		float data[2];
	};

	vec2f() = default;

	explicit vec2f(const float s)
	{
		data[0] = s;
		data[1] = s;
	}

	vec2f(const float _x, const float _y)
	    : x(_x)
	    , y(_y)
	{
	}

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec2f getAxis(const unsigned int axisIndex, const float axisLen = 1.f)
	{
		sgeAssert(axisIndex >= 0 && axisIndex < 2);
		vec2f result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec2f getZero() { return vec2f(0); }

	// Less and greater operators.
	friend bool operator<(const vec2f& a, const vec2f& b)
	{
		if (a[0] < b[0])
			return true;

		if (a[1] < b[1])
			return true;

		return false;
	}

	friend bool operator<=(const vec2f& a, const vec2f& b)
	{
		if (a[0] <= b[0])
			return true;

		if (a[1] <= b[1])
			return true;

		return false;
	}


	friend bool operator>(const vec2f& a, const vec2f& b)
	{
		if (a[0] > b[0])
			return true;

		if (a[1] > b[1])
			return true;

		return false;
	}

	friend bool operator>=(const vec2f& a, const vec2f& b)
	{
		if (a[0] >= b[0])
			return true;

		if (a[1] >= b[1])
			return true;

		return false;
	}

	// Indexing operators.
	float& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 2);
		return data[t];
	}

	const float operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 2);
		return data[t];
	}

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec2f& v) const { return (data[0] == v[0]) && (data[1] == v[1]); }

	bool operator!=(const vec2f& v) const { return !((*this) == v); }

	// Unary operators - +
	vec2f operator-() const
	{
		vec2f result;

		result[0] = -data[0];
		result[1] = -data[1];

		return result;
	}

	vec2f operator+() const { return *this; }

	// vec + vec
	vec2f& operator+=(const vec2f& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		return *this;
	}

	vec2f operator+(const vec2f& v) const
	{
		vec2f r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec2f& operator-=(const vec2f& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		return *this;
	}


	vec2f operator-(const vec2f& v) const
	{
		vec2f r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec2f& operator*=(const float s)
	{
		data[0] *= s;
		data[1] *= s;
		return *this;
	}

	vec2f operator*(const float s) const
	{
		vec2f r(*this);
		r *= s;
		return r;
	}

	friend vec2f operator*(const float s, const vec2f& v) { return v * s; }


	// Vector / Scalar
	vec2f& operator/=(const float s)
	{
		data[0] /= s;
		data[1] /= s;
		return *this;
	}

	vec2f operator/(const float s) const
	{
		vec2f r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec2f& operator*=(const vec2f& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		return *this;
	}

	vec2f operator*(const vec2f& s) const
	{
		vec2f r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec2f& operator/=(const vec2f& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		return *this;
	}

	vec2f operator/(const vec2f& s) const
	{
		vec2f r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec2f cdiv(const vec2f& a, const vec2f& b)
	{
		vec2f r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	float hsum() const
	{
		float r = data[0];
		r += data[1];
		return r;
	}

	friend float hsum(const vec2f& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	float dot(const vec2f& v) const
	{
		float r = data[0] * v[0];
		r += data[1] * v[1];

		return r;
	}

	friend float dot(const vec2f& a, const vec2f& b) { return a.dot(b); }

	/// Computes the length of the vector.
	float lengthSqr() const { return dot(*this); }

	float length() const
	{
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend float length(const vec2f& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	float volume() const
	{
		float r = abs(data[0]);
		r *= abs(data[1]);

		return r;
	}

	/// Returns the normalized vector (with length 1).
	/// Assumes that the vector current size is not 0.
	vec2f normalized() const
	{
		const float invLength = 1.f / length();

		vec2f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;

		return result;
	}

	friend vec2f normalized(const vec2f& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1).
	/// If the size of the vector is 0, the zero vector is returned.
	vec2f normalized0() const
	{
		float lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec2f(0);
		}

		const float invLength = 1.f / sqrt(lenSqr);

		vec2f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;

		return result;
	}

	friend vec2f normalized0(const vec2f& v) { return v.normalized0(); }

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec2f speedLerp(const vec2f& a, const vec2f& b, const float speed, const float epsilon = 1e-6f)
	{
		vec2f const diff = b - a;
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

		return a + k * diff;
	}

	float distance(const vec2f& other) const { return (*this - other).length(); }
	friend float distance(const vec2f& a, const vec2f& b) { return a.distance(b); }

	/// Rentusn a vector containing min/max components from each vector.
	vec2f pickMin(const vec2f& other) const
	{
		vec2f result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);

		return result;
	}

	vec2f pickMax(const vec2f& other) const
	{
		vec2f result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);

		return result;
	}

	/// Returns the min/max component in the vector.
	float componentMin() const
	{
		float retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		return retval;
	}

	float componentMinAbs() const
	{
		float retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	float componentMax() const
	{
		float retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
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

		return retval;
	}
};

} // namespace sge
