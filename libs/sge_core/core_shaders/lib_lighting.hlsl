#ifndef LIGHTING_SHADER
#define LIGHTING_SHADER

#include "ShadeCommon.h"
#include "lib_pbr.hlsl"
#include "lib_textureMapping.hlsl"

struct MaterialSample {
	float3 hitPointWs;
	float3 shadeNormalWs; ///< The normal in the point that should be used for shading. It might not be the face normal if bump/normal mapping is applied.
	float3 vertexToCameraDirWs; ///< Normalized vector point from @hitPointWs to the camera.

	float4 albedo;
	float metallic;
	float roughness;
};

void MaterialSample_constructor(out MaterialSample mtlSample)
{
	mtlSample.hitPointWs = float3(0.f, 0.f, 0.f);
	mtlSample.shadeNormalWs = float3(0.f, 0.f, -1.f);
	mtlSample.vertexToCameraDirWs = float3(0.f, 0.f, -1.f);

	mtlSample.albedo = float4(0.f, 0.f, 0.f, 1.f);
	mtlSample.metallic = 0.f;
	mtlSample.roughness = 1.f;
}

/// Returns the amount of light at the specified point @positionWs that is reached from @light
/// based on the @lightShadowMap.
/// @param cameraPositionWs is nedeed for direction lights to add their light fading effect near the edges of the shadow map.
/// 0 - fully in shadow. 1 - the light fully illuminates the object.
/// values between (0;1) are a result of filtering and smoothing, This does not produce soft shadows.
float Light_computeShadowMultipler(
	in ShaderLightData light, 
	in sampler2D lightShadowMap, 
	in float3 positionWs,
	in float3 cameraPositionWs)
{
	if (light.lightType == LightType_point) {
		// [FWDDEF_POINTLIGHT_LINEAR_ZDEPTH]
		float3 lightToVertexWs = positionWs - light.lightPosition;
		float lightToVertexWsLength = length(lightToVertexWs);

		float2 sampleUv = directionToUV_cubeMapping(lightToVertexWs / lightToVertexWsLength, 1.f / tex2Dsize(lightShadowMap));

		const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
		const float shadowSampleDistanceToLight = shadowZ;
		const float currentFragmentDistanceToLight = lightToVertexWsLength / light.lightShadowRange;

		if (shadowSampleDistanceToLight < (currentFragmentDistanceToLight - light.lightShadowBias)) {
			return 0.f;
		}
		return 1.f;
	}
	else if (light.lightType == LightType_directional) {
		// Compute the derivatices of the position we are trying to find if it is in shadow or not.
		// These are needed for the PCF filtering, as we need to know where the position is for the neigbour
		// pixels to be able to figure out if they are in shadow or not.
		// Caution:
		// Most resources on the internet do not do that, they simply use the depth of the current frangment being
		// rasterized and compare its depth with the depth of the neighboring pixels on the shadow map.
		// while this is faster it always makes the shading a bit dimmer
		// in the cases where we have a flat plane and a light hitting the plane at 45deg angle.
		// (at least with my implementation, maybe I am doing something wrong).
		// This happens because the current pixel depth is 100% bigger than the 
		// depth of the pixel below it (relative to the shadow map), as a result
		// we think that the neighbor is in shadow (while it is not) and we darken the image.
		#ifdef SGE_PIXEL_SHADER
			// Some people say that dd*(dFd*) in some GLSL version work only for floats.
			const float3 positionWsDerivX = ddx(positionWs);
			const float3 positionWsDerivY = ddy(positionWs);
		#else
			// ddx/ddy are fine to get called in Vertex Shader in HLSL, I guess they return zero.
			// However  in GLSL this is a compilation error.
			const float3 positionWsDerivX = float3(0.f, 0.f, 0.f);
			const float3 positionWsDerivY = float3(0.f, 0.f, 0.f);
		#endif
		const float2 pixelSizeUVShadow = (1.f / tex2Dsize(lightShadowMap));

		// Perform the shadow map sampling in the pixel and around @pcfWidth pixels around it.
		// This will be used to soften the shadow.
		const float pcfWidth = 1.f; ///< An integer value, but because of computations made float.
		const float pcfTotalSampleCnt = (2.f * pcfWidth + 1.f) * (2.f * pcfWidth + 1.f);

		float numSamplesNotInShadow = 0.f;
		[unroll]
		for (float ix = -pcfWidth; ix <= pcfWidth; ix += 1) {
			[unroll]
			for (float iy = -pcfWidth; iy <= pcfWidth; iy += 1) {

				float3 shadowMapSamplePositionWs = positionWs;
				shadowMapSamplePositionWs += (float)ix * positionWsDerivX;
				shadowMapSamplePositionWs += (float)iy * positionWsDerivY;

				const float4 pixelShadowProj = mul(light.lightShadowMapProjView, float4(shadowMapSamplePositionWs, 1.f));
				const float4 pixelShadowNDC = pixelShadowProj / pixelShadowProj.w;

				float2 shadowMapSampleUV = pixelShadowNDC.xy;

				#ifndef OpenGL
					shadowMapSampleUV = shadowMapSampleUV * float2(0.5, -0.5) + float2(0.5, 0.5);
				#else
					shadowMapSampleUV = shadowMapSampleUV * float2(0.5, 0.5) + float2(0.5, 0.5);
				#endif

		#ifndef OpenGL
				const float shadowZ = tex2D(lightShadowMap, shadowMapSampleUV).x;
				const float currentZ = pixelShadowNDC.z;
		#else
				const float shadowZ = tex2D(lightShadowMap, shadowMapSampleUV).x;
				const float currentZ = (pixelShadowNDC.z + 1.f) * 0.5f;
		#endif
				const bool isInShadow = shadowZ < (currentZ - light.lightShadowBias);
				if (!isInShadow) {
					numSamplesNotInShadow += 1.f;
				}
			}
		}

		float lightMultDueToShadow = (numSamplesNotInShadow / pcfTotalSampleCnt);
		
		// Make directional light fade as they capture some limited area,
		// and when the shadow map ends in the distance this will hide the rough edge
		// where the shadow map ends.
		// TODO: people usually just multiple by the .w coord of the fragment (the one after the projection matrix).
		const float distanceFromCamera = length(positionWs - cameraPositionWs);
		const float fadeStart = light.lightShadowRange * 0.80f;
		if(distanceFromCamera > fadeStart) {
			const float fadeOffDistance = light.lightShadowRange * 0.2f;
			float k = min(1.f, (distanceFromCamera - fadeStart) / fadeOffDistance);
			// Slowly add up to lightMultDueToShadow until it reaches 1.
			lightMultDueToShadow = lightMultDueToShadow + (1.f - lightMultDueToShadow) * k;
		}

		return lightMultDueToShadow;
	}
	else if (light.lightType == LightType_spot) {
		const float4 pixelShadowProj = mul(light.lightShadowMapProjView, float4(positionWs, 1.f));
		const float4 pixelShadowNDC = pixelShadowProj / pixelShadowProj.w;

		float2 shadowMapSampleUV = pixelShadowNDC.xy;

		#ifndef OpenGL
			shadowMapSampleUV = shadowMapSampleUV * float2(0.5, -0.5) + float2(0.5, 0.5);
		#else
			shadowMapSampleUV = shadowMapSampleUV * float2(0.5, 0.5) + float2(0.5, 0.5);
		#endif

		const float shadowZ = tex2D(lightShadowMap, shadowMapSampleUV).x;

#ifndef OpenGL
		const float currentZ = pixelShadowNDC.z;
#else
		const float currentZ = (pixelShadowNDC.z + 1.f) * 0.5f;
#endif

		const bool isInShadow = shadowZ < (currentZ - light.lightShadowBias);
		const float lightMultDueToShadow = isInShadow ? 0.f : 1.f;

		return lightMultDueToShadow;
	}

	return 0.f;
}

