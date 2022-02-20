#pragma once

#include <vector>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/StaticArray.h"
#include "sge_utils/math/Box2f.h"

namespace sge {
struct QuickFont;

struct SGE_CORE_API TextRenderer : public NoCopy {
	void create(SGEContext* sgecon);

	/// A shortcut for @drawText2d where you specify the position of the text.
	/// The text is aligned on its baseline, meaning if you pass (0,0) for position it will get
	/// draw on the top left of the screen with most of the text not being visible.
	void drawText2d(
	    QuickFont& font,
	    float fontSize,
	    const char* asciiText,
	    const vec2f& position,
	    const RenderDestination& rdest,
	    vec4f colorTint,
	    const Rect2s* scissors);

	/// Draws text on the specified viewport by @rdest.
	/// The text is aligned on its baseline in its local space (starting from (0,0,0)).
	/// @param [in] fontSize, the size of the font to be used. If 0 the default font size in @font is used.
	/// @param [in] transform the transformation to be applied to the text geometry.
	void drawText2d(
	    QuickFont& font,
	    float fontSize,
	    const char* asciiText,
	    const mat4f& transform,
	    const RenderDestination& rdest,
	    vec4f colorTint,
	    const Rect2s* scissors);

	/// Computes the text bounding box of the text using the speicified parameters.
	static Box2f computeTextMetrics(QuickFont& font, float fontSize, const char* asciiText)
	{
		return computeTextMetricsInternal(font, fontSize, asciiText, nullptr);
	}

  private:
	struct TextVertex {
		vec2f position;
		vec2f uv;
	};

	/// Computes the text bounding box and optionally (if @outVertices is specified) generates the geometry data
	/// needed to render the specified text.
	/// The bounding box goes like screen coordinates - +x is right, +y is down.
	static Box2f computeTextMetricsInternal(
	    QuickFont& font, float fontSize, const char* asciiText, std::vector<TextVertex>* outVertices);

  private:
	DrawCall drawCall;
	StateGroup stateGroup;

	VertexDeclIndex vertexDeclIndex = VertexDeclIndex_Null;

	GpuHandle<ShadingProgram> shader;
	GpuHandle<BlendState> blendState;
	GpuHandle<DepthStencilState> depthState;
	GpuHandle<RasterizerState> rasterState;

	GpuHandle<Buffer> vertexBuffer;
	std::vector<TextVertex> vertices;

	BindLocation uniform_projViewWorld;
	BindLocation uniform_colorTint;
	BindLocation uniform_charactersTexture;
	BindLocation uniform_charactersTexture_sampler;
};

} // namespace sge
