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
    QuickFont& font,
    const TextRenderer::TextDisplaySettings& displaySets,
    const char* asciiText,
    std::vector<TextVertex>* outVertices)
{
	Box2f textBbox;

	const float fontHeight = (displaySets.fontHeight > 0.f) ? displaySets.fontHeight : (float)font.height;
	const float sizeScaling = fontHeight / (float)font.height;

	const float invTexWidth = 1.f / float(font.texture->getDesc().texture2D.width);
	const float invTexHeight = 1.f / float(font.texture->getDesc().texture2D.height);

	int iCurrentLine = 0;
	float nextCharacterPosX = 0.f; // Where is the baseline and x position of the next character.
	int currentLine1stVertexIdx = outVertices ? int(outVertices->size()) : 0;
	const int text1stVertex = currentLine1stVertexIdx;

	Box2f currentLineBBox;
	while (true) {
		if (*asciiText == '\n' || *asciiText == '\0') {
			// We are switcing to a new line, apply the alignement.
			{
				switch (displaySets.horizontalAlign) {
					case horizontalAlign_left: {
						// nothing to do.
					} break;
					case horizontalAlign_middle: {
						const float halfSizeX = currentLineBBox.size().x * 0.5f;
						currentLineBBox.move(vec2f(-halfSizeX, 0.f));
						if (outVertices) {
							for (int iVert = currentLine1stVertexIdx; iVert < outVertices->size(); ++iVert) {
								outVertices->at(iVert).position.x -= halfSizeX;
							}
						}
					} break;
					case horizontalAlign_right: {
						const float sizeX = currentLineBBox.size().x;
						currentLineBBox.move(vec2f(-sizeX, 0.f));
						if (outVertices) {
							for (int iVert = currentLine1stVertexIdx; iVert < outVertices->size(); ++iVert) {
								outVertices->at(iVert).position.x -= sizeX;
							}
						}
					} break;
					default:
						sgeAssert(false);
				}
			}

			// Y alignment
			{
				switch (displaySets.verticalAlign) {
					case verticalAlign_top: {
						const float currentBaselineYPos = float(iCurrentLine) * fontHeight;
						const float yShiftToAlignment = currentLineBBox.min.y;
						const float totalMovementToAlign = currentBaselineYPos - yShiftToAlignment;

						currentLineBBox.move(vec2f(0.f, totalMovementToAlign));

						if (outVertices) {
							for (int iVert = currentLine1stVertexIdx; iVert < outVertices->size(); ++iVert) {
								outVertices->at(iVert).position.y += totalMovementToAlign;
							}
						}
					} break;
					case verticalAlign_baseLine: {
						const float currentBaselineYPos = float(iCurrentLine) * fontHeight;
						const float totalMovementToAlign = currentBaselineYPos;

						currentLineBBox.move(vec2f(0.f, totalMovementToAlign));

						if (outVertices && currentBaselineYPos != 0.f) {
							for (int iVert = currentLine1stVertexIdx; iVert < outVertices->size(); ++iVert) {
								outVertices->at(iVert).position.y += totalMovementToAlign;
							}
						}
					} break;
					case verticalAlign_middle: {
						const float currentBaselineYPos = float(iCurrentLine) * fontHeight;
						const float yShiftToAlignment = currentLineBBox.center().y;
						const float totalMovementToAlign = currentBaselineYPos - yShiftToAlignment;

						currentLineBBox.move(vec2f(0.f, totalMovementToAlign));

						if (outVertices) {
							for (int iVert = currentLine1stVertexIdx; iVert < outVertices->size(); ++iVert) {
								outVertices->at(iVert).position.y += totalMovementToAlign;
							}
						}

					} break;
					case verticalAlign_bottom: {
						if (!textBbox.IsEmpty()) {
							// Shift the prevous lines 1 height above to make space for the current line.
							textBbox.move(vec2f(0.f, -fontHeight));
						}

						// Shift the current line above, by its "overhang" below the baseline.
						const float yShiftToAlignment = -currentLineBBox.max.y;
						currentLineBBox.move(vec2f(0.f, yShiftToAlignment));

						// We shift the vertices the same way we did the bounding box.
						if (outVertices) {
							for (int iVert = text1stVertex; iVert < outVertices->size(); ++iVert) {
								if (iVert >= currentLine1stVertexIdx) {
									// Shift the prevous lines 1 height above to make space for the current line.
									outVertices->at(iVert).position.y += yShiftToAlignment;
								}
								else {
									// Move the part of the current line line that is below the baseline upwards.
									outVertices->at(iVert).position.y -= fontHeight;
								}
							}
						}


					} break;
					default:
						sgeAssert(false);
				}
			}

			// Reset the line X position.
			nextCharacterPosX = 0.f;
			currentLine1stVertexIdx = outVertices ? int(outVertices->size()) : 0;
			textBbox.expand(currentLineBBox);
			currentLineBBox = Box2f();
			iCurrentLine += 1;

			if (*asciiText == '\0') {
				// This is the while(true) exit.
				break;
			}
			else {
				asciiText += 1;
				continue;
			}
		}

		const stbtt_bakedchar& ch = font.cdata[*asciiText];

		// The (ch.x0, ch.y0) and (ch.x1, ch.y1)are the character region inside the texture.
		Box2f characteBox;
		characteBox.expand(vec2f(0.f));
		characteBox.expand(vec2f(float(ch.x1 - ch.x0), float(ch.y1 - ch.y0)));

		// Shift the bbox so it is aligned to the baseline of 0,0.
		// ch has xoffs, I'm not sure if it is really needed.
		characteBox.move(vec2f(0.f, ch.yoff));

		// Now apply the scaling of the font size.
		characteBox.scale(sizeScaling);

		// Now Move the bounding box to its location in the text.
		characteBox.move(vec2f(nextCharacterPosX, 0.f));

		// Accumulate the total bounding box of the whole text.
		currentLineBBox.expand(characteBox);

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
		nextCharacterPosX += sizeScaling * ch.xadvance;

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
	drawText2d(font, TextDisplaySettings(fontSize), asciiText, transform, rdest, colorTint, scissors);
}

/// A shortcut for @drawText2d where you specify the position of the text.
/// The text is aligned on its baseline, meaning if you pass (0,0) for position it will get
/// draw on the top left of the screen with most of the text not being visible.
void TextRenderer::drawText2d(
    QuickFont& font,
    const TextRenderer::TextDisplaySettings& displaySets,
    const char* asciiText,
    const vec2f& position,
    const RenderDestination& rdest,
    vec4f colorTint,
    const Rect2s* scissors)
{
	mat4f transform = mat4f::getTranslation(position.x, position.y, 0.f);
	drawText2d(font, displaySets, asciiText, transform, rdest, colorTint, scissors);
}

void TextRenderer::drawText2d(
    QuickFont& font,
    const TextRenderer::TextDisplaySettings& displaySets,
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
	computeTextMetricsInternal(font, displaySets, asciiText, &vertices);

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
