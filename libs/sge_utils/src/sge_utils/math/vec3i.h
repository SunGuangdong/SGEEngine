#pragma once

#include "common.h"
#include "math_base.h"
#include "vec2i.h"

namespace sge {

struct vec3i {
	union {
		struct {
			int x, y, z;
		};
		int data[3];
	};

	vec3i() = default;

	vec3i(const vec3i& ref)
	    : x(ref.x)
	    , y(ref.y)
	    , z(ref.z)
	{
	}

	explicit vec3i(const int& s)
	{
		for (unsigned int t = 0; t < 3; ++t) {
			data[t] = s;
		}
	}

	vec3i(int _x, int _y, int _z)
	    : x(_x)
	    , y(_y)
	    , z(_z)
	{
	}

	vec3i(const vec2i& xy, const int _z)
	    : x(xy[0])
	    , y(xy[1])
	    , z(_z)
	{
	}

	vec3i(int _x, const vec2i& yz)
	    : x(_x)
	    , y(yz[0])
	    , z(yz[1])
	{
	}

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec3i getAxis(const unsigned int axisIndex, const int axisLen = 1)
	{
		vec3i result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	static vec3i axis_x(const int axisLen = 1) { return vec3i(axisLen, 0, 0); }

	static vec3i axis_y(const int axisLen = 1) { return vec3i(0, axisLen, 0); }

	static vec3i axis_z(const int axisLen = 1) { return vec3i(0, 0, axisLen); }

	static vec3i getY(const int axisLen = 1)
	{
		vec3i result(0);
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec3i getZero() { return vec3i(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const int* const pData)
	{
		data[0] = pData[0];
		data[1] = pData[1];
		data[2] = pData[2];
	}

	// Less and greater operators.
	friend bool operator<(const vec3i& a, const vec3i& b)
	{
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;
		if (a[2] < b[2])
			return true;

		return false;
	}

	friend bool operator<=(const vec3i& a, const vec3i& b)
	{
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;
		if (a[2] <= b[2])
			return true;

		return false;
	}


	friend bool operator>(const vec3i& a, const vec3i& b)
	{
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;
		if (a[2] > b[2])
			return true;

		return false;
	}

	friend bool operator>=(const vec3i& a, const vec3i& b)
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
	int& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 3);
		return data[t];
	}
	const int& operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 3);
		return data[t];
	}

	bool operator==(const vec3i& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]); }
	bool operator!=(const vec3i& v) const { return !((*this) == v); }

	// Unary operators - +
	vec3i operator-() const { return vec3i(-data[0], -data[1], -data[2]); }

	vec3i operator+() const { return *this; }

	// vec + vec
	vec3i& operator+=(const vec3i& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		return *this;
	}

	vec3i operator+(const vec3i& v) const
	{
		vec3i r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec3i& operator-=(const vec3i& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		return *this;
	}


	vec3i operator-(const vec3i& v) const
	{
		vec3i r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec3i& operator*=(const int s)
	{
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		return *this;
	}

	vec3i operator*(const int s) const
	{
		vec3i r(*this);
		r *= s;
		return r;
	}

	friend vec3i operator*(const int s, const vec3i& v) { return v * s; }


	// Vector / Scalar
	vec3i& operator/=(const int s)
	{
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		return *this;
	}

	vec3i operator/(const int s) const
	{
		vec3i r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec3i& operator*=(const vec3i& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		return *this;
	}

	vec3i operator*(const vec3i& s) const
	{
		vec3i r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec3i& operator/=(const vec3i& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		return *this;
	}

	vec3i operator/(const vec3i& s) const
	{
		vec3i r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec3i cdiv(const vec3i& a, const vec3i& b)
	{
		vec3i r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	int hsum() const
	{
		int r = data[0];
		r += data[1];
		r += data[2];
		return r;
	}

	friend int hsum(const vec3i& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	int dot(const vec3i& v) const
	{
		int r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];

		return r;
	}

	friend int dot(const vec3i& a, const vec3i& b) { return a.dot(b); }

	/// Computes the length of the vector.
	int lengthSqr() const { return dot(*this); }


	/// Computes the unsigned-volume of a cube.
	int volume() const
	{
		int r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);

		return r;
	}

	/// Retrieves the minimum of each pairs of components by picking the smallest one.
	vec3i pickMin(const vec3i& other) const
	{
		vec3i result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);

		return result;
	}

	/// Retrieves the minimum of each pairs of components by picking the biggest one.
	vec3i pickMax(const vec3i& other) const
	{
		vec3i result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);

		return result;
	}

	/// Returns the min component in the vector.
	int componentMin() const
	{
		int retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		if (data[2] < retval) {
			retval = data[2];
		}

		return retval;
	}

	/// @brief Computes the absolute values of each compoents in the vector and returns the smallest one.
	int componentMinAbs() const
	{
		int retval = std::abs(data[0]);

		{
			int abs = std::abs(data[1]);
			if (abs < retval) {
				retval = abs;
			}
		}

		{
			int abs = std::abs(data[2]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	/// Returns the max component in the vector.
	int componentMax() const
	{
		int retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
		}

		if (data[2] > retval) {
			retval = data[2];
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

		{
			int abs = std::abs(data[2]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec3i getAbs() const
	{
		vec3i retval = *this;
		retval.x = std::abs(retval.x);
		retval.y = std::abs(retval.y);
		retval.z = std::abs(retval.z);

		return retval;
	}

	vec3i getSorted() const
	{
		vec3i retval = *this;

		for (int t = 0; t < 3; ++t)
			for (int k = t + 1; k < 3; ++k) {
				if (retval.data[t] > retval.data[k]) {
					int x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}

	static vec3i getSignedAxis(const SignedAxis sa)
	{
		switch (sa) {
			case axis_x_pos:
				return vec3i(1, 0, 0);
			case axis_y_pos:
				return vec3i(0, 1, 0);
			case axis_z_pos:
				return vec3i(0, 0, 1);
			case axis_x_neg:
				return vec3i(-1, 0, 0);
			case axis_y_neg:
				return vec3i(0, -1, 0);
			case axis_z_neg:
				return vec3i(0, 0, -1);
		}

		sgeAssert(false);
		return vec3i(0);
	}
};


} // namespace sge
