#include "TextRender.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/Font.h"
#include "sge_core/QuickDraw/QuickDraw.h"

namespace sge {

const char TEXT_2D_SHADER[] = R"(
uniform float4x4 projViewWorld;
uniform float4 colorTint;
uniform sampler2D charactersTexture;

struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
	float2 v_uv : v_uv;
};

struct VERTEX_IN {
	float2 a_position : a_position;
	float2 a_uv : a_uv;
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

	OUT.v_uv = IN.a_uv;
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 0.f, 1.0));

	return OUT;
}

float4 psMain(VERTEX_OUT IN) : COLOR {
	float4 color = tex2D(charactersTexture, IN.v_uv);
	
	if(color.x < 0.5f) {
		discard;
	}

	color = colorTint * color.x;
	return color;
}

)";

void TextRenderer::create(SGEContext* sgecon)
{
	SGEDevice* sgedev = sgecon->getDevice();

	shader = sgedev->requestResource<ShadingProgram>();
	shader->createFromCustomHLSL(TEXT_2D_SHADER, TEXT_2D_SHADER);

	uniform_projViewWorld = shader->getReflection().findUniform("projViewWorld", ShaderType::VertexShader);
	uniform_colorTint = shader->getReflection().findUniform("colorTint", ShaderType::PixelShader);
	uniform_charactersTexture = shader->getReflection().findUniform("charactersTexture", ShaderType::PixelShader);
	uniform_charactersTexture_sampler =
	    shader->getReflection().findUniform("charactersTexture_sampler", ShaderType::PixelShader);

	VertexDecl vertexDecl[] = {
	    VertexDecl(0, "a_position", UniformType::Float2, 0),
	    VertexDecl(0, "a_uv", UniformType::Float2, sizeof(float) * 2),
	};

	blendState = sgedev->requestBlendState(BlendStateDesc());
	depthState = sgedev->requestDepthStencilState(DepthStencilDesc());
	rasterState = sgedev->requestRasterizerState(RasterDesc());

	vertexDeclIndex = sgedev->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));
}

Box2f TextRenderer::computeTextMetricsInternal(
    QuickFont& font, float fontSize, const char* asciiText, std::vector<TextVertex>* outVertices)
{
	Box2f textBbox;

	const float sizeScaling = (fontSize > 0.f) ? fontSize / (float)font.height : 1.f;

	const float invTexWidth = 1.f / float(font.texture->getDesc().texture2D.width);
	const float invTexHeight = 1.f / float(font.texture->getDesc().texture2D.height);

	vec2f nextCharacterPos = vec2f(0.f, 0.f); // Where is the baseline and x position of the next character.
	while (*asciiText != '\0') {
		if (*asciiText == '\n') {
			// Move the cursor to a new line and reset the x position.
			nextCharacterPos.x = 0.f;
			nextCharacterPos.y += sizeScaling * font.height;
			asciiText += 1;
			continue;
		}

		const stbtt_bakedchar& ch = font.cdata[*asciiText];

		// The (ch.x0, ch.y0) and (ch.x1, ch.y1)are the character region inside the texture.
		Box2f characteBox;
		characteBox.expand(vec2f(0.f));
		characteBox.expand(vec2f(float(ch.x1 - ch.x0), float(ch.y1 - ch.y0)));

		// Shift the bbox so it is aligned to the baseline of 0,0.
		characteBox.move(vec2f(0.f, ch.yoff));

		// Now apply the scaling of the font size.
		characteBox.scale(sizeScaling);

		// Now Move the bounding box to its location in the text.
		characteBox.move(nextCharacterPos);

		// Accumulate the total bounding box of the whole text.
		textBbox.expand(characteBox);

		// Create the verices.
		if (*asciiText != ' ') {
			// v3---v2
			// |2  /|
			// |  / |
			// | / 1|
			// v0---v1
			TextVertex v0, v1, v2, v3;

			v0.position = vec2f(characteBox.min.x, characteBox.max.y);
			v0.uv = vec2f(float(ch.x0) * invTexWidth, float(ch.y1) * invTexHeight);

			v1.position = vec2f(characteBox.max.x, characteBox.max.y);
			v1.uv = vec2f(float(ch.x1) * invTexWidth, float(ch.y1) * invTexHeight);

			v2.position = vec2f(characteBox.max.x, characteBox.min.y);
			v2.uv = vec2f(float(ch.x1) * invTexWidth, float(ch.y0) * invTexHeight);

			v3.position = vec2f(characteBox.min.x, characteBox.min.y);
			v3.uv = vec2f(float(ch.x0) * invTexWidth, float(ch.y0) * invTexHeight);

			if (outVertices) {
				outVertices->push_back(v0);
				outVertices->push_back(v1);
				outVertices->push_back(v2);

				outVertices->push_back(v0);
				outVertices->push_back(v2);
				outVertices->push_back(v3);
			}
		}

		// Change the next character starting position.
		nextCharacterPos.x += sizeScaling * ch.xadvance;

		asciiText++;
	}

	return textBbox;
}

