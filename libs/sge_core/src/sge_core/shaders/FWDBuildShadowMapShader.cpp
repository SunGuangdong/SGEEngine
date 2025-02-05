#include "FWDBuildShadowMapShader.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_core/shaders/LightDesc.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/math/mat4f.h"

// Caution:
// this include is an exception do not include anything else like it.
#include "../core_shaders/FWDDefault_buildShadowMaps.h"
#include "../core_shaders/ShadeCommon.h"

namespace sge {

void FWDBuildShadowMapShader::drawGeometry(
    const RenderDestination& rdest,
    const vec3f& camPos,
    const mat4f& projView,
    const mat4f& world,
    const ShadowMapBuildInfo& shadowMapBuildInfo,
    const Geometry& geometry,
    const Texture* diffuseTexForAlphaMask,
    const bool forceNoCulling)
{
	enum {
		OPT_LightType,
		OPT_HasVertexSkinning,
		OPT_HasDiffuseTexForAlphaMasking,
		kNumOptions,
	};

	enum : int {
		uWorld,
		uProjView,
		uPointLightPositionWs,
		uPointLightFarPlaneDistance,
		uSkinningBones,
		uSkinningFirstBoneOffsetInTex,
		uDiffuseTexForAlphaMasking,
		uUseDiffuseTexForAlphaMasking
	};

	if (shadingPermutFWDBuildShadowMaps.isValid() == false) {
		shadingPermutFWDBuildShadowMaps = ShadingProgramPermuator();

		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_LightType,
		     "OPT_LightType",
		     {SGE_MACRO_STR(FWDDBSM_OPT_LightType_SpotOrDirectional), SGE_MACRO_STR(FWDDBSM_OPT_LightType_Point)}},
		    {OPT_HasVertexSkinning,
		     "OPT_HasVertexSkinning",
		     {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		    {OPT_HasDiffuseTexForAlphaMasking,
		     "OPT_HasDiffuseTexForAlphaMasking",
		     {SGE_MACRO_STR(kasDiffuseTexForAlphaMasking_No), SGE_MACRO_STR(kHasDiffuseTexForAlphaMasking_Yes)}},
		};

		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uWorld, "world", ShaderType::VertexShader},
		    {uProjView, "projView", ShaderType::VertexShader},
		    {uPointLightPositionWs, "uPointLightPositionWs", ShaderType::PixelShader},
		    {uPointLightFarPlaneDistance, "uPointLightFarPlaneDistance", ShaderType::PixelShader},
		    {uSkinningBones, "uSkinningBones", ShaderType::VertexShader},
		    {uSkinningFirstBoneOffsetInTex, "uSkinningFirstBoneOffsetInTex", ShaderType::VertexShader},
		    {uDiffuseTexForAlphaMasking, "uDiffuseTexForAlphaMasking", ShaderType::PixelShader},
		    {uUseDiffuseTexForAlphaMasking, "uUseDiffuseTexForAlphaMasking", ShaderType::PixelShader}};

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermutFWDBuildShadowMaps->createFromFile(
		    sgedev,
		    "core_shaders/FWDDefault_buildShadowMaps.hlsl",
		    "shader_cache/FWDBuildShadowMaps.shadercache",
		    compileTimeOptions,
		    uniformsToCache);
	}

	const int optHasVertexSkinning = (geometry.hasVertexSkinning()) ? kHasVertexSkinning_Yes : kHasVertexSkinning_No;
	const int optHasDiffuseTexForAlphaMasking = geometry.vertexDeclHasUv && diffuseTexForAlphaMask != nullptr;

	const OptionPermuataor::OptionChoice optionChoice[kNumOptions] = {
	    {OPT_LightType,
	     shadowMapBuildInfo.isPointLight ? FWDDBSM_OPT_LightType_Point : FWDDBSM_OPT_LightType_SpotOrDirectional},
	    {OPT_HasVertexSkinning, optHasVertexSkinning},
	    {OPT_HasDiffuseTexForAlphaMasking, optHasDiffuseTexForAlphaMasking},
	};

	const int iShaderPerm = shadingPermutFWDBuildShadowMaps->getCompileTimeOptionsPerm().computePermutationIndex(
	    optionChoice, SGE_ARRSZ(optionChoice));
	const ShadingProgramPermuator::Permutation& shaderPerm =
	    shadingPermutFWDBuildShadowMaps->getShadersPerPerm()[iShaderPerm];

	StaticArray<BoundUniform, 8> uniforms;

	shaderPerm.bind<8>(uniforms, (int)uWorld, (void*)&world);
	shaderPerm.bind<8>(uniforms, (int)uProjView, (void*)&projView);

	if (shadowMapBuildInfo.isPointLight) {
		vec3f pointLightPositionWs = camPos;
		shaderPerm.bind<8>(uniforms, uPointLightPositionWs, (void*)&pointLightPositionWs);
		shaderPerm.bind<8>(
		    uniforms, uPointLightFarPlaneDistance, (void*)&shadowMapBuildInfo.pointLightFarPlaneDistance);
	}

	if (optHasVertexSkinning == kHasVertexSkinning_Yes) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningBones], (geometry.skinningBoneTransforms)));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
		uniforms.push_back(
		    BoundUniform(shaderPerm.uniformLUT[uSkinningFirstBoneOffsetInTex], (void*)&geometry.firstBoneOffset));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	if (optHasDiffuseTexForAlphaMasking == kHasDiffuseTexForAlphaMasking_Yes) {
		int intTrue = 1;
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uUseDiffuseTexForAlphaMasking], &intTrue));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
		uniforms.push_back(
		    BoundUniform(shaderPerm.uniformLUT[uDiffuseTexForAlphaMasking], (void*)diffuseTexForAlphaMask));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	// Feed the draw call data to the state group.
	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
	stateGroup.setVBDeclIndex(geometry.vertexDeclIndex);
	stateGroup.setVB(0, geometry.vertexBuffer, uint32(geometry.vbByteOffset), geometry.stride);

	RasterizerState* rasterState = nullptr;
	if (forceNoCulling) {
		rasterState = getCore()->getGraphicsResources().RS_noCulling;
	}
	else {
		// TODO: Add a way to specify in the geometry to use the back faces for geometries.
		// This would remove the shadow acne for those gemetries without needed any depth bias.
		bool flipCulling = determinant(world) > 0.f;
		rasterState = flipCulling ? getCore()->getGraphicsResources().RS_default
		                          : getCore()->getGraphicsResources().RS_defaultBackfaceCCW;
	}


	stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual);

	if (geometry.ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry.indexBuffer, geometry.ibFmt, geometry.ibByteOffset);
	}
	else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	DrawCall dc;
	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.setStateGroup(&stateGroup);

	if (geometry.ibFmt != UniformType::Unknown) {
		dc.drawIndexed(geometry.numElements, 0, 0);
	}
	else {
		dc.draw(geometry.numElements, 0);
	}

	// Execute the draw call.
	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}


} // namespace sge
