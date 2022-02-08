#pragma once

#include "common.h"
#include "math_base.h"
#include "vec3i.h"

namespace sge {
struct vec4i {
	union {
		struct {
			int x, y, z, w;
		};
		int data[4];
	};

	vec4i() = default;

	explicit vec4i(const int s)
	{
		data[0] = s;
		data[1] = s;
		data[2] = s;
		data[3] = s;
	}

	vec4i(const int _x, const int _y, const int _z, const int _w)
	    : x(_x)
	    , y(_y)
	    , z(_z)
	    , w(_w)
	{
	}

	vec4i(const vec2i& xy, const vec2i& zw)
	    : x(xy[0])
	    , y(xy[1])
	    , z(zw[0])
	    , w(zw[1])
	{
	}

	vec4i(const vec2i& xy, const int z, const int w)
	    : x(xy[0])
	    , y(xy[1])
	    , z(z)
	    , w(w)
	{
	}

	vec4i(const int x, const int y, const vec2i& zw)
	    : x(x)
	    , y(y)
	    , z(zw[0])
	    , w(zw[1])
	{
	}

	vec4i(const int _x, const vec3i& v)
	    : x(_x)
	    , y(v[0])
	    , z(v[1])
	    , w(v[2])
	{
	}

	vec4i(const vec3i& v, const int _w)
	    : x(v[0])
	    , y(v[1])
	    , z(v[2])
	    , w(_w)
	{
	}

	vec3i xyz() const { return vec3i(data[0], data[1], data[2]); }
	vec3i wyz() const { return vec3i(w, y, z); }
	vec3i xwz() const { return vec3i(x, w, z); }
	vec3i xyw() const { return vec3i(x, y, w); }
	vec2i xy() const { return vec2i(data[0], data[1]); }

	// Less and greater operators.
	friend bool operator<(const vec4i& a, const vec4i& b)
	{
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;
		if (a[2] < b[2])
			return true;
		if (a[3] < b[3])
			return true;

		return false;
	}

	friend bool operator<=(const vec4i& a, const vec4i& b)
	{
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;
		if (a[2] <= b[2])
			return true;
		if (a[3] <= b[3])
			return true;

		return false;
	}


	friend bool operator>(const vec4i& a, const vec4i& b)
	{
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;
		if (a[2] > b[2])
			return true;
		if (a[3] > b[3])
			return true;

		return false;
	}

	friend bool operator>=(const vec4i& a, const vec4i& b)
	{
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
			return true;
		if (a[2] >= b[2])
			return true;
		if (a[3] >= b[3])
			return true;

		return false;
	}

	// Indexing operators.
	int& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 4);
		return data[t];
	}
	const int operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 4);
		return data[t];
	}

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec4i& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]) && (data[3] == v[3]); }

	bool operator!=(const vec4i& v) const { return !((*this) == v); }

	// Unary operators - +
	vec4i operator-() const
	{
		vec4i result;

		result[0] = -data[0];
		result[1] = -data[1];
		result[2] = -data[2];
		result[3] = -data[3];

		return result;
	}

	vec4i operator+() const { return *this; }

	// vec + vec
	vec4i& operator+=(const vec4i& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		data[3] += v[3];
		return *this;
	}

	vec4i operator+(const vec4i& v) const
	{
		vec4i r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec4i& operator-=(const vec4i& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		data[3] -= v[3];
		return *this;
	}


	vec4i operator-(const vec4i& v) const
	{
		vec4i r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec4i& operator*=(const int s)
	{
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		data[3] *= s;
		return *this;
	}

	vec4i operator*(const int s) const
	{
		vec4i r(*this);
		r *= s;
		return r;
	}

	friend vec4i operator*(const int s, const vec4i& v) { return v * s; }


	// Vector / Scalar
	vec4i& operator/=(const int s)
	{
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		data[3] /= s;
		return *this;
	}

	vec4i operator/(const int s) const
	{
		vec4i r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec4i& operator*=(const vec4i& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		data[3] *= v.data[3];
		return *this;
	}

	vec4i operator*(const vec4i& s) const
	{
		vec4i r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec4i& operator/=(const vec4i& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		data[3] /= v.data[3];
		return *this;
	}

	vec4i operator/(const vec4i& s) const
	{
		vec4i r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec4i cdiv(const vec4i& a, const vec4i& b)
	{
		vec4i r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		r[3] = a[3] / b[3];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	int hsum() const
	{
		int r = data[0];
		r += data[1];
		r += data[2];
		r += data[3];
		return r;
	}

	friend int hsum(const vec4i& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	int dot(const vec4i& v) const
	{
		int r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];
		r += data[3] * v[3];

		return r;
	}

	friend int dot(const vec4i& a, const vec4i& b) { return a.dot(b); }

	/// Computes the unsigned-volume of a cube.
	int volume() const
	{
		int r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);
		r *= abs(data[3]);

		return r;
	}

	/// Rentusn a vector containing min/max components from each vector.
	vec4i pickMin(const vec4i& other) const
	{
		vec4i result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);
		result.data[3] = minOf(data[3], other.data[3]);

		return result;
	}

	friend vec4i pick_min(const vec4i& a, const vec4i& b) { return a.pickMin(b); }

	vec4i pickMax(const vec4i& other) const
	{
		vec4i result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);
		result.data[3] = maxOf(data[3], other.data[3]);

		return result;
	}

	/// Returns the min/max component in the vector.
	int componentMin() const
	{
		int retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		if (data[2] < retval) {
			retval = data[2];
		}

		if (data[3] < retval) {
			retval = data[3];
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

		{
			int abs = std::abs(data[2]);
			if (abs < retval) {
				retval = abs;
			}
		}

		{
			int abs = std::abs(data[3]);
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

		if (data[2] > retval) {
			retval = data[2];
		}

		if (data[3] > retval) {
			retval = data[3];
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

		{
			int abs = std::abs(data[3]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec4i getSorted() const
	{
		vec4i retval = *this;

		for (int t = 0; t < 4; ++t)
			for (int k = t + 1; k < 4; ++k) {
				if (retval.data[t] > retval.data[k]) {
					int x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}
};

} // namespace sge