void TextRenderer::drawText2d(
    QuickFont& font,
    float fontSize,
    const char* asciiText,
    const vec2f& position,
    const RenderDestination& rdest,
    vec4f colorTint,
    const Rect2s* scissors = nullptr)
{
	mat4f transform = mat4f::getTranslation(position.x, position.y, 0.f);
	drawText2d(font, fontSize, asciiText, transform, rdest, colorTint, scissors);
}

void TextRenderer::drawText2d(
    QuickFont& font,
    float fontSize,
    const char* asciiText,
    const mat4f& transform,
    const RenderDestination& rdest,
    vec4f colorTint,
    const Rect2s* scissors = nullptr)
{
	if (asciiText == nullptr) {
		return;
	}

	const mat4f ortho =
	    mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);

	vertices.clear();
	computeTextMetricsInternal(font, fontSize, asciiText, &vertices);

	if (vertices.empty()) {
		return;
	}

	// Now update the vertex buffer.
	const int neededSizeBytes = int(vertices.size() * sizeof(vertices[0]));

	const bool needsNewVertBuffer =
	    !vertexBuffer.IsResourceValid() || vertexBuffer->getDesc().sizeBytes < neededSizeBytes;

	if (needsNewVertBuffer) {
		BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(neededSizeBytes, ResourceUsage::Dynamic);
		vertexBuffer = rdest.sgecon->getDevice()->requestResource<Buffer>();
		vertexBuffer->create(vbDesc, vertices.data());
	}
	else {
		void* mappedMemory = rdest.sgecon->map(vertexBuffer, Map::WriteDiscard);
		if (mappedMemory) {
			memcpy(mappedMemory, vertices.data(), neededSizeBytes);
			rdest.sgecon->unMap(vertexBuffer);
		}
	}

	// Generate the draw call and execute it.
	stateGroup.setProgram(shader);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
	stateGroup.setVB(0, vertexBuffer.GetPtr(), 0, sizeof(TextVertex));
	stateGroup.setVBDeclIndex(vertexDeclIndex);
	stateGroup.setRenderState(
	    getCore()->getGraphicsResources().RS_noCulling,
	    getCore()->getGraphicsResources().DSS_default_lessEqual,
	    getCore()->getGraphicsResources().BS_addativeColor);

	// Bind the uniforms.
	const mat4f projViewWorld = ortho * transform;

	BoundUniform boundUnforms[] = {
	    BoundUniform(uniform_projViewWorld, (void*)&projViewWorld),
	    BoundUniform(uniform_colorTint, colorTint.data),
	    BoundUniform(uniform_charactersTexture, font.texture.GetPtr()),
	    // BoundUniform(uniform_charactersTexture_sampler, font.texture->getSamplerState()),
	};

	drawCall.setStateGroup(&stateGroup);
	drawCall.setUniforms(boundUnforms, SGE_ARRSZ(boundUnforms));
	drawCall.draw(int(vertices.size()), 0);

	rdest.sgecon->executeDrawCall(drawCall, rdest.frameTarget, &rdest.viewport, scissors);
}

} // namespace sge
