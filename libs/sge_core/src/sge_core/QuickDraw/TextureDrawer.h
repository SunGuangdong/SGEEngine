#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box2f.h"

namespace sge {

struct SGE_CORE_API TextureDrawer : public NoCopy{
	struct Vertex2D {
		Vertex2D() = default;

		Vertex2D(sge::vec2f p, sge::vec2f uv)
		    : p(p)
		    , uv(uv)
		{
		}

		sge::vec2f p;
		sge::vec2f uv;
	};

	struct Shape2DInfo {
		int vertexOffset = 0;
		int numPoints = 0;
	};

  public:
	void create(SGEContext* sgecon);

	void drawRect(
	    const RenderDestination& rdest, const Box2f& boxPixels, const vec4f& rgba, BlendState* blendState = nullptr);

	void drawRect(
	    const RenderDestination& rdest,
	    float xPixels,
	    float yPixels,
	    float width,
	    float height,
	    const vec4f& rgba,
	    BlendState* blendState = nullptr);

	void drawTriLeft(
	    const RenderDestination& rdest,
	    const Box2f& boxPixels,
	    float rotation,
	    const vec4f& rgba,
	    BlendState* blendState = nullptr);

	void drawRectTexture(
	    const RenderDestination& rdest,
	    float xPixels,
	    float yPixels,
	    float width,
	    float height,
	    Texture* texture,
	    BlendState* blendState = nullptr,
	    vec2f topUV = vec2f(0),
	    vec2f bottomUV = vec2f(1.f),
	    float alphaMult = 1.f);


	void drawRectTexture(
	    const RenderDestination& rdest,
	    const Box2f& boxPixels,
	    Texture* texture,
	    BlendState* blendState = nullptr,
	    vec2f topUV = vec2f(0),
	    vec2f bottomUV = vec2f(1.f),
	    float alphaMult = 1.f);

	// Draws a textured quad using width and auto picking width based on the aspect ratio of the image.
	void drawTexture(
	    const RenderDestination& rdest,
	    float xPixels,
	    float yPixels,
	    float width,
	    Texture* texture,
	    BlendState* blendState = nullptr,
	    vec2f topUV = vec2f(0),
	    vec2f bottomUV = vec2f(1.f),
	    float alphaMult = 1.f);

  private:
	StateGroup stateGroup;
	GpuHandle<Buffer> m_vb2d;
	GpuHandle<ShadingProgram> m_effect2DColored;
	GpuHandle<ShadingProgram> m_effect2DTextured;
	VertexDeclIndex vertexDeclIndex_pos2d_uv = VertexDeclIndex_Null;

	Shape2DInfo m_rect2DShapeInfo;
	Shape2DInfo m_triLeftShapeInfo;
};

} // namespace sge
