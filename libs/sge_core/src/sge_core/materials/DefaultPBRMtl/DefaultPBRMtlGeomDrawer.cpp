#include "DefaultPBRMtlGeomDrawer.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/FileStream.h"
#include <sge_utils/math/mat4.h>

// Caution:
// these includes is an exception do not include anything else like it:
// Note: it used to be just one file...
// but how does one share a struct between HLSL and C++ without something like this?
#include "../core_shaders/FWDDefault_buildShadowMaps.h"
#include "../core_shaders/ShadeCommon.h"
#include "../core_shaders/ShaderLightData.h"

#include "DefaultPBRMtl.h"

using namespace sge;

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN

__declspec(align(4)) struct ParamsCbFWDDefaultShading {
	mat4f projView;
	mat4f world;
	mat4f uvwTransform;
	vec4f cameraPositionWs;
	vec4f uCameraLookDirWs;

	vec4f uDiffuseColorTint;
	vec3f texDiffuseXYZScaling;
	float uMetalness;

	float uRoughness;
	int uPBRMtlFlags;
	float alphaMultiplier;
	int alphaMultiplier_padding;

	vec4f uRimLightColorWWidth;
	vec3f uAmbientLightColor;

	// Skinning.
	int uSkinningFirstBoneOffsetInTex; ///< The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.

	ShaderLightData lights[kMaxLights];
	int lightsCnt;
	int lightCnt_padding[3];
};

