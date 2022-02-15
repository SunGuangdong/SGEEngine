#pragma once

#include <vector>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/StaticArray.h"

namespace sge {


struct DebugFont;

struct SGE_CORE_API TextRenderer {
	void create(SGEDevice* sgedev);

	void drawText(
	    const RenderDestination& rdest,
	    const mat4f& transform,
	    vec4f colorTint,
	    const char* asciiText,
	    DebugFont& font,
	    float fontSize = 0);


  private:
	struct TextVertex {
		vec2f position;
		vec2f uv;
	};

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
