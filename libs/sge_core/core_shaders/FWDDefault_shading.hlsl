#include "ShadeCommon.h"

#define PI 3.14159265f

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
#include "lib_skinning.hlsl"
#endif

#include "lib_lighting.hlsl"
#include "lib_textureMapping.hlsl"

float3 linearToSRGB(in float3 linearCol) {
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
	return sRGB;
	
}

//--------------------------------------------------------------------
// Uniforms
//--------------------------------------------------------------------
// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
cbuffer ParamsCbFWDDefaultShading {
	float4x4 projView;
	float4x4 world;
	float4x4 uvwTransform;
	float4 cameraPositionWs;
	float4 uCameraLookDirWs;

	float4 uDiffuseColorTint;
	float3 texDiffuseXYZScaling;
	float uMetalness;

	float uRoughness;
	int uPBRMtlFlags;
	float alphaMultiplier;

	float4 uRimLightColorWWidth;
	float3 uAmbientLightColor;
	float uAmbientFakeDetailAmount;

	// Skinning.
	int uSkinningFirstBoneOffsetInTex; ///< The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.

	ShaderLightData lights[kMaxLights];
	int lightsCnt;
	int lightCnt_padding[3];
};

// Material.
uniform sampler2D texDiffuse;
uniform sampler2D texDiffuseX;
uniform sampler2D texDiffuseY;
uniform sampler2D texDiffuseZ;
uniform sampler2D uTexNormalMap;
uniform sampler2D uTexMetalness;
uniform sampler2D uTexRoughness;

// Shadows.
uniform sampler2D uLightShadowMap[kMaxLights];

// Skinning.
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
uniform sampler2D uSkinningBones;
#endif

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
float2 directionToUV_Spherical(float3 dir) {
	const float2 invAtan = float2(0.1591, 0.3183);
	float2 uv = float2(atan2(dir.z, dir.x), asin(-dir.y));
	uv *= invAtan;
	uv += float2(0.5, 0.5);
	return uv;
}

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct IAVertex {
	float3 a_position : a_position;
	float3 a_normal : a_normal;

#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	float3 a_tangent : a_tangent;
	float3 a_binormal : a_binormal;
#endif

#if OPT_HasUV == kHasUV_Yes
	float2 a_uv : a_uv;
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	float4 a_color : a_color;
#endif

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	int4 a_bonesIds : a_bonesIds;
	float4 a_bonesWeights : a_bonesWeights;
#endif
};

struct StageVertexOut {
	float4 SV_Position : SV_Position;
	float3 v_posWS : v_posWS;

#if OPT_HasUV == kHasUV_Yes
	float2 v_uv : v_uv;
#endif

	float3 v_normal : v_normal;
#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	float3 v_tangent : v_tangent;
	float3 v_binormal : v_binormal;
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	float4 v_vertexDiffuse : v_vertexDiffuse;
#endif
};

StageVertexOut vsMain(IAVertex vsin) {
	StageVertexOut res;

	float3 vertexPosOs = vsin.a_position;
	float3 normalOs = vsin.a_normal;

	// If there is a skinning avilable apply it to the vertex in object space.
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	float4x4 skinMtx = libSkining_getSkinningTransform(vsin.a_bonesIds, uSkinningFirstBoneOffsetInTex, vsin.a_bonesWeights, uSkinningBones);
	vertexPosOs = mul(skinMtx, float4(vertexPosOs, 1.0)).xyz;
	normalOs = mul(skinMtx, float4(normalOs, 0.0)).xyz; // TODO: Proper normal transfrom by inverse transpose.
#endif

	// Pass the varyings to the next shader.
	float4 worldPos = mul(world, float4(vertexPosOs, 1.0));
	const float4 worldNormal = mul(world, float4(normalOs, 0.0)); // TODO: Proper normal transfrom by inverse transpose.

	float4 worldPosNonDistorted = worldPos;

	res.v_posWS = worldPosNonDistorted.xyz;
	res.SV_Position = mul(projView, worldPos);

	res.v_normal = worldNormal.xyz;
#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	res.v_tangent = mul(world, float4(vsin.a_tangent, 0.0)).xyz;
	res.v_binormal = mul(world, float4(vsin.a_binormal, 0.0)).xyz;
#endif

#if (OPT_HasUV == kHasUV_Yes)
	res.v_uv = mul(uvwTransform, float4(vsin.a_uv, 0.0, 1.0)).xy;
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	res.v_vertexDiffuse = vsin.a_color;
#endif

	return res;
}