//-----------------------------------------------------------------------------
// BasicModelDraw
//-----------------------------------------------------------------------------
void DefaultPBRMtlGeomDrawer::drawGeometry(const RenderDestination& rdest,
                                           const ICamera& camera,
                                           const mat4f& geomWorldTransfrom,
                                           const ObjectLighting& lighting,
                                           const Geometry& geometry,
                                           const IMaterialData* mtlDataBase,
                                           const InstanceDrawMods& instDrawMods) {
	const DefaultPBRMtlData& mtlData = *dynamic_cast<const DefaultPBRMtlData*>(mtlDataBase);

	SGEDevice* const sgedev = rdest.getDevice();
	if (!paramsBuffer.IsResourceValid()) {
		BufferDesc bd = BufferDesc::GetDefaultConstantBuffer(4096, ResourceUsage::Dynamic);
		paramsBuffer = sgedev->requestResource<Buffer>();
		paramsBuffer->create(bd, nullptr);
	}

	enum : int {
		OPT_HasVertexColor,
		OPT_HasUV,
		OPT_HasTangentSpace,
		OPT_HasVertexSkinning,

		// OPT_UseNormalMap,
		// OPT_DiffuseColorSrc,
		// OPT_Lighting,
		// OPT_HasVertexSkinning,
		kNumOptions,
	};

	enum : int {
		uTexDiffuse,
		uTexDiffuseSampler,
		uTexNormalMap,
		uTexNormalMapSampler,
		uTexDiffuseX,
		uTexDiffuseXSampler,
		uTexDiffuseY,
		uTexDiffuseYSampler,
		uTexDiffuseZ,
		uTexDiffuseZSampler,
		uTexDiffuseXYZScaling,
		uTexMetalness,
		uTexMetalnessSampler,
		uTexRoughness,
		uTexRoughnessSampler,
		uLightShadowMap,
		uLightShadowMapSampler,
		uTexSkinningBones,
		uParamsCbFWDDefaultShading_vertex,
		uParamsCbFWDDefaultShading_pixel,
	};

	ParamsCbFWDDefaultShading paramsCb;
	memset(&paramsCb, 0, sizeof(paramsCb));

	if (shadingPermutFWDShading.isValid() == false) {
		shadingPermutFWDShading = ShadingProgramPermuator();

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
		    //{OPT_UseNormalMap, "OPT_UseNormalMap", {"0", "1"}},
		    //{OPT_DiffuseColorSrc, "OPT_DiffuseColorSrc", {"0", "1", "2", "3", "4"}},
		    //{OPT_Lighting, "OPT_Lighting", {SGE_MACRO_STR(kLightingShaded), SGE_MACRO_STR(kLightingForceNoLighting)}},
		    //{OPT_HasVertexSkinning, "OPT_HasVertexSkinning", {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		};
		// clang-format on

		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uTexDiffuse, "texDiffuse", ShaderType::PixelShader},
		    {uTexDiffuseSampler, "texDiffuse_sampler", ShaderType::PixelShader},
		    {uTexNormalMap, "uTexNormalMap", ShaderType::PixelShader},
		    {uTexNormalMapSampler, "uTexNormalMap_sampler", ShaderType::PixelShader},
		    {uTexDiffuseX, "texDiffuseX", ShaderType::PixelShader},
		    {uTexDiffuseXSampler, "texDiffuseX_sampler", ShaderType::PixelShader},
		    {uTexDiffuseY, "texDiffuseY", ShaderType::PixelShader},
		    {uTexDiffuseYSampler, "texDiffuseY_sampler", ShaderType::PixelShader},
		    {uTexDiffuseZ, "texDiffuseZ", ShaderType::PixelShader},
		    {uTexDiffuseZSampler, "texDiffuseZ_sampler", ShaderType::PixelShader},
		    {uTexDiffuseXYZScaling, "texDiffuseXYZScaling", ShaderType::PixelShader},
		    {uTexMetalness, "uTexMetalness", ShaderType::PixelShader},
		    {uTexMetalnessSampler, "uTexMetalness_sampler", ShaderType::PixelShader},
		    {uTexRoughness, "uTexRoughness", ShaderType::PixelShader},
		    {uTexRoughnessSampler, "uTexRoughness_sampler", ShaderType::PixelShader},
		    {uLightShadowMap, "uLightShadowMap", ShaderType::PixelShader},
		    {uLightShadowMapSampler, "uLightShadowMap_sampler", ShaderType::PixelShader},
		    {uTexSkinningBones, "uSkinningBones", ShaderType::VertexShader},
		    {uParamsCbFWDDefaultShading_vertex, "ParamsCbFWDDefaultShading", ShaderType::VertexShader},
		    {uParamsCbFWDDefaultShading_pixel, "ParamsCbFWDDefaultShading", ShaderType::PixelShader},
		};


		shadingPermutFWDShading->createFromFile(sgedev, "core_shaders/FWDDefault_shading.shader", compileTimeOptions, uniformsToCache);
	}

	// Find the values of the shader options that are going to be used.
	// This is needed for find the permutation of the shader compilation flags that we need.
	const int OPT_HasVertexColor_choice = geometry.vertexDeclHasVertexColor ? kHasVertexColor_Yes : kHasVertexColor_No;
	const int OPT_HasUV_choice = geometry.vertexDeclHasUv ? kHasUV_Yes : kHasUV_No;
	int OPT_HasNormals_choice = kHasTangetSpace_No;
	if (geometry.vertexDeclHasNormals) {
		if (geometry.vertexDeclHasTangentSpace) {
			OPT_HasNormals_choice = kHasTangetSpace_Yes;
		}
	}
	const int OPT_HasVertexSkinning_choice = geometry.hasVertexSkinning() ? kHasVertexColor_Yes : kHasVertexColor_No;

	// Compute the shader material flags.
	// Depending on the shader settings we might turn off some options as we would not need them.
	int pbrMtlFlags = 0;

	if (OPT_HasUV_choice != kHasUV_No && OPT_HasNormals_choice == kHasTangetSpace_Yes && mtlData.texNormalMap != nullptr) {
		pbrMtlFlags |= kPBRMtl_Flags_HasNormalMap;
	}

	if (OPT_HasUV_choice != kHasUV_No && mtlData.texMetalness != nullptr) {
		pbrMtlFlags |= kPBRMtl_Flags_HasMetalnessMap;
	}

	if (OPT_HasUV_choice != kHasUV_No && mtlData.texRoughness != nullptr) {
		pbrMtlFlags |= kPBRMtl_Flags_HasRoughnessMap;
	}

	switch (mtlData.diffuseColorSrc) {
		case DefaultPBRMtlData::diffuseColorSource_vertexColor: {
			if (OPT_HasVertexColor_choice == kHasVertexColor_Yes) {
				pbrMtlFlags |= kPBRMtl_Flags_DiffuseFromVertexColor;
			} else {
				pbrMtlFlags |= kPBRMtl_Flags_DiffuseFromConstantColor;
			}
		} break;
		case DefaultPBRMtlData::diffuseColorSource_constantColor: {
			pbrMtlFlags |= kPBRMtl_Flags_DiffuseFromConstantColor;
		} break;
		case DefaultPBRMtlData::diffuseColorSource_diffuseMap: {
			if (OPT_HasUV_choice != kHasUV_No) {
				pbrMtlFlags |= kPBRMtl_Flags_DiffuseFromTexture;
			}
		} break;
		default:
			sgeAssert(false);
			pbrMtlFlags |= kPBRMtl_Flags_DiffuseFromConstantColor;
			break;
	}

	if (mtlData.tintByVertexColor) {
		pbrMtlFlags |= kPBRMtl_Flags_DiffuseTintByVertexColor;
	}

	if (instDrawMods.forceNoLighting) {
		pbrMtlFlags |= kPBRMtl_Flags_NoLighting;
	}

	// Find the shader permuatation that we are going to use.
	const OptionPermuataor::OptionChoice optionChoice[kNumOptions] = {{OPT_HasVertexColor, OPT_HasVertexColor_choice},
	                                                                  {OPT_HasUV, OPT_HasUV_choice},
	                                                                  {OPT_HasTangentSpace, OPT_HasNormals_choice},
	                                                                  {OPT_HasVertexSkinning, OPT_HasVertexSkinning_choice}};

	const int iShaderPerm =
	    shadingPermutFWDShading->getCompileTimeOptionsPerm().computePermutationIndex(optionChoice, SGE_ARRSZ(optionChoice));
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermutFWDShading->getShadersPerPerm()[iShaderPerm];

	// Assemble the draw call.
	DrawCall dc;

	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setVBDeclIndex(geometry.vertexDeclIndex);
	stateGroup.setVB(0, geometry.vertexBuffer, uint32(geometry.vbByteOffset), geometry.stride);
	stateGroup.setPrimitiveTopology(geometry.topology);
	if (geometry.ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry.indexBuffer, geometry.ibFmt, geometry.ibByteOffset);
	} else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	RasterizerState* rasterState = nullptr;
	if (instDrawMods.forceNoCulling || mtlData.disableCulling) {
		rasterState = getCore()->getGraphicsResources().RS_noCulling;
	} else {
		// We are baking shadow maps and we want to render the backfaces
		// *opposing to the regular rendering which uses front faces... duh).
		// This is done to avoid the Shadow Acne artifacts caused by floating point
		// innacuraties introduced by the depth texture.
		bool flipCulling = determinant(geomWorldTransfrom) > 0.f;
		rasterState = flipCulling ? getCore()->getGraphicsResources().RS_default : getCore()->getGraphicsResources().RS_defaultBackfaceCCW;
	}

	StaticArray<BoundUniform, 64> uniforms;

	mat4f combinedUVWTransform = instDrawMods.uvwTransform * mtlData.uvwTransform;

	paramsCb.world = geomWorldTransfrom;
	paramsCb.cameraPositionWs = vec4f(camera.getCameraPosition(), 1.f);
	paramsCb.uCameraLookDirWs = vec4f(camera.getCameraLookDir(), 0.f);
	paramsCb.projView = camera.getProjView();
	paramsCb.uDiffuseColorTint = mtlData.diffuseColor;
	paramsCb.uvwTransform = combinedUVWTransform;
	paramsCb.texDiffuseXYZScaling = mtlData.diffuseTexXYZScaling;
	paramsCb.uMetalness = mtlData.metalness;
	paramsCb.uRoughness = mtlData.roughness;
	paramsCb.uSkinningFirstBoneOffsetInTex = geometry.firstBoneOffset;
	paramsCb.uPBRMtlFlags = pbrMtlFlags;
	paramsCb.alphaMultiplier = mtlData.alphaMultiplier;

	if (mtlData.diffuseTexture) {
		shaderPerm.bind<64>(uniforms, uTexDiffuse, (void*)mtlData.diffuseTexture);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseSampler, (void*)mtlData.diffuseTexture->getSamplerState());
