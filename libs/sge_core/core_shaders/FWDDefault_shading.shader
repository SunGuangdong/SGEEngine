#include "ShadeCommon.h"

#define PI 3.14159265f

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
#include "lib_skinning.shader"
#endif

#include "lib_pbr.shader"

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
	float3 ambientLightColor;

	// Lights uniforms.
	float4 lightPosition;           // Position, w encodes the type of the light.
	float4 lightSpotDirAndCosAngle; // all Used in spot lights :( other lights do not use it
	float4 lightColorWFlag;         // w used for flags.

	float4x4 lightShadowMapProjView;
	float4 lightShadowRange;
	float4 lightShadowBias;

	// Skinning.
	int uSkinningFirstBoneOffsetInTex; ///< The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.
	
	float uCurvatureY;
	float uCurvatureZ;
	int uUseCurvature;
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
sampler2D lightShadowMap;
uniform samplerCUBE uPointLightShadowMap;

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
struct VS_INPUT {
	float3 a_position : a_position;

#if OPT_HasUV == kHasUV_Yes
	float2 a_uv : a_uv;
#endif

	float3 a_normal : a_normal;
#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	float3 a_tangent : a_tangent;
	float3 a_binormal : a_binormal;
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	float4 a_color : a_color;
#endif

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	int4 a_bonesIds : a_bonesIds;
	float4 a_bonesWeights : a_bonesWeights;
#endif
};

struct VS_OUTPUT {
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

VS_OUTPUT vsMain(VS_INPUT vsin) {
	VS_OUTPUT res;

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
#if 1
	if(uUseCurvature) {
		float curvatureY = uCurvatureY;
		float curvatureZ = uCurvatureZ;

		float curveStart = cameraPositionWs.x + 00.f;
		float curveCenter = cameraPositionWs.x + 100.f;
		float curveEnd = curveCenter + 30.f;

		float kz = (worldPos.x - curveStart) / (curveCenter - curveStart);
		kz = kz * kz * kz * kz;

		if(worldPos.x > curveStart && worldPos.x < curveCenter) {
			float k = (worldPos.x - curveStart) / (curveCenter - curveStart);
			k = 3.f * k * k - 2.f * k * k * k;
			
			worldPos.y += k * curvatureY;
			worldPos.z += kz * curvatureZ;
		} else if (worldPos.x > curveCenter) {
			float k = (worldPos.x - curveCenter) / (curveEnd - curveCenter);
			k = k * k * k * k;
			worldPos.y += curvatureY;			
			worldPos.y += -k * curvatureY;			
				
			worldPos.z += kz * curvatureZ;			
		}
	}
#endif

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
float4 psMain(VS_OUTPUT IN)
    : SV_Target0 {
	float4 diffuseColor = uDiffuseColorTint; //pow(uDiffuseColorTint, 2.2f);

	if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromConstantColor) {
		// We already use uDiffuseColorTint. So nothing to do here.
	} else if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromTexture) {
	#if (OPT_HasUV == kHasUV_Yes)
		diffuseColor *= tex2D(texDiffuse, IN.v_uv);
		//diffuseColor.xyz = pow(diffuseColor.xyz, 2.2f);
	#else // Should never happen.
		return float4(1.f, 0.f, 1.f, 1.f);
	#endif
	} else if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromVertexColor) {
	#if OPT_HasVertexColor == kHasVertexColor_Yes
		diffuseColor = IN.v_vertexDiffuse;
	#else // Should never happen.
		return float4(1.f, 0.f, 1.f, 1.f);
	#endif
	}

#if OPT_HasVertexColor == kHasVertexColor_Yes
	if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseTintByVertexColor) {
		diffuseColor *= IN.v_vertexDiffuse;
	}
