#pragma once

#include "sge_core/AssetLibrary.h"
#include "sge_engine_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/mat4.h"

namespace sge {

struct SGE_ENGINE_API TexturedPlaneDraw {
	void draw(const RenderDestination& rdest,
	          const mat4f& projViewWorld,
	          Texture* texture,
	          const vec4f& tint,
	          const vec4f uvRegion = vec4f(0.f, 0.f, 1.f, 1.f));

	Geometry getGeometry(SGEDevice* sgedev);
	PBRMaterial getMaterial(Texture* texture) const;

  private:
	struct Vertex {
		vec3f p;
		vec3f n;
		vec2f uv;
	};

	void initialize(SGEDevice* sgedev);

	bool m_isInitialized = false;

	BindLocation m_projViewWorld_bindLoc;
	BindLocation m_texDiffuse_bindLoc;
#ifdef SGE_RENDERER_D3D11
	BindLocation m_texDiffuseSampler_bindLoc;
#endif
	BindLocation m_tint_bindLoc;
	BindLocation m_uvRegion_bindLoc;

	VertexDeclIndex m_vertexDecl = VertexDeclIndex_Null;

	GpuHandle<ShadingProgram> m_shadingProgram;
	GpuHandle<Buffer> m_vertexBuffer;

	StateGroup m_stateGroup;
	CBufferFiller uniforms;
};

} // namespace sge