#endif
	}

	// TRIPLANAR LEGACY
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseX, (void*)mtlData.diffuseTextureX);
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseY, (void*)mtlData.diffuseTextureY);
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseZ, (void*)mtlData.diffuseTextureZ);
	//#ifdef SGE_RENDERER_D3D11
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseXSampler, (void*)mtlData.diffuseTextureX->getSamplerState());
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseYSampler, (void*)mtlData.diffuseTextureY->getSamplerState());
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseZSampler, (void*)mtlData.diffuseTextureZ->getSamplerState());
	//#endif
	//		shaderPerm.bind<64>(uniforms, uTexDiffuseXYZScaling, (void*)&mtlData.diffuseTexXYZScaling);

	if (mtlData.texMetalness != nullptr) {
		shaderPerm.bind<64>(uniforms, uTexMetalness, (void*)mtlData.texMetalness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexMetalnessSampler, (void*)mtlData.texMetalness->getSamplerState());
#endif
	}

	if (mtlData.texRoughness != nullptr) {
		shaderPerm.bind<64>(uniforms, uTexRoughness, (void*)mtlData.texRoughness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexRoughnessSampler, (void*)mtlData.texRoughness->getSamplerState());
#endif
	}

	if (mtlData.texNormalMap) {
		shaderPerm.bind<64>(uniforms, uTexNormalMap, (void*)mtlData.texNormalMap);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexNormalMapSampler, (void*)mtlData.texNormalMap->getSamplerState());
