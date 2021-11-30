#ifndef LIGHTING_SHADER
#define LIGHTING_SHADER

#include "ShaderLightData.h"
#include "lib_pbr.shader"

struct MaterialSample {
	float3 hitPointWs;
	float3 shadeNormalWs; ///< The normal in the point that should be used for shading. It might not be the face normal if bump/normal mapping is applied.
	float3 vertexToCameraDirWs; ///< Normalized vector point from @hitPointWs to the camera.

	float4 albedo;
	float metallic;
	float roughness;
};

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
	int lightType = light.lightType;

	if (lightType == LightType_point) {
		// [FWDDEF_POINTLIGHT_LINEAR_ZDEPTH]
		float3 lightToVertexWs = positionWs - light.lightPosition;
		float lightToVertexWsLength = length(lightToVertexWs);

		float2 sampleUv = directionToUV_cubeMapping(lightToVertexWs / lightToVertexWsLength, 1.f / tex2Dsize(lightShadowMap));
		const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
		const float shadowSampleDistanceToLight = light.lightShadowRange * shadowZ;
		const float currentFragmentDistanceToLight = lightToVertexWsLength;

		if (shadowSampleDistanceToLight < (currentFragmentDistanceToLight - light.lightShadowBias)/* || NdotL <= 0.0*/) {
			return 0.f;
		}
		return 1.f;
	} else {
		// Direction and Spot lights.
		const float4 pixelShadowProj = mul(light.lightShadowMapProjView, float4(positionWs, 1.f));
		const float4 pixelShadowNDC = pixelShadowProj / pixelShadowProj.w;

		float2 shadowMapSampleLocation = pixelShadowNDC.xy;

		#ifndef OpenGL
			shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, -0.5) + float2(0.5, 0.5);
		#else
			shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, 0.5) + float2(0.5, 0.5);
		#endif

		const float2 pixelSizeUVShadow = (1.f / tex2Dsize(lightShadowMap));

		float samplesWeights = 0.f;
		const int pcfWidth = 2;
		const int pcfTotalSampleCnt = (2 * pcfWidth + 1) * (2 * pcfWidth + 1);
		int pcfTotalSampleCnt2 = 0;
		for (int ix = -pcfWidth; ix <= pcfWidth; ix += 1) {
			for (int iy = -pcfWidth; iy <= pcfWidth; iy += 1) {
				const float2 sampleUv =
					shadowMapSampleLocation + float2(float(ix) * pixelSizeUVShadow.x, float(iy) * pixelSizeUVShadow.y);

		#ifndef OpenGL
				const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
				const float currentZ = pixelShadowNDC.z;
		#else
				const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
				const float currentZ = (pixelShadowNDC.z + 1.f) * 0.5f;
		#endif

				if (shadowZ < (currentZ - light.lightShadowBias)/* || NdotL <= 0.f*/) {
					samplesWeights += 1.f;
				}
				
				pcfTotalSampleCnt2++;
			}
		}

		float lightMultDueToShadow = 1.f - (samplesWeights / (float)(pcfTotalSampleCnt2));
		
		// Make directional light fade as they capture some limited area,
		// and when the shadow map ends in the distance this will hide the rough edge
		// where the shadow map ends.
		if  (lightType == LightType_directional) {
			const float distanceFromCamera = length(positionWs - cameraPositionWs);
			const float fadeStart = light.lightShadowRange * 0.80f;
			if(distanceFromCamera > fadeStart) {
				const float fadeOffDistance = light.lightShadowRange * 0.2f;
				float k = min(1.f, (distanceFromCamera - fadeStart) / fadeOffDistance);
				
				// Slowly add up to lightMultDueToShadow until it reaches 1.
				lightMultDueToShadow = lightMultDueToShadow + (1.f - lightMultDueToShadow) * k;
			}
		}

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