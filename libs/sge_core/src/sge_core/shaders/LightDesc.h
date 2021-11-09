#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_core/Camera.h"

namespace sge {

struct transf3d;
struct Frustum;

/// ShadowMapBuildInfo constains the camera (or many if needed) and other settings
/// needed to render the shadow map for the specified light.
struct SGE_CORE_API ShadowMapBuildInfo {
	ShadowMapBuildInfo() = default;

	ShadowMapBuildInfo(RawCamera shadowMapCamera)
	    : shadowMapCamera(shadowMapCamera) {}

	bool isPointLight = false;
	RawCamera shadowMapCamera;
	RawCamera pointLightShadowMapCameras[SignedAxis::signedAxis_numElements];
	float pointLightFarPlaneDistance = 0.f;
};

// [LIGHTYPE_ENUM_COPY]
// This enum needs to match between shaders and C++.
enum LightType : int {
	light_point = 0,
	light_directional = 1,
	light_spot = 2,
};

/// @brief Describes properties of a single light.
struct SGE_CORE_API LightDesc {
	/// True if the light should participate in the shading.
	bool isOn = true;
	/// The lights type.
	LightType type = light_point;
	/// The intensity of the light.
	float intensity = 1.f;
	/// The maximum illumination distance, the light will start fadeing into black as the shaded point gets away form the light source.
	float range = 10.f;
	/// For Spot light only.
	float spotLightAngle = deg2rad(30.f);
	vec3f color = vec3f(1.f);
	/// True if the light should cast shadows mapping.
	bool hasShadows = false;
	/// The resolution of the shadow map image.
	/// Depending on the light type it can be used a bit differently.
	int shadowMapRes = 128;
	float shadowMapBias = 0.0001f;

	/// @brief Returns the settings to be used when building the shadow map for the light.
	/// @return The settings for the shadow map rendering or null if shadows are disabled for this light.
	Optional<ShadowMapBuildInfo> buildShadowMapInfo(const transf3d& lightWs, const Frustum& mainCameraFrustumWs) const;
};

} // namespace sge