float3 Light_computeDirectLighting(
	in ShaderLightData light,
	in sampler2D lightShadowMap,
	in MaterialSample mtlSample,
	in float3 cameraPositionWs)
{
	const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), mtlSample.albedo.xyz, mtlSample.metallic);

	// The @lightRadiance is the light that reaches the mtlSample.hitPointWs form the light
	// not how much light is reflected from the specified surface!
	float3 lightRadiance = float3(0.f, 0.f, 0.f);
	float3 L = float3(0.f, 0.f, 0.f);
	const float3 V = normalize(cameraPositionWs - mtlSample.hitPointWs);
	const float3 N = mtlSample.shadeNormalWs;

	const float range2 = light.lightShadowRange * light.lightShadowRange;

	if (light.lightType == LightType_point) {
		// Point ShaderLightData.
		const float3 toLightWs = light.lightPosition - mtlSample.hitPointWs;
		float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
		lightRadiance.xyz = light.lightColor * saturate(k * k);
		L = normalize(toLightWs);
	} else if (light.lightType == LightType_directional) {
		// Directional ShaderLightData.
		lightRadiance.xyz = light.lightColor;
		L = -light.lightDirection;
	} else if (light.lightType == LightType_spot) {
		// Spot ShaderLightData.
		const float3 toLightWs = light.lightPosition - mtlSample.hitPointWs;
		const float3 revSpotLightDirWs = -light.lightDirection;
		const float visibilityCosine = saturate(dot(normalize(toLightWs), revSpotLightDirWs));
		const float c = saturate(visibilityCosine - light.spotLightAngleCosine);
		const float range = 1.f - light.spotLightAngleCosine;
		const float scale = saturate(c / range);
		const float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
		lightRadiance.xyz = light.lightColor * (scale * saturate(k * k));
		L = normalize(toLightWs);
	}

	const float NdotL = max(0.f, dot(mtlSample.shadeNormalWs, L));

	float lightMultDueToShadow = 1.f;
	if(NdotL > 0.f && (light.lightFlags & kLightFlg_HasShadowMap) != 0) {
		lightMultDueToShadow = Light_computeShadowMultipler(light, lightShadowMap, mtlSample.hitPointWs, cameraPositionWs);
	}

	if (NdotL > 0.f && lightMultDueToShadow > 0.f) {
	#if 1
		const float3 H = normalize(V + L);

		// cook-torrance brdf
		const float NDF = DistributionGGX(N, H, mtlSample.roughness);
		const float G = GeometrySmith(N, V, L, mtlSample.roughness);
		const float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		const float3 kS = F;
		const float3 kD = (float3(1.f, 1.f, 1.f) - kS) * (1.0 - mtlSample.metallic);

		const float3 numerator = NDF * G * F;
		const float denominator = 4.f * max(dot(N, V), 0.f) * max(dot(N, L), 0.f);
		const float3 specular = numerator / max(denominator, 0.001f);

		return lightMultDueToShadow * (kD * mtlSample.albedo.xyz / PI + specular) * lightRadiance * NdotL;
	#else
		return lightMultDueToShadow * mtlSample.albedo.xyz * lightRadiance * NdotL;
	#endif
	}

	return float3(0.f, 0.f, 0.f);
}

#endif