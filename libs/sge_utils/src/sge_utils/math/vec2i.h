#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan

namespace sge {

struct vec2i {
	union {
		struct {
			int x, y;
		};
		int data[2];
	};

	vec2i() = default;

	explicit vec2i(const int s)
	{
		for (unsigned int t = 0; t < 2; ++t) {
			data[t] = s;
		}
	}

	vec2i(const int _x, const int _y)
	    : x(_x)
	    , y(_y)
	{
	}

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec2i getAxis(const unsigned int& axisIndex, const int axisLen = 1)
	{
		vec2i result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	static vec2i getY(const int axisLen = 1)
	{
		vec2i result(0);
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec2i getZero() { return vec2i(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const int* const pData)
	{
		data[0] = pData[0];
		data[1] = pData[1];
	}

	// Less and greater operators.
	friend bool operator<(const vec2i& a, const vec2i& b)
	{
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;

		return false;
	}

	friend bool operator<=(const vec2i& a, const vec2i& b)
	{
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;

		return false;
	}


	friend bool operator>(const vec2i& a, const vec2i& b)
	{
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;

		return false;
	}

	friend bool operator>=(const vec2i& a, const vec2i& b)
	{
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
			return true;

		return false;
	}

	// Indexing operators.
	int& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 2);
		return data[t];
	}
	const int operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 2);
		return data[t];
	}

	bool operator==(const vec2i& v) const { return (data[0] == v[0]) && (data[1] == v[1]); }
	bool operator!=(const vec2i& v) const { return !((*this) == v); }

	// Unary operators - +
	vec2i operator-() const
	{
		vec2i result;

		result[0] = -data[0];
		result[1] = -data[1];

		return result;
	}

	vec2i operator+() const { return *this; }

	// vec + vec
	vec2i& operator+=(const vec2i& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		return *this;
	}

	vec2i operator+(const vec2i& v) const
	{
		vec2i r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec2i& operator-=(const vec2i& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		return *this;
	}


	vec2i operator-(const vec2i& v) const
	{
		vec2i r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec2i& operator*=(const int s)
	{
		data[0] *= s;
		data[1] *= s;
		return *this;
	}

	vec2i operator*(const int s) const
	{
		vec2i r(*this);
		r *= s;
		return r;
	}

	friend vec2i operator*(const int s, const vec2i& v) { return v * s; }


	// Vector / Scalar
	vec2i& operator/=(const int s)
	{
		data[0] /= s;
		data[1] /= s;
		return *this;
	}

	vec2i operator/(const int s) const
	{
		vec2i r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec2i& operator*=(const vec2i& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		return *this;
	}

	vec2i operator*(const vec2i& s) const
	{
		vec2i r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec2i& operator/=(const vec2i& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		return *this;
	}

	vec2i operator/(const vec2i& s) const
	{
		vec2i r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec2i cdiv(const vec2i& a, const vec2i& b)
	{
		vec2i r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	int hsum() const
	{
		int r = data[0];
		r += data[1];
		return r;
	}

	friend int hsum(const vec2i& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	int dot(const vec2i& v) const
	{
		int r = data[0] * v[0];
		r += data[1] * v[1];

		return r;
	}

	friend int dot(const vec2i& a, const vec2i& b) { return a.dot(b); }

	/// Computes the length of the vector.
	int lengthSqr() const { return dot(*this); }

	/// Computes the unsigned-volume of a cube.
	int volume() const
	{
		int r = abs(data[0]);
		r *= abs(data[1]);
		return r;
	}

	/// Rentusn a vector containing min/max components from each vector.
	vec2i pickMin(const vec2i& other) const
	{
		vec2i result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);

		return result;
	}

	vec2i pickMax(const vec2i& other) const
	{
		vec2i result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);

		return result;
	}

	/// Returns the min/max component in the vector.
	int componentMin() const
	{
		int retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		return retval;
	}

	int componentMinAbs() const
	{
		int retval = std::abs(data[0]);

		{
			int abs = std::abs(data[1]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	int componentMax() const
	{
		int retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
		}

		return retval;
	}

	int componentMaxAbs() const
	{
		int retval = std::abs(data[0]);

		{
			int abs = std::abs(data[1]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec2i getSorted() const
	{
		vec2i retval;

		retval.data[0] = maxOf(data[0], data[1]);
		retval.data[1] = minOf(data[0], data[1]);

		return retval;
	}
};

} // namespace sge
