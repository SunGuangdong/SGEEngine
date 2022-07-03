#include "GenericGeomListShader.h"
#include "sge_core/ICore.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/text/Path.h"

namespace sge {

const char* g_preApendedCode =
    R"shaderCode(
#include "ShadeCommon.h"
#include "lib_lighting.hlsl"
#include "lib_skinning.hlsl"

//--------------------------------------------------------------------
// Uniforms and cbuffers.
//--------------------------------------------------------------------
cbuffer ParamsCbFWDDefaultShading {
	FWDShader_CBuffer_Params fwdParams;
};

// Shadows.
uniform sampler2D uLightShadowMap[kMaxLights];

// Skinning nones textures.
uniform sampler2D uSkinningBones;

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

void vsMainVertexModifier(inout Vertex vertex);

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
	vertex.vertexUv0 = vsin.a_uv;
#else
	vertex.vertexUv0 = float2(0.0, 0.0);
#endif

#if OPT_HasVertexColor == kHasVertexColor_Yes
	vertex.color = vsin.a_color;
#else
	// Set to white if no vertex color is used, as we might tint by this color.
	vertex.color = float4(1.0, 1.0, 1.0, 1.0);
#endif

	vsMainVertexModifier(vertex); // <---- The function that the user writes

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
float modifyMaterialSample(
	inout MaterialSample mtlSample,
	bool isFrontFacing,
	in StageVertexOut inVert);

#ifdef SGE_PIXEL_SHADER
float4 psMain(StageVertexOut inVert, bool isFrontFacing : SV_IsFrontFace) : SV_Target0
{
	MaterialSample mtlSample;
	MaterialSample_constructor(mtlSample);

	modifyMaterialSample(mtlSample, inVert, isFrontFacing);

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
#endif

// More code to be appended at runtime below:

)shaderCode";

bool GenericGeomLitShader::createWithCodeFromFile(const char* codeFilePath)
{
	// clang-format off
	const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		{OPT_HasVertexColor, "OPT_HasVertexColor", {
			SGE_MACRO_STR(kHasVertexColor_No), 
			SGE_MACRO_STR(kHasVertexColor_Yes)
		}},
		{OPT_HasUV, "OPT_HasUV", {
			SGE_MACRO_STR(kHasUV_No), 
			SGE_MACRO_STR(kHasUV_Yes)
		}},
		{OPT_HasTangentSpace, "OPT_HasTangentSpace", { 
			SGE_MACRO_STR(kHasTangetSpace_No), 
			SGE_MACRO_STR(kHasTangetSpace_Yes)
		}},
		{OPT_HasVertexSkinning, "OPT_HasVertexSkinning", { 
			SGE_MACRO_STR(kHasVertexSkinning_No), 
			SGE_MACRO_STR(kHasVertexSkinning_Yes)
		}},
	};
	// clang-format on

	// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
	const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
	    {uTexSkinningBones, "uSkinningBones", ShaderType::VertexShader},
	    {uParamsCbFWDDefaultShading_vertex, "ParamsCbFWDDefaultShading", ShaderType::VertexShader},
	    {uParamsCbFWDDefaultShading_pixel, "ParamsCbFWDDefaultShading", ShaderType::PixelShader},
	};

	std::string baseShaderCode;
	FileReadStream::readTextFile(codeFilePath, baseShaderCode);

	std::string filename = extractFileNameWithExt(codeFilePath);

	std::string shaderCode = g_preApendedCode;
	shaderCode += "#line 0 " + filename + "\n";
	shaderCode += baseShaderCode;

	std::set<std::string> includedFiles;

	MAYBE_UNUSED bool succeeded = shaderPermutator.create(
	    getCore()->getDevice(),
	    shaderCode.c_str(),
	    filename.c_str(),
	    std::string(),
	    compileTimeOptions,
	    uniformsToCache,
	    &includedFiles);

	includedFiles.insert(codeFilePath);

	// Assuming that all shader permutations have the same uniforms.
	const ShadingProgramRefl& shaderRefl = shaderPermutator.getShadersPerPerm()[0].shadingProgram->getReflection();

	const int MtlCBuffer_strIdx = getCore()->getDevice()->getStringIndex("MtlCBuffer");

	for (const auto& cbufferReflPair : shaderRefl.cbuffers.m_uniforms) {
		const CBufferRefl& cbufferRefl = cbufferReflPair.second;
		if (cbufferRefl.name == "MtlCBuffer") {
			const std::vector<CBufferVariableRefl>& variables = cbufferRefl.variables;

			for (const CBufferVariableRefl& var : variables) {
				MtlCBufferExtraNumericUniforms extra;
				extra.uniformType = var.type;
				extra.byteOffset = var.offset;

				extraNumericUniforms[var.name] = extra;
			}
		}
	}

	return succeeded;

}

} // namespace sge
