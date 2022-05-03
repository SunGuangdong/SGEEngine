//
// A file written in C containing commonalities between C++ and HLSL
//
#ifndef SHADECOMMON_H
#define SHADECOMMON_H

#ifdef __cplusplus
	#include "sge_utils/math/mat4f.h"

	#define float2 sge::vec2f
	#define float3 sge::vec3f
	#define float4 sge::vec4f
	#define float4x4 sge::mat4f
#endif

#define kMaxLights 4

// Setting for OPT_HasVertexColor, vertex color can be used as diffuse source or for tinting.
#define kHasVertexColor_No 0
#define kHasVertexColor_Yes 1

// OPT_HasUV
#define kHasUV_No 0
#define kHasUV_Yes 1

// OPT_HasNormals
#define kHasTangetSpace_No 0
#define kHasTangetSpace_Yes 1

// OPT_HasVertexSkinning settings
#define kHasVertexSkinning_No 0
#define kHasVertexSkinning_Yes 1

// OPT_DiffuseTexForAlphaMasking
#define kHasDiffuseTexForAlphaMasking_No 0
#define kHasDiffuseTexForAlphaMasking_Yes 1

#define kLightFlg_HasShadowMap 1

// [LIGHTYPE_ENUM_COPY]
// This enum needs to match between shaders and C++.
#define LightType_point 0
#define LightType_directional 1
#define LightType_spot 2

struct ShaderLightData {
	float4x4 lightShadowMapProjView; // The projection matrix used for shadow mapping. Point light do not use it.

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
};


struct Camera_CBuffer {
	float4x4 projView;
	float4 cameraPositionWs;
};


struct Mesh_CBuffer {
	float4x4 node2world; ///< Transforms the mesh to world space form it's mesh space.

	// Skinning:
	/// The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.
	int uSkinningFirstBoneOffsetInTex;
	int padding0;
	int padding1;
	int padding2;
};

struct Lighting_CBuffer {
	int lightsCnt;
	int padding0;
	int padding1;
	int padding2;

	float3 uAmbientLightColor;
	float uAmbientFakeDetailAmount;

	ShaderLightData lights[kMaxLights];
};

#ifdef __cplusplus
// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
static_assert(
    sizeof(ShaderLightData) % sizeof(sge::vec4f) == 0, "Keep the size multiple of float4 as it's used in cbuffers!");
static_assert(
    sizeof(Camera_CBuffer) % sizeof(sge::vec4f) == 0, "Keep the size multiple of float4 as it's used in cbuffers!");
static_assert(
    sizeof(Mesh_CBuffer) % sizeof(sge::vec4f) == 0, "Keep the size multiple of float4 as it's used in cbuffers!");
static_assert(
    sizeof(Lighting_CBuffer) % sizeof(sge::vec4f) == 0, "Keep the size multiple of float4 as it's used in cbuffers!");
#endif

//------------------------------------------------------------------------------
// Caution:
// HACK: (kind of uncessery)
//
// Material specifics below!
//
// Technically these can be duplicated - one in HLSL and a copy in C++
// However as an attempt at sharing code we do this HACK.
//
// This is material specific and I do not believe that this is the right place for it.
// However for the sake of simplicity why not add materials' cbuffers here and deal with this later?
//------------------------------------------------------------------------------
// 
// // Material flags.
#define kPBRMtl_Flags_HasNormalMap 1
#define kPBRMtl_Flags_HasMetalnessMap 2
#define kPBRMtl_Flags_HasRoughnessMap 4

#define kPBRMtl_Flags_DiffuseFromConstantColor 8
#define kPBRMtl_Flags_DiffuseFromTexture 16
#define kPBRMtl_Flags_DiffuseFromVertexColor 32
#define kPBRMtl_Flags_DiffuseTintByVertexColor 64
#define kPBRMtl_Flags_NoLighting 128
// 
// Forward shading PBR material shader cbuffer data.
struct PBRMaterial_CBuffer {
	float4x4 uvwTransform;
	float4 uDiffuseColorTint;
	
	float alphaMultiplier;
	float uRoughness;
	float uMetalness;

	int uPBRMtlFlags; ///< Flags from the macros starting with kPBRMtl_Flags_.
};


struct FWDShader_CBuffer_Params {
	Camera_CBuffer camera;
	PBRMaterial_CBuffer material;
	Mesh_CBuffer mesh;
	Lighting_CBuffer lighting;
};

#ifdef __cplusplus
static_assert(
    sizeof(PBRMaterial_CBuffer) % sizeof(sge::vec4f) == 0,
    "Keep the size multiple of float4 as it's used in cbuffers!");
static_assert(
    sizeof(FWDShader_CBuffer_Params) % sizeof(sge::vec4f) == 0,
    "Keep the size multiple of float4 as it's used in cbuffers!");
#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#ifdef __cplusplus
	#undef float2
	#undef float3
	#undef float4
	#undef float4x4
#endif

#endif