#endif

	// TODO: Triplanar mapping.

	if (diffuseColor.w <= 0.f || alphaMultiplier <= 0.f) {
		discard;
	}

	// DOING SHADING HERE

	float3 normal;

	if (uPBRMtlFlags & kPBRMtl_Flags_HasNormalMap) {
#if (OPT_HasUV == kHasUV_Yes) && OPT_HasTangentSpace == kHasTangetSpace_Yes)
		const float3 normalMapNorm = 2.f * tex2D(uTexNormalMap, IN.v_uv).xyz - float3(1.f, 1.f, 1.f);
		const float3 normalFromNormalMap = normalMapNorm.x * normalize(IN.v_tangent) + normalMapNorm.y * normalize(IN.v_binormal) +
		                                   normalMapNorm.z * normalize(IN.v_normal);
		normal = normalize(normalFromNormalMap);
#else
		return float4(1.f, 1.f, 0.f, 1.f);
#endif
	} else {
		normal = normalize(IN.v_normal);
	}

	// GGX Crap:
	const float3 N = normalize(normal);
	const float3 V = normalize(cameraPositionWs.xyz - IN.v_posWS);

	float3 lighting = float3(0.f, 0.f, 0.f);
	if ((uPBRMtlFlags & kPBRMtl_Flags_NoLighting) == 0) {
		// Lighting.
		const int fLightFlags = (int)lightColorWFlag.w;
		const bool dontLight = (fLightFlags & kLightFlg_DontLight) != 0;
		if (dontLight == false) {
			// GGX Material crap:
			float metallic = uMetalness;
			float roughness = uRoughness;

#if OPT_HasUV == kHasUV_Yes
			if (uPBRMtlFlags & kPBRMtl_Flags_HasMetalnessMap) {
				metallic *= tex2D(uTexMetalness, IN.v_uv).r;
			}

			if ((uPBRMtlFlags & kPBRMtl_Flags_HasRoughnessMap) != 0) {
				roughness *= tex2D(uTexRoughness, IN.v_uv).r;
			}
#endif

			const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), diffuseColor.xyz, metallic);

			float3 lightRadiance = float3(0.f, 0.f, 0.f);
			float3 L = float3(0.f, 0.f, 0.f);

			const float4 lightPosF4 = lightPosition;
			const float3 lightColor = lightColorWFlag.xyz;
			const float range2 = lightShadowRange.x * lightShadowRange.x;


			if (lightPosF4.w == 0.f) {
				// Point Light.
				const float3 toLightWs = lightPosF4.xyz - IN.v_posWS;
				float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
				lightRadiance.xyz = lightColor * saturate(k * k);
				L = normalize(toLightWs);
			} else if (lightPosF4.w == 1.f) {
				// Direction Light.
				lightRadiance.xyz = lightColor;
				L = lightPosF4.xyz;
			} else if (lightPosF4.w == 2.f) {
				// Spot Light.
				const float3 toLightWs = lightPosF4.xyz - IN.v_posWS;
				const float3 revSpotLightDirWs = -lightSpotDirAndCosAngle.xyz;
				const float spotLightAngleCosine = lightSpotDirAndCosAngle.w;
				const float visibilityCosine = saturate(dot(normalize(toLightWs), revSpotLightDirWs));
				const float c = saturate(visibilityCosine - spotLightAngleCosine);
				const float range = 1.f - spotLightAngleCosine;
				const float scale = saturate(c / range);
				const float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
				lightRadiance.xyz = lightColor * (scale * saturate(k * k));
				L = normalize(toLightWs);
			}

			const float NdotL = max(dot(N, L), 0.0);

			const bool useShadowsMap = (fLightFlags & kLightFlg_HasShadowMap) != 0;
			
			// A  multiplier telling us how much the shadow should dim the lighting.
			// Usually it is a boolean telling if the object is in shadow or not.
			// However to make the shadows softer we use PCF filtering. This returns 
			// a value in [0;1] where 1 means there is no shadow, and 0 mean the object is totally in shadow.
			float lightMultDueToShadow = 1.f;
#if 1
			if (useShadowsMap != 0) {
				if (lightPosF4.w == 0.f) {
					// Point light.
					// [FWDDEF_POINTLIGHT_LINEAR_ZDEPTH]
					float3 lightToVertexWs = IN.v_posWS - lightPosF4.xyz;
					float lightToVertexWsLength = length(lightToVertexWs);
					const float shadowSampleDistanceToLight =
					    lightShadowRange.x * texCUBE(uPointLightShadowMap, lightToVertexWs / lightToVertexWsLength).x;
					const float currentFragmentDistanceToLight = lightToVertexWsLength;

					if (shadowSampleDistanceToLight < (currentFragmentDistanceToLight - lightShadowBias.x) || NdotL <= 0.0) {
						lightMultDueToShadow = 0.f;
					}
				} else {
					// Direction and Spot lights.
					const float4 pixelShadowProj = mul(lightShadowMapProjView, float4(IN.v_posWS, 1.f));
					const float4 pixelShadowNDC = pixelShadowProj / pixelShadowProj.w;

					float2 shadowMapSampleLocation = pixelShadowNDC.xy;
#ifndef OpenGL
					shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, -0.5) + float2(0.5, 0.5);
#else
					shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, 0.5) + float2(0.5, 0.5);
