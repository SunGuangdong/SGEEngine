#pragma once

#include "math_base.h"
#include <cmath>

namespace sge {

using std::abs;
using std::copysign;
using std::pow;
using std::sqrt;

using std::acos;
using std::asin;
using std::atan2;
using std::cos;
using std::sin;
using std::tan;

enum Axis {
	axis_x = 0,
	axis_y = 1,
	axis_z = 2,
};

/// @brief An enum indicating axes in 3D with a sign.
/// This is a friendly reminder that these do not match the
/// Cube Map signed axes in OpenGL and D3D.
/// This enum is designed to be an extenstion to @Axis enum.
enum SignedAxis {
	axis_x_pos = 0,
	axis_y_pos = 1,
	axis_z_pos = 2,
	axis_x_neg = 3,
	axis_y_neg = 4,
	axis_z_neg = 5,

	signedAxis_numElements
};

template <typename T = float>
inline T pi(void)
{
	return T(3.141592653589793);
}

template <typename T = float>
inline T half_pi(void)
{
	return T(1.57079632679);
}

template <typename T = float>
inline T two_pi(void)
{
	return T(6.283185307179586);
}

#define sgeHalfPi 1.57079632679f
#define sgePi 3.141592653589793f
#define sge2Pi 6.283185307179586f

template <class T>
inline T deg2rad(const T& degrees)
{
	return (degrees * T(0.01745329251994));
}

template <class T>
inline T rad2deg(const T& radians)
{
	return (radians * T(57.2957795130785));
}

inline float normalizeAngle(float angle)
{
	int const num2Pi = int(angle / sge2Pi);
	angle = angle - num2Pi * sge2Pi;
	if (angle < 0.f)
		angle += sge2Pi;

	return angle;
};

/// Normalized the specified angle to be in [-pi;pi) range.
inline float normalizeAnglePiRange(float angle)
{
	// TODO: Remove these whiles....
	while (angle <= -sgePi)
		angle += sge2Pi;
	while (angle > sgePi)
		angle -= sge2Pi;
	return angle;
}

inline void SinCos(const float& angle, float& rsin, float& rcos)
{
	rsin = std::sin(angle);
	rcos = std::cos(angle);
}

inline void SinCos(const double& angle, double& rsin, double& rcos)
{
	rsin = std::sin(angle);
	rcos = std::cos(angle);
}

inline bool fltcmp(const float A, const float B, const int maxUlps)
{
	static_assert(sizeof(int) == sizeof(float), "");

	int x = reinterpret_cast<const int&>(A);
	if (A < 0.f)
		x = 0x80000000 - x;

	int y = reinterpret_cast<const int&>(B);
	if (B < 0.f)
		x = 0x80000000 - y;

	int diff = (x - y);
	diff = (diff < 0) ? -diff : diff;

	return diff <= maxUlps;
}

inline float sinc(float x, float k)
{
	const float a = pi() * (k * x - 1.f);
	return sin(a) / a;
}

/// https://www.iquilezles.org/www/articles/functions/functions.htm
/// Great for triggering behaviours or making envelopes for music or animation, and for anything that
/// grows fast and then slowly decays. Use k to control the stretching of the function. Btw, its maximum,
/// which is 1, happens at exactly x = 1/k.
inline float expImpulse(float x, float k)
{
	const float h = k * x;
	return h * expf(1.f - h);
}

template <class T>
T minOf(const T& a, const T& b)
{
	return a < b ? a : b;
}

template <class T>
T maxOf(const T& a, const T& b)
{
	return a > b ? a : b;
}

template <class T>
inline T clamp(const T& x, const T& min, const T& max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

inline float clamp01(const float x)
{
	return clamp(x, 0.f, 1.f);
}

inline double clamp01(const double x)
{
	return clamp(x, 0.0, 1.0);
}

/// A version of clamp where the ranges of the clamp don't need to be ordered (@a could be bigger or smaller than @b).
template <class T>
inline T clampUnordered(const T& x, const T& a, const T& b)
{
	const T min = minOf(a, b);
	const T max = maxOf(a, b);
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

template <class T>
inline T lerp(const T& a, const T& b, const float t)
{
	return a * (1.f - t) + b * t;
}

template <class T>
inline T lerp(const T& a, const T& b, const double t)
{
	return a * (1.0 - t) + b * t;
}

/// Moves the value a towards b with @speed units. The value will not get extrapolated.
/// @param [in] speed is the amount of units to be used to move @a towards @b. Must be positive.
/// @param [in] a is the value that is the starting point.
/// @param [in] b is the end point that the function is lerping towards.
/// @param [in] epsilon if the distance between @a and @b is <= than this value b is going to be directly returned.
/// @result is the moved value of a towards b.
inline float speedLerp(const float& a, const float& b, const float speed, const float epsilon = 1e-6f)
{
	float const a_to_b = b - a;
	float const a_to_b_abs = abs(a_to_b);

	// if the two points are close together just return the target point.
	if (a_to_b_abs <= epsilon) {
		return b;
	}

	float k = speed / a_to_b_abs;
	if (k > 1.f) {
		k = 1.f;
	}

	return a + float(k) * a_to_b;
}

template <typename T>
inline bool doesOverlap1D(const T& min0, const T& max0, const T& min1, const T& max1)
{
	return minOf(max0, max1) > maxOf(min0, min1);
}

/// Returns in @outMin and @outMax the overlapping segment of the two input segments defined by (@min0, @max0) and
/// (@min1, @max1). if @outMin > @outMax then there is no overlap.
template <typename T>
inline void overlapSegment1D(const T& min0, const T& max0, const T& min1, const T& max1, T& outMin, T& outMax)
{
	outMax = minOf(max0, max1);
	outMin = maxOf(min0, min1);
}

inline bool isEpsEqual(float a, float b, float eps = 1e-6f)
{
	float diff = b - a;
	return abs(diff) <= eps;
}

inline bool isEpsZero(float a, float eps = 1e-6f)
{
	return abs(a) <= eps;
}

template <class T>
inline T sqr(const T& f)
{
	return f * f;
}

inline float invSqrt(const float f)
{
	return 1.f / sqrt(f);
}

inline double invSqrt(const double f)
{
	return 1.0 / sqrt(f);
}

/// Returns the sign of the specified values. Always returns +1 or -1. +0 is positive.
template <class T>
inline T sign(const T& f)
{
	return (f < (T)(0)) ? -(T)(1) : T(1);
}

/// @brief Returns a sigmoid like remapping of the specified variable.
/// @k is expected to be in [0;1] range.
inline float smoothstep(float k)
{
	return 3.f * k * k - 2.f * k * k * k;
}
/// @brief arccosine where the input argument is clamped in [-1;1] range.
/// Acosf would crash if the input parameter is out of bounds, however when we do maths
/// it sometimes happen due to floating point precision for that value to get slightly out of range.
/// in that case we really do not want to crash.
inline float acosSafe(const float v)
{
	return acos(clamp(v, -1.f, 1.f));
}

} // namespace sge
