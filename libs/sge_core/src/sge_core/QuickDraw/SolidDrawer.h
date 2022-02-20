#pragma once

#include "sge_core/GeomGen.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct SGE_CORE_API SolidDrawer : public NoCopy{
	void create(SGEContext* sgecon);

	void drawSolidAdd_Triangle(const vec3f a, const vec3f b, const vec3f c, const uint32 rgba);
	void drawSolidAdd_Quad(const vec3f& origin, const vec3f& ex, const vec3f& ey, const uint32 rgba);
	void drawSolidAdd_QuadCentered(const vec3f& center, const vec3f& exHalf, const vec3f& eyHalf, const uint32 rgba);

	void drawSolid_Execute(
	    const RenderDestination& rdest,
	    const mat4f& projViewWorld,
	    bool shouldUseCulling = true,
	    BlendState* blendState = nullptr);

  private:
	std::vector<GeomGen::PosColorVert> m_solidColorVerts;
	GpuHandle<Buffer> m_vbSolidColorGeometry;

	VertexDeclIndex vertexDeclIndex_pos3d_rgba_int = VertexDeclIndex_Null;

	GpuHandle<Buffer> m_vb3d;
	GpuHandle<ShadingProgram> m_effect3DVertexColored;

	StateGroup stateGroup;
};


} // namespace sge
