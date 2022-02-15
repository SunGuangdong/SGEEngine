#include "SimpleTriplanarMtlGeomDrawer.h"
#include "SimpleTriplanarMtl.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/io/FileStream.h"
#include <sge_utils/math/mat4f.h>

// Caution:
// these includes is an exception do not include anything else like it:
// Note: it used to be just one file...
// but how does one share a struct between HLSL and C++ without something like this?
#include "../core_shaders/FWDDefault_buildShadowMaps.h"
#include "../core_shaders/ShadeCommon.h"
#include "../core_shaders/ShaderLightData.h"

using namespace sge;

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN

__declspec(align(4)) struct ParamsCbFWDDefaultShading {
	mat4f projView;
	mat4f world;
	mat4f uvwTransform;
	vec4f cameraPositionWs;
	vec4f uCameraLookDirWs;

	vec4f uDiffuseColorTint;

	int uHasDiffuseTextures;
	int uIgnoreTranslationForTriplanarSampling;
	int uForceNoLighting;
	float uTriplanarSharpness;

	vec3f uAmbientLightColor;
	float uAmbientFakeDetailAmount;

	ShaderLightData lights[kMaxLights];
	int lightsCnt;
	int lightCnt_padding[3];
};

//-----------------------------------------------------------------------------
// SimpleTriplanarMtlGeomDrawer
//-----------------------------------------------------------------------------
void SimpleTriplanarMtlGeomDrawer::drawGeometry(
    const RenderDestination& rdest,
    const ICamera& camera,
    const mat4f& geomWorldTransfrom,
    const ObjectLighting& lighting,
    const Geometry& geometry,
    const IMaterialData* mtlDataBase,
    const InstanceDrawMods& UNUSED(instDrawMods))
{
	const SimpleTriplanarMtlData& mtlData = *dynamic_cast<const SimpleTriplanarMtlData*>(mtlDataBase);

	SGEDevice* const sgedev = rdest.getDevice();
	if (!paramsBuffer.IsResourceValid()) {
		BufferDesc bd = BufferDesc::GetDefaultConstantBuffer(4096, ResourceUsage::Dynamic);
		paramsBuffer = sgedev->requestResource<Buffer>();
		paramsBuffer->create(bd, nullptr);
	}

	enum : int {
		uParamsCb_vertex,
		uParamsCb_pixel,

		uTexDiffuseX,
		uTexDiffuseXSampler,
		uTexDiffuseY,
		uTexDiffuseYSampler,
		uTexDiffuseZ,
		uTexDiffuseZSampler,

		uLightShadowMap,
		uLightShadowMapSampler,

	};

	ParamsCbFWDDefaultShading paramsCb;
	memset(&paramsCb, 0, sizeof(paramsCb));

	if (shadingPermutFWDShading.isValid() == false) {
		shadingPermutFWDShading = ShadingProgramPermuator();

		// clang-format off
		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {};
		// clang-format on

		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uParamsCb_vertex, "ParamCB", ShaderType::VertexShader},
		    {uParamsCb_pixel, "ParamCB", ShaderType::PixelShader},


		    {uTexDiffuseX, "texDiffuseX", ShaderType::PixelShader},
		    {uTexDiffuseXSampler, "texDiffuseX_sampler", ShaderType::PixelShader},

		    {uTexDiffuseY, "texDiffuseY", ShaderType::PixelShader},
		    {uTexDiffuseYSampler, "texDiffuseY_sampler", ShaderType::PixelShader},

		    {uTexDiffuseZ, "texDiffuseZ", ShaderType::PixelShader},
		    {uTexDiffuseZSampler, "texDiffuseZ_sampler", ShaderType::PixelShader},

		    {uLightShadowMap, "uLightShadowMap", ShaderType::PixelShader},
		    {uLightShadowMapSampler, "uLightShadowMap_sampler", ShaderType::PixelShader},
		};


		shadingPermutFWDShading->createFromFile(
		    sgedev,
		    "core_shaders/FWDSimpleTriplanar.hlsl",
		    "shader_cache/FWDSimpleTriplanar.shadercache",
		    compileTimeOptions,
		    uniformsToCache);
	}

	const int iShaderPerm = shadingPermutFWDShading->getCompileTimeOptionsPerm().computePermutationIndex(nullptr, 0);
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermutFWDShading->getShadersPerPerm()[iShaderPerm];

	// Assemble the draw call.
	DrawCall dc;

	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setVBDeclIndex(geometry.vertexDeclIndex);
	stateGroup.setVB(0, geometry.vertexBuffer, uint32(geometry.vbByteOffset), geometry.stride);
	stateGroup.setPrimitiveTopology(geometry.topology);
	if (geometry.ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry.indexBuffer, geometry.ibFmt, geometry.ibByteOffset);
	}
	else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	RasterizerState* rasterState = nullptr;

	// We are baking a shadow map and we want to render the backfaces.
	// *opposing to the regular rendering which uses front faces... duh).
	// This is done to avoid the Shadow Acne artifacts caused by floating point
	// innacuraties introduced by the depth texture.
	bool flipCulling = determinant(geomWorldTransfrom) > 0.f;
	rasterState = flipCulling ? getCore()->getGraphicsResources().RS_default
	                          : getCore()->getGraphicsResources().RS_defaultBackfaceCCW;

	StaticArray<BoundUniform, 64> uniforms;

	const bool hasAllTextures = mtlData.diffuseTextureX && mtlData.diffuseTextureY && mtlData.diffuseTextureZ;

	paramsCb.world = geomWorldTransfrom;
	paramsCb.cameraPositionWs = vec4f(camera.getCameraPosition(), 1.f);
	paramsCb.uCameraLookDirWs = vec4f(camera.getCameraLookDir(), 0.f);
	paramsCb.projView = camera.getProjView();
	paramsCb.uDiffuseColorTint = mtlData.diffuseColor;
	paramsCb.uvwTransform = mtlData.uvwTransform;
	paramsCb.uDiffuseColorTint = mtlData.diffuseColor;
	paramsCb.uForceNoLighting = mtlData.forceNoLighting ? 1 : 0;
	paramsCb.uIgnoreTranslationForTriplanarSampling = mtlData.ignoreTranslation;
	paramsCb.uHasDiffuseTextures = hasAllTextures ? 1 : 0;
	paramsCb.uTriplanarSharpness = mtlData.sharpness;


	if (hasAllTextures) {
		shaderPerm.bind<64>(uniforms, uTexDiffuseX, (void*)mtlData.diffuseTextureX);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseXSampler, (void*)mtlData.diffuseTextureX->getSamplerState());
