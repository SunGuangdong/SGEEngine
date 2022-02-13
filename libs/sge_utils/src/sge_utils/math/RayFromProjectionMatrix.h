#pragma once

#include "sge_utils/math/common.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/math/vec3f.h"

namespace sge {

/// Generates a ray, coresponding to a pixel (specified in "uv" space (0,0) is top left, (1,1) is bottom right) in world space.
/// The generated ray.pos + ray.dir is a point on the near projection plane.
inline Ray rayFromProjectionMatrix(const mat4f& proj, const mat4f& projInv, const mat4f& viewInv, const vec2f& cursorsUV)
{
	bool const isOrthographic = proj.data[2][3] == 0.f;

	const float vx = (2.f * cursorsUV.x - 1.f);
	const float vy = (-2.f * cursorsUV.y + 1.f);

	vec4f pickDirVS = projInv * vec4f(vx, vy, -1.f, 1.f);
	pickDirVS.x = pickDirVS.x / pickDirVS.w;
	pickDirVS.y = pickDirVS.y / pickDirVS.w;
	pickDirVS.z = pickDirVS.z / pickDirVS.w;
	pickDirVS.w = 0.f;

	// Determine the position and the direction of the ray.
	vec3f pickDirWS;
	vec3f rayStartWs;
	if (isOrthographic) {
		rayStartWs = mat_mul_pos(viewInv, vec3f(pickDirVS.x, pickDirVS.y, 0.f));
		pickDirWS = -viewInv.data[2].xyz();
	}
	else {
		rayStartWs = viewInv.data[3].xyz(); // the ray starts at the world position of the camera in perspective.
		pickDirWS = (viewInv * pickDirVS).xyz();
	}

	return Ray(rayStartWs, pickDirWS);
}

#if 0
// Returns a non-normalized ray direction from the origin, that represents
// the "near plane pick-ray" in view space.
inline vec3f rayFromProjectionMatrix(const vec2f& pixelUV, const float verticalFov, const float ratio) {
	const float fovyTan = tan(verticalFov * 0.5f);

	const float p0 = 1.f / (fovyTan * ratio);
	const float p1 = 1.f / (fovyTan);

	const float vx = (2.f * pixelUV.x - 1.f) / p0;
	const float vy = (-2.f * pixelUV.y + 1.f) / p1;

	return vec3f(vx, vy, -1.f); // -1 is the near plane for the RH view. (As the camera is watching along negative Z).
}

// Returns the picked ray in view space.
inline vec3f rayFromProjectionMatrix(
    const int pixelX, const int pixelY, const int width, const int height, const float horizontalFov, const float aspectRatio) {
	return rayFromProjectionMatrix(vec2f((float)pixelX / (float)width, (float)pixelY / (float)height), horizontalFov, aspectRatio);
}
#endif
} // namespace sge
