#include "ShadeCommon.h"
#include "lib_skinning.hlsl"
#include "lib_lighting.hlsl"
#include "lib_textureMapping.hlsl"

float3 linearToSRGB(in float3 linearCol) {
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
	return sRGB;
}
//--------------------------------------------------------------------
// Uniforms and cbuffers.
//--------------------------------------------------------------------
// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
cbuffer ParamsCbFWDDefaultShading {
	FWDShader_CBuffer_Params fwdParams;
};

// Material maps.
uniform sampler2D texDiffuse;
uniform sampler2D uTexNormalMap;
uniform sampler2D uTexMetalness;
uniform sampler2D uTexRoughness;

// Shadows.
uniform sampler2D uLightShadowMap[kMaxLights];

// Skinning nones textures.
uniform sampler2D uSkinningBones;

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
// Vertex of the input assembler.
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

struct Vertex {
	float3 vertexOs; ///< The vertex position in object space.
	float3 normalOs; ///< The normal of the vertex in object space.
	float3 tangetOs;
	float3 binormalOs;
	float2 vertexUv0; ///< The UV coordinate of the vertex.
	float4 color; ///< The vertex color.
};

struct StageVertexOut {
	float4 SV_Position : SV_Position;
	float3 v_posWS : v_posWS;
	float2 v_uv : v_uv;
	float3 v_normal : v_normal;
	float3 v_tangent : v_tangent;
	float3 v_binormal : v_binormal;
	float4 v_vertexDiffuse : v_vertexDiffuse;
};

#ifdef SGE_MESH_VERTEX_MODIFIER_SHADER
	SGE_MESH_VERTEX_MODIFIER_SHADER__MAGIC_REPLACE
#else
	void vsMainVertexModifier(inout Vertex vertex)
	{

	}
#endif

StageVertexOut vsMain(IAVertex vsin)
{
	// Read the input vertex.
	Vertex vertex;

	vertex.vertexOs = vsin.a_position;
	vertex.normalOs = vsin.a_normal;
#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	vertex.tangetOs = vsin.a_tangent;
	vertex.binormalOs = vsin.a_binormal;
#else
	vertex.tangetOs = float3(0.0, 0.0, 0.0);
	vertex.binormalOs = float3(0.0, 0.0, 0.0);
#endif

	// If there is a skinning avilable apply it to the vertex in object space.
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	float4x4 skinMtx = libSkining_getSkinningTransform(
		vsin.a_bonesIds,
		fwdParams.mesh.uSkinningFirstBoneOffsetInTex,
		vsin.a_bonesWeights,
		uSkinningBones);
	vertex.vertexOs = mul(skinMtx, float4(vertex.vertexOs, 1.0)).xyz;
	vertex.normalOs = mul(skinMtx, float4(vertex.normalOs, 0.0)).xyz; // TODO: Proper normal transfrom by inverse transpose.
#endif

#if (OPT_HasUV == kHasUV_Yes)
	vertex.vertexUv0 = mul(fwdParams.material.uvwTransform, float4(vsin.a_uv, 0.0, 1.0)).xy;
#else
	vertex.vertexUv0 = float2(0.0, 0.0);
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	vertex.color = vsin.a_color;
#else
	// Set to white if no vertex color is used, as we might tint by this color.
	vertex.color = float4(1.0, 1.0, 1.0, 1.0);
#endif

	vsMainVertexModifier(vertex);

	// Read the processed vertex and pass it to the fragment shader.
	StageVertexOut stageVertexOut;

	// Pass the varyings to the next shader.
	const float4 vertexPosWs = mul(fwdParams.mesh.node2world, float4(vertex.vertexOs, 1.0));
	stageVertexOut.SV_Position = mul(fwdParams.camera.projView, vertexPosWs);
	stageVertexOut.v_posWS = vertexPosWs;

#if (OPT_HasUV == kHasUV_Yes)
	// Material stuff:
	stageVertexOut.v_uv  = mul(fwdParams.material.uvwTransform, float4(vertex.vertexUv0, 0.0, 1.0)).xy;
#else
	stageVertexOut.v_uv  = vertex.vertexUv0;
#endif
	// TODO: Proper normal transfrom by inverse transpose for normals.
	stageVertexOut.v_normal = mul(fwdParams.mesh.node2world, float4(vertex.normalOs, 0.0)); 
#if OPT_HasTangentSpace == kHasTangetSpace_Yes
	stageVertexOut.v_tangent = mul(fwdParams.mesh.node2world, float4(vertex.tangetOs, 0.0)).xyz;
	stageVertexOut.v_binormal = mul(fwdParams.mesh.node2world, float4(vertex.binormalOs, 0.0)).xyz;
#else
	stageVertexOut.v_tangent = float3(0.0, 0.0, 0.0);
	stageVertexOut.v_binormal = float3(0.0, 0.0, 0.0);
#endif
	stageVertexOut.v_vertexDiffuse = vertex.color;

	return stageVertexOut;
}

