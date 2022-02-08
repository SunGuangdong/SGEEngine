#pragma once

#include "common.h"
#include "math_base.h"
#include "vec3f.h"

namespace sge {

/// A standard quaternion where the 1st 3 numbers (xyz) are the imaginary part
/// and 4th one (w) is the real part.
struct quatf {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};

	quatf() = default;

	quatf(const float sx, const float sy, const float sz, const float sw)
	{
		data[0] = sx;
		data[1] = sy;
		data[2] = sz;
		data[3] = sw;
	}

	quatf(const vec3f& v, float sw)
	{
		data[0] = v[0];
		data[1] = v[1];
		data[2] = v[2];
		w = sw;
	}

	//-----------------------------------------------------------
	static quatf getAxisAngle(const vec3f& axis, const float angle)
	{
		float s, c;
		SinCos(angle * 0.5f, s, c);

		quatf result;

		result[0] = s * axis[0];
		result[1] = s * axis[1];
		result[2] = s * axis[2];
		result[3] = c;

		return result;
	}

	static quatf getIdentity() { return quatf(0, 0, 0, 1.f); }

	quatf conjugate() const { return quatf(-data[0], -data[1], -data[2], data[3]); }

	friend quatf conjugate(const quatf& q) { return q.conjugate(); }

	friend float hsum(const quatf& q) { return q.data[0] + q.data[1] + q.data[2] + q.data[3]; }

	friend quatf cmul(const quatf& a, const quatf& b)
	{
		quatf q;

		q.x = a.x * b.x;
		q.y = a.y * b.y;
		q.z = a.z * b.z;
		q.w = a.w * b.w;

		return q;
	}

	vec3f xyz() const { return vec3f(x, y, z); }

	//-----------------------------------------------------------
	// length
	//-----------------------------------------------------------
	float lengthSqr() const
	{
		float result = 0;

		for (unsigned int t = 0; t < 4; ++t) {
			result += data[t] * data[t];
		}

		return result;
	}

	float length() const
	{
		// [TODO] fix that
		return sqrtf(lengthSqr());
	}

	friend float length(const quatf& q) { return q.length(); }

	//-----------------------------------------------------------
	// normalized
	//-----------------------------------------------------------
	quatf normalized() const
	{
		const float invLength = 1.f / length();

		quatf result;
		for (unsigned t = 0; t < 4; ++t) {
			result[t] = data[t] * invLength;
		}

		return result;
	}

	friend quatf normalized(const quatf& q) { return q.normalized(); }

	//-----------------------------------------------------------
	// inverse
	//-----------------------------------------------------------
	quatf inverse() const
	{
		const float invLensqr = 1.f / lengthSqr();
		return conjugate() * invLensqr;
	}

	friend quatf inverse(const quatf& q) { return q.inverse(); }

	//---------------------------------------------------------
	//
	//---------------------------------------------------------
	bool operator==(const quatf& ref) const
	{
		return data[0] == ref.data[0] && data[1] == ref.data[1] && data[2] == ref.data[2] && data[3] == ref.data[3];
	}

	bool operator!=(const quatf& ref) const { return !((*this) == ref); }

	//-----------------------------------------------------------
	// operator[]
	//-----------------------------------------------------------
	float& operator[](const unsigned int t) { return data[t]; }
	const float operator[](const unsigned int t) const { return data[t]; }

	//-----------------------------------------------------------
	// quatf + quatf
	//-----------------------------------------------------------
	quatf& operator+=(const quatf& v)
	{
		for (unsigned int t = 0; t < 4; ++t) {
			data[t] += v[t];
		}
		return *this;
	}

	quatf operator+(const quatf& v) const
	{
		quatf r;
		for (unsigned int t = 0; t < 4; ++t) {
			r[t] = data[t] + v[t];
		}
		return r;
	}

	//---------------------------------------------------------
	// quatf - quatf
	//---------------------------------------------------------
	quatf& operator-=(const quatf& v)
	{
		for (unsigned int t = 0; t < 4; ++t) {
			data[t] -= v[t];
		}
		return *this;
	}


	quatf operator-(const quatf& v) const
	{
		quatf r;
		for (unsigned int t = 0; t < 4; ++t) {
			r[t] = data[t] - v[t];
		}
		return r;
	}


	//-----------------------------------------------------------
	// Quat * Scalar
	//-----------------------------------------------------------
	quatf operator*=(const float s)
	{
		for (unsigned int t = 0; t < 4; ++t) {
			data[t] *= s;
		}
		return *this;
	}

	quatf operator*(const float s) const
	{
		quatf r;
		for (unsigned int t = 0; t < 4; ++t) {
			r[t] = data[t] * s;
		}
		return r;
	}

	friend quatf operator*(const float s, const quatf& q) { return q * s; }

	quatf operator-() const { return quatf(-data[0], -data[1], -data[2], -data[3]); }

	//-----------------------------------------------------------
	// Quat / Scalar
	//-----------------------------------------------------------
	quatf operator/=(const float s)
	{
		for (unsigned int t = 0; t < 4; ++t) {
			data[t] /= s;
		}
		return *this;
	}