#endif
	}

	if (OPT_HasVertexSkinning_choice == kHasVertexSkinning_Yes) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uTexSkinningBones], (geometry.skinningBoneTransforms)));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	// Lights and draw call.
	Texture* shadowMaps[kMaxLights] = {nullptr};

	paramsCb.lightsCnt = minOf((int)SGE_ARRSZ(paramsCb.lights), lighting.lightsCount);
	for (int iLight = 0; iLight < paramsCb.lightsCnt; ++iLight) {
		ShaderLightData& shaderLight = paramsCb.lights[iLight];
		const ShadingLightData* srcLight = lighting.ppLightData[iLight];

		if_checked(srcLight && srcLight->pLightDesc) {
			// We assume that the light is enabled if it reached here.
			// We don't want to make the code more complicated with redundadnt checks.
			sgeAssert(srcLight->pLightDesc->isOn == true);

			const bool shouldHaveShadows = srcLight->pLightDesc->hasShadows && srcLight->shadowMap;

			shaderLight.lightType = int(srcLight->pLightDesc->type);
			shaderLight.lightPosition = srcLight->lightPositionWs;
			shaderLight.lightDirection = srcLight->lightDirectionWs;
			shaderLight.lightShadowRange = srcLight->pLightDesc->range;
			shaderLight.lightShadowBias = srcLight->pLightDesc->shadowMapBias;
			shaderLight.lightColor = srcLight->pLightDesc->color * srcLight->pLightDesc->intensity;
			shaderLight.spotLightAngleCosine = cosf(srcLight->pLightDesc->spotLightAngle);
			shaderLight.lightShadowMapProjView = srcLight->shadowMapProjView;

			int lightFlags = 0;
			if (shouldHaveShadows) {
				lightFlags |= kLightFlg_HasShadowMap;
			}

			shaderLight.lightFlags = lightFlags;

			// Bind the shadow map.
			if (shouldHaveShadows && !shaderPerm.uniformLUT[uLightShadowMap].isNull()) {
				shadowMaps[iLight] = srcLight->shadowMap;
			}
		}
	}


	// TODO: Do we have binding arrays of textures in any shape of form?
	uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uLightShadowMap], (&shadowMaps)));
	sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);

	paramsCb.uAmbientLightColor = lighting.ambientLightColor;
	paramsCb.uRimLightColorWWidth = lighting.uRimLightColorWWidth;

	if (instDrawMods.forceAdditiveBlending) {
		stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
		                          getCore()->getGraphicsResources().BS_backToFrontAlpha);
	} else {
		stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
		                          getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}

	void* paramsMappedData = sgedev->getContext()->map(paramsBuffer, Map::WriteDiscard);
	memcpy(paramsMappedData, &paramsCb, sizeof(paramsCb));
	sgedev->getContext()->unMap(paramsBuffer);

	uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_vertex], paramsBuffer.GetPtr()));
	uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_pixel], paramsBuffer.GetPtr()));

	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.setStateGroup(&stateGroup);

	if (geometry.ibFmt != UniformType::Unknown) {
		dc.drawIndexed(geometry.numElements, 0, 0);
	} else {
		dc.draw(geometry.numElements, 0);
	}

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}
