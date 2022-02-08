#pragma once

#include "common.h"
#include "math_base.h"
#include "vec3f.h"

namespace sge {

struct vec4f {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};

	vec4f() = default;

	explicit vec4f(const float s)
	{
		data[0] = s;
		data[1] = s;
		data[2] = s;
		data[3] = s;
	}

	vec4f(const float _x, const float _y, const float _z, const float _w)
	    : x(_x)
	    , y(_y)
	    , z(_z)
	    , w(_w)
	{
	}

	vec4f(const vec2f& xy, const vec2f& zw)
	    : x(xy[0])
	    , y(xy[1])
	    , z(zw[0])
	    , w(zw[1])
	{
	}

	vec4f(const vec2f& xy, const float z, const float w)
	    : x(xy[0])
	    , y(xy[1])
	    , z(z)
	    , w(w)
	{
	}

	vec4f(const float x, const float y, const vec2f& zw)
	    : x(x)
	    , y(y)
	    , z(zw[0])
	    , w(zw[1])
	{
	}

	vec4f(const float _x, const vec3f& v)
	    : x(_x)
	    , y(v[0])
	    , z(v[1])
	    , w(v[2])
	{
	}

	vec4f(const vec3f& v, const float _w)
	    : x(v[0])
	    , y(v[1])
	    , z(v[2])
	    , w(_w)
	{
	}

	void setXyz(const vec3f& v3)
	{
		data[0] = v3.data[0];
		data[1] = v3.data[1];
		data[2] = v3.data[2];
	}

	vec3f xyz() const { return vec3f(data[0], data[1], data[2]); }
	vec3f wyz() const { return vec3f(w, y, z); }
	vec3f xwz() const { return vec3f(x, w, z); }
	vec3f xyw() const { return vec3f(x, y, w); }
	vec2f xy() const { return vec2f(data[0], data[1]); }

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec4f getAxis(const unsigned int& axisIndex, const float axisLen = 1.f)
	{
		vec4f result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec4f getZero() { return vec4f(0); }

	// Less and greater operators.
	friend bool operator<(const vec4f& a, const vec4f& b)
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

	friend bool operator<=(const vec4f& a, const vec4f& b)
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

	friend bool operator>(const vec4f& a, const vec4f& b)
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

	friend bool operator>=(const vec4f& a, const vec4f& b)
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
	float& operator[](const int t)
	{
		sgeAssert(t >= 0 && t < 4);
		return data[t];
	}

	const float operator[](const int t) const
	{
		sgeAssert(t >= 0 && t < 4);
		return data[t];
	}

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec4f& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]) && (data[3] == v[3]); }

	bool operator!=(const vec4f& v) const { return !((*this) == v); }

	// Unary operators - +
	vec4f operator-() const
	{
		vec4f result;

		result[0] = -data[0];
		result[1] = -data[1];
		result[2] = -data[2];
		result[3] = -data[3];

		return result;
	}

	vec4f operator+() const { return *this; }

	// vec + vec
	vec4f& operator+=(const vec4f& v)
	{
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		data[3] += v[3];
		return *this;
	}

	vec4f operator+(const vec4f& v) const
	{
		vec4f r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec4f& operator-=(const vec4f& v)
	{
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		data[3] -= v[3];
		return *this;
	}


	vec4f operator-(const vec4f& v) const
	{
		vec4f r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec4f& operator*=(const float s)
	{
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		data[3] *= s;
		return *this;
	}

	vec4f operator*(const float s) const
	{
		vec4f r(*this);
		r *= s;
		return r;
	}

	friend vec4f operator*(const float s, const vec4f& v) { return v * s; }


	// Vector / Scalar
	vec4f& operator/=(const float s)
	{
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		data[3] /= s;
		return *this;
	}

	vec4f operator/(const float s) const
	{
		vec4f r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec4f& operator*=(const vec4f& v)
	{
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		data[3] *= v.data[3];
		return *this;
	}

	vec4f operator*(const vec4f& s) const
	{
		vec4f r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec4f& operator/=(const vec4f& v)
	{
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		data[3] /= v.data[3];
		return *this;
	}

	vec4f operator/(const vec4f& s) const
	{
		vec4f r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec4f cdiv(const vec4f& a, const vec4f& b)
	{
		vec4f r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		r[3] = a[3] / b[3];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	float hsum() const
	{
		float r = data[0];
		r += data[1];
		r += data[2];
		r += data[3];
		return r;
	}

	friend float hsum(const vec4f& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	float dot(const vec4f& v) const
	{
		float r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];
		r += data[3] * v[3];

		return r;
	}

	friend float dot(const vec4f& a, const vec4f& b) { return a.dot(b); }

	/// Computes the length of the vector.
	float lengthSqr() const { return dot(*this); }

	float length() const
	{
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend float length(const vec4f& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	float volume() const
	{
		float r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);
		r *= abs(data[3]);

		return r;
	}

	/// Returns the normalized vector (with length 1.f).
	/// Assumes that the vector current size is not 0.
	vec4f normalized() const
	{
		const float invLength = float(1.0) / length();

		vec4f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;
		result[3] = data[3] * invLength;

		return result;
	}

	friend vec4f normalized(const vec4f& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1.f).
	/// If the size of the vector is 0, the zero vector is returned.
	vec4f normalized0() const
	{
		float lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec4f(0);
		}

		const float invLength = float(1.0) / sqrt(lenSqr);

		vec4f result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;
		result[3] = data[3] * invLength;

		return result;
	}

	friend vec4f normalized0(const vec4f& v) { return v.normalized0(); }

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec4f speedLerp(const vec4f& a, const vec4f& b, const float speed, const float epsilon = 1e-6f)
	{
		vec4f const diff = b - a;
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
	float distance(const vec4f& other) const { return (*this - other).length(); }

	friend float distance(const vec4f& a, const vec4f& b) { return a.distance(b); }

	/// Rentusn a vector containing min/max components from each vector.
	vec4f pickMin(const vec4f& other) const
	{
		vec4f result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);
		result.data[3] = minOf(data[3], other.data[3]);

		return result;
	}

	vec4f pickMax(const vec4f& other) const
	{
		vec4f result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);
		result.data[3] = maxOf(data[3], other.data[3]);

		return result;
	}

	/// Returns the min/max component in the vector.
	float componentMin() const
	{
		float retval = data[0];

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

		{
			float abs = std::abs(data[3]);
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

		if (data[2] > retval) {
			retval = data[2];
		}

		if (data[3] > retval) {
			retval = data[3];
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

		{
			float abs = std::abs(data[3]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec4f getSorted() const
	{
		vec4f retval = *this;

		for (int t = 0; t < 4; ++t)
			for (int k = t + 1; k < 4; ++k) {
				if (retval.data[t] > retval.data[k]) {
					float x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}
};

} // namespace sge
