#pragma once

#include "sge_utils/math/vec3f.h"

namespace sge {

// Hermite cubic spline. See here:
// https://math.stackexchange.com/questions/1270776/how-to-find-tangent-at-any-point-along-a-cubic-hermite-spline?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

// 1D Hermite cubic spline.
inline float hermiteEval(float t, const float p[4]) {
	sgeAssert(t >= 0.f && t <= 1.f);
	const float tt = t * t;
	const float ttt = tt * t;

	const float q1 = -ttt + 2.0f * tt - t;
	const float q2 = 3.0f * ttt - 5.0f * tt + 2.0f;
	const float q3 = -3.0f * ttt + 4.0f * tt + t;
	const float q4 = ttt - tt;

	const float res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

inline float hermiteEvalTanget(float t, const float p[4]) {
	const float tt = t * t;

	float q1 = -3.f * tt + 4.f * t - 1.f;
	float q2 = 9.f * tt - 10.f * t;
	float q3 = -9.f * tt + 8.f * t + 1.f;
	float q4 = 3.f * tt - 2.f * t;

	const float res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

// 2D Hermite cubic spline.
inline vec2f hermiteEval(float t, const vec2f p[4]) {
	sgeAssert(t >= 0.f && t <= 1.f);
	const float tt = t * t;
	const float ttt = tt * t;

	const float q1 = -ttt + 2.0f * tt - t;
	const float q2 = 3.0f * ttt - 5.0f * tt + 2.0f;
	const float q3 = -3.0f * ttt + 4.0f * tt + t;
	const float q4 = ttt - tt;

	const vec2f res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

inline vec2f hermiteEvalTanget(float t, const vec2f p[4]) {
	const float tt = t * t;

	float q1 = -3.f * tt + 4.f * t - 1.f;
	float q2 = 9.f * tt - 10.f * t;
	float q3 = -9.f * tt + 8.f * t + 1.f;
	float q4 = 3.f * tt - 2.f * t;

	const vec2f res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

// 3D Hermite cubic spline.
inline vec3f hermiteEval(float t, const vec3f p[4]) {
	sgeAssert(t >= 0.f && t <= 1.f);
	const float tt = t * t;
	const float ttt = tt * t;

	const float q1 = -ttt + 2.0f * tt - t;
	const float q2 = 3.0f * ttt - 5.0f * tt + 2.0f;
	const float q3 = -3.0f * ttt + 4.0f * tt + t;
	const float q4 = ttt - tt;

	const vec3f res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

inline vec3f hermiteEvalTanget(float t, const vec3f p[4]) {
	const float tt = t * t;

	float q1 = -3.f * tt + 4.f * t - 1.f;
	float q2 = 9.f * tt - 10.f * t;
	float q3 = -9.f * tt + 8.f * t + 1.f;
	float q4 = 3.f * tt - 2.f * t;

	const vec3f res = (p[0] * q1 + p[1] * q2 + p[2] * q3 + p[3] * q4) * 0.5f;
	return res;
}

} // namespace sge