//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
#ifdef SGE_PIXEL_SHADER
float4 psMain(StageVertexOut inVert)
    : SV_Target0 {
	MaterialSample mtlSample;

	mtlSample.albedo = uDiffuseColorTint;

	if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromConstantColor) {
		// We already use uDiffuseColorTint. So nothing to do here.
	} else if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromTexture) {
#if (OPT_HasUV == kHasUV_Yes)
		mtlSample.albedo *= tex2D(texDiffuse, inVert.v_uv); // TODO: srgb to linear.
#else                                                       // Should never happen.
		return float4(1.f, 0.f, 1.f, 1.f);
#endif
	} else if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromVertexColor) {
#if OPT_HasVertexColor == kHasVertexColor_Yes
		mtlSample.albedo = inVert.v_vertexDiffuse;
#else // Should never happen.
		return float4(1.f, 0.f, 1.f, 1.f);
#endif
	}

#if OPT_HasVertexColor == kHasVertexColor_Yes
	if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseTintByVertexColor) {
		mtlSample.albedo *= inVert.v_vertexDiffuse;
	}
#endif

	if (uPBRMtlFlags & kPBRMtl_Flags_HasNormalMap) {
#if (OPT_HasUV == kHasUV_Yes) && (OPT_HasTangentSpace == kHasTangetSpace_Yes)
		const float3 normalMapNorm = 2.f * tex2D(uTexNormalMap, inVert.v_uv).xyz - float3(1.f, 1.f, 1.f);
		const float3 normalFromNormalMap = normalMapNorm.x * normalize(inVert.v_tangent) + normalMapNorm.y * normalize(inVert.v_binormal) +
		                                   normalMapNorm.z * normalize(inVert.v_normal);

		mtlSample.shadeNormalWs = normalize(normalFromNormalMap);
#else
		return float4(1.f, 1.f, 0.f, 1.f);
#endif
	} else {
		mtlSample.shadeNormalWs = normalize(inVert.v_normal);
	}


	// If the fragment is going to be fully transparent discard the sample.
	if (mtlSample.albedo.w <= 0.f || alphaMultiplier <= 0.f) {
		discard;
	}

	mtlSample.hitPointWs = inVert.v_posWS;
	mtlSample.vertexToCameraDirWs = (cameraPositionWs - inVert.v_posWS);

	mtlSample.metallic = uMetalness;
	mtlSample.roughness = uRoughness;

#if OPT_HasUV == kHasUV_Yes
	if (uPBRMtlFlags & kPBRMtl_Flags_HasMetalnessMap) {
		mtlSample.metallic *= tex2D(uTexMetalness, inVert.v_uv).r; // TODO: srgb to linear.
	}

	if ((uPBRMtlFlags & kPBRMtl_Flags_HasRoughnessMap) != 0) {
		mtlSample.roughness *= tex2D(uTexRoughness, inVert.v_uv).r; // TODO: srgb to linear.
	}
#endif

	// Compute the lighting for each light.
	float4 finalColor;
	if (uPBRMtlFlags & kPBRMtl_Flags_NoLighting) {
		finalColor = mtlSample.albedo;
	} else {
		finalColor = float4(0.f, 0.f, 0.f, mtlSample.albedo.w);

		[unroll]
		for (int iLight = 0; iLight < lightsCnt; ++iLight) {
			finalColor.xyz += Light_computeDirectLighting(lights[iLight], uLightShadowMap[iLight], mtlSample, cameraPositionWs);
		}

		// Add ambient lighting.
		// @ambientLightingFake describes the fake detailed ambient lighting,
		// it produces some good results where the geometry is not lit by anything.
		// the amount of how much regular and fake-detailed ambient light we want
		// is controlled by @uAmbientFakeDetailAmount.
		float ambientLightingFake = 0.f;
		ambientLightingFake += 0.25f;
		ambientLightingFake += 0.25f * 0.5f * (mtlSample.shadeNormalWs.y + 1.f);
		ambientLightingFake += 0.25f * (1.f - abs(mtlSample.shadeNormalWs.x));
		ambientLightingFake += 0.25f * (1.f - abs(mtlSample.shadeNormalWs.z)); 

		float ambientLightAmount = lerp(1.f, ambientLightingFake, uAmbientFakeDetailAmount);

		finalColor.xyz += mtlSample.albedo.xyz * uAmbientLightColor * ambientLightAmount;
	} 
	 
	//finalColor.xyz = pow(finalColor.xyz, float3(1.f / 2.2f, 1.f / 2.2f, 1.f / 2.2f));
	//finalColor.xyz = finalColor.xyz / (finalColor.xyz + float3(1.f, 1.f, 1.f));
	//finalColor.xyz = linearToSRGB(finalColor.xyz);

	// Applt the alpha multiplier.
	finalColor.w *= alphaMultiplier;

	return finalColor;
}
#endif
