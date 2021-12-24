#include "ShadeCommon.h"

#define PI 3.14159265f

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
#include "lib_skinning.hlsl"
#endif

#include "lib_lighting.hlsl"
#include "lib_textureMapping.hlsl"

//--------------------------------------------------------------------
// Uniforms
//--------------------------------------------------------------------
// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
cbuffer ParamCB {
	float4x4 projView;
	float4x4 world;
	float4x4 uvwTransform;
	float4 cameraPositionWs;
	float4 uCameraLookDirWs;

	float4 uDiffuseColorTint;
	
	int uHasDiffuseTextures;
	int uIgnoreTranslationForTriplanarSampling;
	int uForceNoLighting;
	float uTriplanarSharpness;

	float3 uAmbientLightColor;
	float uAmbientFakeDetailAmount;

	ShaderLightData lights[kMaxLights];
	int lightsCnt;
	int lightCnt_padding[3];
};

// Material.
uniform sampler2D texDiffuseX;
uniform sampler2D texDiffuseY;
uniform sampler2D texDiffuseZ;

// Shadows.
uniform sampler2D uLightShadowMap[kMaxLights];


//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct IAVertex {
	float3 a_position : a_position;
	float3 a_normal : a_normal;
};

struct StageVertexOut {
	float4 SV_Position : SV_Position;
	float3 v_posWS : v_posWS;
	float3 v_triplanarSamplingPos : v_triplanarSamplingPos;
	float3 v_normal : v_normal;
};

StageVertexOut vsMain(IAVertex vsin) {
	StageVertexOut res;

	float3 vertexPosOs = vsin.a_position;
	float3 normalOs = vsin.a_normal;

	// Pass the varyings to the next shader.
	float4 worldPos = mul(world, float4(vertexPosOs, 1.0));
	const float4 worldNormal = mul(world, float4(normalOs, 0.0)); // TODO: Proper normal transfrom by inverse transpose.

	float4 worldPosNonDistorted = worldPos;

	res.v_posWS = worldPosNonDistorted.xyz;
	
	if(uIgnoreTranslationForTriplanarSampling) {
		float4 vertexNoTransl = mul(world, float4(vertexPosOs, 0.0));
		res.v_triplanarSamplingPos = mul(uvwTransform, vertexNoTransl).xyz;
	} else {
		res.v_triplanarSamplingPos = mul(uvwTransform, worldPos).xyz;
	}
	
	res.SV_Position = mul(projView, worldPos);
	res.v_normal = worldNormal.xyz;

	return res;
}

float3 libTriplanar_computeWeights(float3 normal, float sharpness) {
	float3 weights = abs(normal);
	weights = pow(weights, sharpness);
	const float invWSum = 1.f / (weights.x + weights.y + weights.z);
	weights *= invWSum;
	return weights;
}

float3 libTriplanar_sample(
	float3 position, 
	float3 normal, 
	float sharpness, 
	sampler2D texX, 
	sampler2D texY, 
	sampler2D texZ) {
		
	const float3 triWeights = libTriplanar_computeWeights(normal, sharpness);
	
	const float4 xColor = tex2D(texX, position.yz);
	const float4 yColor = tex2D(texY, position.xz);
	const float4 zColor = tex2D(texZ, position.yx);
	
	float4 triPlanarMixedColor = 
		  xColor * triWeights.x
		+ yColor * triWeights.y
		+ zColor * triWeights.z;
	
	return triPlanarMixedColor;
}

//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
#ifdef SGE_PIXEL_SHADER
float4 psMain(StageVertexOut inVert)
    : SV_Target0 {
	MaterialSample mtlSample;

	mtlSample.albedo = uDiffuseColorTint;
	mtlSample.hitPointWs = inVert.v_posWS;
	mtlSample.shadeNormalWs = normalize(inVert.v_normal);
	mtlSample.vertexToCameraDirWs = (cameraPositionWs - inVert.v_posWS);

	mtlSample.metallic = 0.f;
	mtlSample.roughness = 1.f;

	if (uHasDiffuseTextures) {
		// Triplanar.
		mtlSample.albedo.xyz *= libTriplanar_sample(
			inVert.v_triplanarSamplingPos, 
			mtlSample.shadeNormalWs, 
			uTriplanarSharpness, 
			texDiffuseX, texDiffuseY, texDiffuseZ);
	}

	// Compute the lighting for each light.
	float4 finalColor;
	if (uForceNoLighting) {
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

	return finalColor;
}
#endif
