#ifndef SHADERLIGHTDATA_H
#define SHADERLIGHTDATA_H

#ifdef __cplusplus
	#include "sge_utils/math/mat4f.h"
#endif

#ifdef __cplusplus
	#define float2 sge::vec2f
	#define float3 sge::vec3f
	#define float4 sge::vec4f
	#define float4x4 sge::mat4f
#endif

#define kLightFlg_HasShadowMap 1

// [LIGHTYPE_ENUM_COPY]
// This enum needs to match between shaders and C++.
#define LightType_point 0
#define LightType_directional 1
#define LightType_spot 2

#ifdef __cplusplus
__declspec(align(4))
#endif
    struct ShaderLightData {
	float3 lightPosition; // Used for spot and point lights.
	int lightType;        // TODO: embed this into the flags.

	float3 lightColor;
	int lightFlags; // A set of flags based on kLightFlg_ macros.

	float3 lightDirection;      // Used for directional and spot lights.
	float spotLightAngleCosine; // The cosine of the angle of the spot light.

	float lightShadowRange;
	float lightShadowBias;
	float lightData_padding0; // Padding for easily matching the C++ memory layout.
	float lightData_padding1; // Padding for easily matching the C++ memory layout.

	float4x4 lightShadowMapProjView; // The projection matrix used for shadow mapping. Point light do not use it.
};

#ifdef __cplusplus
static_assert(
    sizeof(ShaderLightData) % sizeof(sge::vec4f) == 0,
    "Keep the size of Light multiple of float4 as it's used in cbuffers!");
#endif

#ifdef __cplusplus
	#undef float2
	#undef float3
	#undef float4
	#undef float4x4
#endif

#endif // SHADERLIGHTDATA_H