#endif

		shaderPerm.bind<64>(uniforms, uTexDiffuseY, (void*)mtlData.diffuseTextureY);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseYSampler, (void*)mtlData.diffuseTextureY->getSamplerState());
#endif

		shaderPerm.bind<64>(uniforms, uTexDiffuseZ, (void*)mtlData.diffuseTextureZ);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseZSampler, (void*)mtlData.diffuseTextureZ->getSamplerState());
#endif
	}


	// Lights and draw call.
	Texture* shadowMaps[kMaxLights] = {nullptr};

	paramsCb.lightsCnt = minOf((int)SGE_ARRSZ(paramsCb.lights), lighting.lightsCount);
	for (int iLight = 0; iLight < paramsCb.lightsCnt; ++iLight) {
		ShaderLightData& shaderLight = paramsCb.lights[iLight];
		const ShadingLightData* srcLight = lighting.ppLightData[iLight];

		if_checked(srcLight && srcLight->pLightDesc)
		{
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
	paramsCb.uAmbientFakeDetailAmount = lighting.ambientFakeDetailBias;

	stateGroup.setRenderState(
	    rasterState,
	    getCore()->getGraphicsResources().DSS_default_lessEqual,
	    getCore()->getGraphicsResources().BS_backToFrontAlpha);

	void* paramsMappedData = sgedev->getContext()->map(paramsBuffer, Map::WriteDiscard);
	memcpy(paramsMappedData, &paramsCb, sizeof(paramsCb));
	sgedev->getContext()->unMap(paramsBuffer);

	uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCb_vertex], paramsBuffer.GetPtr()));
	uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCb_pixel], paramsBuffer.GetPtr()));

	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.setStateGroup(&stateGroup);

	if (geometry.ibFmt != UniformType::Unknown) {
		dc.drawIndexed(geometry.numElements, 0, 0);
	}
	else {
		dc.draw(geometry.numElements, 0);
	}

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}