	quatf operator/(const float s) const
	{
		quatf r;
		for (unsigned int t = 0; t < 4; ++t) {
			r[t] = data[t] / s;
		}
		return r;
	}

	friend quatf operator/(const float s, const quatf& q) { return q / s; }

	/// Standard quaternion multiplication like in math.
	/// q2 * q1, will 1st rotate around q1 and then by q2.
	friend quatf operator*(const quatf& p, const quatf& q)
	{
		quatf r;

		quatf v(p[3], -p[2], p[1], p[0]);
		r[0] = hsum(cmul(v, q));

		v = quatf(p[2], p[3], -p[0], p[1]);
		r[1] = hsum(cmul(v, q));

		v = quatf(-p[1], p[0], p[3], p[2]);
		r[2] = hsum(cmul(v, q));

		v = quatf(-p[0], -p[1], -p[2], p[3]);
		r[3] = hsum(cmul(v, q));

		return r;
	}

	/// Transforms the specified point by the quaternion q.
	friend vec3f quat_mul_pos(const quatf& q, const vec3f& p) { return (q * quatf(p, 0) * q.inverse()).xyz(); }
	friend vec3f normalizedQuat_mul_pos(const quatf& q, const vec3f& p) { return (q * quatf(p, 0) * q.conjugate()).xyz(); }

	float dot(const quatf& q) const
	{
		float r = 0;
		for (unsigned int t = 0; t < 4; ++t) {
			r += data[t] * q.data[t];
		}
		return r;
	}

	friend float dot(const quatf& q0, const quatf& q1) { return q0.dot(q1); }

	friend quatf nlerp(const quatf& q1, const quatf& q2, const float t) { return (lerp(q1, q2, t)).normalized(); }

	friend quatf slerp(const quatf& q1, const quatf& q2, const float t)
	{
		float dt = q1.dot(q2);

		if (abs(dt) >= 1.f) {
			return q1;
		}

		quatf q3;
		if (dt < 0.f) {
			dt = -dt;
			q3 = -q2;
		}
		else {
			q3 = q2;
		}

		if (dt < 0.9999f) {
			const float angle = acos(dt);
			const float invs = 1 / sin(angle);
			const float s0 = sin(angle * (1.f - t));
			const float s1 = sin(angle * t);

			return ((q1 * s0 + q3 * s1) * invs).normalized();
		}

		return nlerp(q1, q3, t);
	}

	friend quatf slerpWithSpeed(quatf from, quatf to, const float speed)
	{
		float const f = from.dot(to);
		float const angle = acosf(std::fmin(fabsf(f), 1.f)) * 2.f;

		if (angle < 1e-6f) {
			return to;
		}

		float const t = std::fmin(1.f, speed / angle);
		return slerp(from, to, t);
	};

	vec3f transformDir(const vec3f& p) const
	{
		const quatf pResQuat = *this * quatf(p, 0.f) * this->conjugate();
		return pResQuat.xyz();
	}

	// Retrieves the rotation created by the quaterion q around the normalized axis normal.
	quatf getTwist(const vec3f& normal) const
	{
		// https://stackoverflow.com/questions/3684269/component-of-a-quaternion-rotation-around-an-axis
		const vec3f ra = this->xyz();
		const vec3f p = ra.dot(normal) * normal;

		quatf twist(p.x, p.y, p.z, this->w);
		twist = twist.normalized();

		return twist;
	}

	quatf getSwing(const vec3f& normal) const
	{
		const vec3f ra = this->xyz();
		const vec3f p = ra.dot(normal) * normal;

		quatf twist(p.x, p.y, p.z, this->w);
		twist = twist.normalized();

		quatf swing = *this * twist.conjugate();

		return swing;
	}

	void getTwistAndSwing(const vec3f& normal, quatf& twist, quatf& swing) const
	{
		const vec3f ra = this->xyz();
		const vec3f p = ra.dot(normal) * normal;

		twist = quatf(p.x, p.y, p.z, this->w);
		twist = twist.normalized();

		swing = *this * twist.conjugate();
	}


	/// Computes the quaternion which would rotate the vector @fromNormalized to @toNormalized.
	/// @fallbackAxis is used if vectors are pointing in the oposite directions.
	/// In that case the rotation will be around that axis..
	static quatf fromNormalizedVectors(const vec3f& fromNormalized, const vec3f& toNormalized, const vec3f& fallbackAxis)
	{
		float dotProd = clamp(fromNormalized.dot(toNormalized), -1.f, 1.f);

		if (isEpsEqual(1.f, dotProd)) {
			return quatf::getIdentity();
		}
		else if (dotProd < float(-0.99999)) {
			return quatf::getAxisAngle(fallbackAxis, pi<float>());
		}
		else {
			float angle = acos(dotProd);
			vec3f axis = fromNormalized.cross(toNormalized).normalized0();
			return quatf::getAxisAngle(axis, angle);
		}
	}
};

} // namespace sge