#endif

					const float2 pixelSizeUVShadow = (1.0 / tex2Dsize(lightShadowMap));

					float samplesWeights = 0.f;
					const int pcfWidth = 0;
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
							const float currentZ = (pixelShadowNDC.z + 1.0) * 0.5;
#endif

							if (shadowZ < (currentZ - lightShadowBias.x) || NdotL <= 0.0) {
								samplesWeights += 1.f;
							}
							
							pcfTotalSampleCnt2++;
						}
					}

					lightMultDueToShadow = 1.f - (samplesWeights / (float)(pcfTotalSampleCnt2));
					
					// Make directional light fade as they capture some limited area,
					// and when the shadow map ends in the distance this will hide the rough edge
					// where the shadow map ends.
					if (lightPosF4.w == 1.f) {
						const float distanceFromCamera = length(IN.v_posWS - cameraPositionWs);
						const float fadeStart = lightShadowRange.x * 0.80f;
						if(distanceFromCamera > fadeStart) {
							const float fadeOffDistance = lightShadowRange.x * 0.2f;
							float k = min(1.f, (distanceFromCamera - fadeStart) / fadeOffDistance);
							
							// Slowly add up to lightMultDueToShadow until it reaches 1.
							lightMultDueToShadow = lightMultDueToShadow + (1.f - lightMultDueToShadow) * k;
						}
					}
					
				}
			}
#endif // shadow map enabled

			if (NdotL > 1e-6f && lightMultDueToShadow > 1e-6f) {
				const float3 H = normalize(V + L);

				// cook-torrance brdf
#if 1
				const float NDF = DistributionGGX(N, H, roughness);
				const float G = GeometrySmith(N, V, L, roughness);
				const float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

				const float3 kS = F;
				const float3 kD = (float3(1.f, 1.f, 1.f) - kS) * (1.0 - metallic);

				const float3 numerator = NDF * G * F;
				const float denominator = 4.f * max(dot(N, V), 0.f) * max(dot(N, L), 0.f);
				const float3 specular = numerator / max(denominator, 0.001f);

				lighting += lightMultDueToShadow * (kD * diffuseColor.xyz / PI + specular) * lightRadiance * NdotL;
#else
				const float3 lightLighting = lightMultDueToShadow * diffuseColor.xyz * lightRadiance * NdotL;
				lighting += lightLighting;
#endif
			}

			if (NdotL < 0.f) {
				const float3 ambience = lightColor * 0.5f;
				lighting += ((normal.y * 0.5f + 0.5f) * ambience + ambience * 0.05f) * diffuseColor.xyz;
			}
		}
	}

	

	float4 finalColor = diffuseColor;

	if ((uPBRMtlFlags & kPBRMtl_Flags_NoLighting) == 0) {
		const float3 ambientLightColorLinear = ambientLightColor; // pow(ambientLightColor, 2.2f);
		const float3 fakeAmbientDetail =
		    ((normal.y * 0.5f + 0.5f) * ambientLightColorLinear + ambientLightColorLinear * 0.05f) * diffuseColor.xyz;
		const float3 rimLighting =
		    smoothstep(1.f - uRimLightColorWWidth.w, 1.f, (1.f - dot(uCameraLookDirWs, N))) * diffuseColor.xyz * uRimLightColorWWidth.xyz;

		finalColor = float4(lighting, diffuseColor.w) + float4(fakeAmbientDetail + rimLighting, 0.f);
	}
	
	// Fog
	// if( uUseCurvature ) {
		// float fogDist = max(0.f, IN.v_posWS.x - cameraPositionWs.x);
		// float fogDensity = exp(-fogDist / 100.f);
		// fogDensity = fogDensity * fogDensity;
		
		// finalColor.xyz = finalColor.xyz * fogDensity + (1 - fogDensity) * float3(0.12, 0.13, 0.12);
	// }

	//// Saturate the color, to make the selection highlight visiable for really brightly lit objects.
	// finalColor.x = saturate(finalColor.x);
	// finalColor.y = saturate(finalColor.y);
	// finalColor.z = saturate(finalColor.z);

	// Apply tone mapping.
	// finalColor.xyz = finalColor / (finalColor + float3(1.f, 1.f, 1.f));

	// Back to linear color space.
	//finalColor.xyz = pow(finalColor, 1.0f / 2.2f);

#if OPT_HasVertexColor == kHasVertexColor_Yes
	if (uPBRMtlFlags & kPBRMtl_Flags_DiffuseTintByVertexColor) {
		diffuseColor.w = IN.v_vertexDiffuse.w;
	}
#endif

	finalColor.w *= alphaMultiplier;

	return finalColor;
}
#endif
