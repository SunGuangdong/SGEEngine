#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/io/FileWatcher.h"
#include "sge_utils/math/mat4.h"

namespace sge {

struct ICamera;

struct SkyShaderSettings {
	// [SKY_ENUM_DUPLICATED]
	enum Mode : int {
		mode_colorGradinet = 0,
		mode_textureSphericalMapped = 1,
		mode_textureCubeMapped = 2,
	};

	Mode mode = mode_colorGradinet;
	vec3f topColor = vec3f(0.75f);
	vec3f bottomColor = vec3f(0.25f);
	vec3f sunDirection = vec3f(0.f, 0.f, 1.f);
	Texture* texture = nullptr;
};

struct SGE_CORE_API SkyShader {
	void draw(
	    const RenderDestination& rdest, const vec3f& cameraPosition, const mat4f view, const mat4f proj, const SkyShaderSettings& sets);

  private:
	Optional<ShadingProgramPermuator> shadingPermut;
	FilesWatcher shaderFilesWatcher;

	GpuHandle<Buffer> m_skySphereVB;
	int m_skySphereNumVerts = 0;
	VertexDeclIndex m_skySphereVBVertexDeclIdx = VertexDeclIndex_Null;

	GpuHandle<Buffer> m_fullScreenQuadVB;
	int m_fullScreenQuadNumVerts = 0;
	VertexDeclIndex m_fullScreenQuadDeclIdx = VertexDeclIndex_Null;

	GpuHandle<Buffer> cbParms;
	StateGroup stateGroup;
};

} // namespace sge
