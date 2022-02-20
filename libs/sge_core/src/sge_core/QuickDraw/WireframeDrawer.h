#pragma once

#include "sge_core/GeomGen.h"
#include "sge_renderer/renderer/renderer.h"

#include <vector>

namespace sge {


struct SGE_CORE_API WireframeDrawer : public NoCopy {
	void create(SGEContext* sgecon);

	/// These methods add the specified primitive to the rendering queue.
	/// The queue is executed(and then cleared by) @drawWired_Execute.
	void drawWiredAdd_Line(const vec3f& a, const vec3f& b, const uint32 rgba);
	void drawWiredAdd_Arrow(const vec3f& from, const vec3f& to, const uint32 rgba);
	void drawWiredAdd_EllipseXZ(const mat4f& transformMtx, float xSize, float ySize, const uint32 rgba);
	void drawWiredAdd_Box(const mat4f& world, const uint32 rgba);
	void drawWiredAdd_Box(const Box3f& aabb, const uint32 rgba);
	void drawWiredAdd_Box(const mat4f& world, const Box3f& aabb, const uint32 rgba);
	void drawWiredAdd_Sphere(const mat4f& world, const uint32 rgba, float radius, int numSides = 3);
	void drawWiredAdd_Capsule(const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_Cylinder(const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_ConeBottomAligned(
	    const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_Basis(const mat4f& world);
	void drawWiredAdd_Bone(const mat4f& n2w, float length, float radius, const vec4f& color = vec4f(1.f));
	void drawWiredAdd_Grid(
	    const vec3f& origin,
	    const vec3f& xAxis,
	    const vec3f& zAxis,
	    const int xLines,
	    const int yLines, // Then number of lines in x and z axis.
	    const int color = 0x000000);
	void drawWiredAdd_triangle(const vec3f& a, const vec3f& b, const vec3f& c, const int color = 0x000000);

	// Removes all curretly queued primitives for drawing.
	void drawWired_Clear();
	void drawWired_Execute(
	    const RenderDestination& rdest,
	    const mat4f& projViewWorld,
	    BlendState* blendState = nullptr,
	    DepthStencilState* dss = nullptr);

  private:
	std::vector<GeomGen::PosColorVert> m_wireframeVerts;
	GpuHandle<Buffer> m_vbWiredGeometry;
	StateGroup stateGroup;

	VertexDeclIndex vertexDeclIndex_pos3d_rgba_int = VertexDeclIndex_Null;
	GpuHandle<ShadingProgram> m_effect3DVertexColored;

	GpuHandle<RasterizerState> m_rsDefault;
	GpuHandle<DepthStencilState> m_dsDefault;
};


} // namespace sge