//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
#ifdef SGE_PIXEL_SHADER
float4 psMain(StageVertexOut inVert, bool isFrontFacing : SV_IsFrontFace) : SV_Target0
{
	MaterialSample mtlSample;
	mtlSample.albedo = fwdParams.material.uDiffuseColorTint;

	if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromConstantColor) {
		// We already use uDiffuseColorTint. So nothing to do here.
	} else if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromTexture) {
		mtlSample.albedo *= tex2D(texDiffuse, inVert.v_uv); // TODO: srgb to linear.
	} else if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_DiffuseFromVertexColor) {
		mtlSample.albedo *= inVert.v_vertexDiffuse;
	}

	if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_DiffuseTintByVertexColor) {
		mtlSample.albedo *= inVert.v_vertexDiffuse;
	}

	const float nrmFacingMul = 1.f; // isFrontFacing ? 1.f : -1.f;

	if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_HasNormalMap) {
		// Assumes that there is a correct tanget space.
		const float3 normalMapNorm = 2.f * tex2D(uTexNormalMap, inVert.v_uv).xyz - float3(1.f, 1.f, 1.f);
		const float3 normalFromNormalMap = 
			  normalMapNorm.x * normalize(inVert.v_tangent) 
			+ normalMapNorm.y * normalize(inVert.v_binormal) 
			+ normalMapNorm.z * normalize(inVert.v_normal);
		mtlSample.shadeNormalWs = normalize(normalFromNormalMap) * nrmFacingMul;
	} else {
		mtlSample.shadeNormalWs = normalize(inVert.v_normal) * nrmFacingMul;
	}

	// If the fragment is going to be fully transparent discard the sample.
	if (mtlSample.albedo.w <= 0.1f || fwdParams.material.alphaMultiplier <= 0.f) {
		discard;
	}

	mtlSample.hitPointWs = inVert.v_posWS;
	mtlSample.vertexToCameraDirWs = (fwdParams.camera.cameraPositionWs - inVert.v_posWS);

	mtlSample.metallic = fwdParams.material.uMetalness;
	mtlSample.roughness = fwdParams.material.uRoughness;

	if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_HasMetalnessMap) {
		mtlSample.metallic *= tex2D(uTexMetalness, inVert.v_uv).r; // TODO: srgb to linear.
	}

	if ((fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_HasRoughnessMap) != 0) {
		mtlSample.roughness *= tex2D(uTexRoughness, inVert.v_uv).r; // TODO: srgb to linear.
	}

	// Compute the lighting for each light.
	float4 finalColor;
	if (fwdParams.material.uPBRMtlFlags & kPBRMtl_Flags_NoLighting) {
		finalColor = mtlSample.albedo;
	} else {
		finalColor = float4(0.f, 0.f, 0.f, mtlSample.albedo.w);

		[unroll]
		for (int iLight = 0; iLight < fwdParams.lighting.lightsCnt; ++iLight) {
			finalColor.xyz += Light_computeDirectLighting(
				fwdParams.lighting.lights[iLight], 
				uLightShadowMap[iLight], 
				mtlSample, 
				fwdParams.camera.cameraPositionWs);
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

		float ambientLightAmount = lerp(1.f, ambientLightingFake, fwdParams.lighting.uAmbientFakeDetailAmount);
		finalColor.xyz += mtlSample.albedo.xyz * fwdParams.lighting.uAmbientLightColor * ambientLightAmount;
	} 


	//finalColor.xyz = pow(finalColor.xyz, float3(1.f / 2.2f, 1.f / 2.2f, 1.f / 2.2f));
	//finalColor.xyz = finalColor.xyz / (finalColor.xyz + float3(1.f, 1.f, 1.f));
	//finalColor.xyz = linearToSRGB(finalColor.xyz);

	// Apply the alpha multiplier.
	finalColor.w *= fwdParams.material.alphaMultiplier;

	return finalColor;
}
#endif
