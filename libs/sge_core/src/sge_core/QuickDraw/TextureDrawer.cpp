#include "TextureDrawer.h"
#include "sge_core/ICore.h"

namespace sge {

const char EFFECT_2D_UBERSHADER[] = R"(
//----------------------------------------
// Vertex Shader.
//----------------------------------------
uniform float4x4 projViewWorld;
uniform float alphaMult;

#ifdef COLOR_TEXTURE
	uniform float4 uvRegion; // xy - top-left, zw - bottom-right.
	uniform sampler2D colorTexture;
	#ifdef COLOR_TEXTURE_TEXT_MODE
		uniform float4 colorText;
	#endif
#else
	uniform float4 color;
#endif

struct VERTEX_IN {
	float2 a_position : a_position;
#ifdef COLOR_TEXTURE
	float2 a_uv : a_uv;
#endif
};

struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
#ifdef COLOR_TEXTURE
	float2 v_uv : v_uv;
#endif
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

#ifdef COLOR_TEXTURE
	OUT.v_uv = IN.a_uv;
#endif
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 0.0, 1.0));

	return OUT;
}

//----------------------------------------
// Pixel shader.
//----------------------------------------
float4 psMain(VERTEX_OUT IN) : COLOR {
#ifdef COLOR_TEXTURE
	float2 sampleUV;
	sampleUV.x = uvRegion.x + (uvRegion.z - uvRegion.x) * IN.v_uv.x;
	sampleUV.y = uvRegion.y + (uvRegion.w - uvRegion.y) * IN.v_uv.y;
	float4 c0 = tex2D(colorTexture, sampleUV);

	c0.w = c0.w * alphaMult;
	return c0;
#else
	float4 c0 = color;
	c0.w = c0.w * alphaMult;
	return c0;
#endif
}
)";


void TextureDrawer::create(SGEContext* sgecon)
{
	SGEDevice* sgedev = sgecon->getDevice();

	std::vector<Vertex2D> vbData;

	// Define the vertices for rectangle TriList CCW (bot-right, top-left)
	{
		m_rect2DShapeInfo.vertexOffset = (int)vbData.size();

		vbData.push_back(Vertex2D(vec2f(-1.f, 1.f), vec2f(0.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, 1.f), vec2f(1.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, -1.f), vec2f(1.f, 0.f)));

		vbData.push_back(Vertex2D(vec2f(1.f, -1.f), vec2f(1.f, 0.f)));
		vbData.push_back(Vertex2D(vec2f(-1.f, -1.f), vec2f(0.f, 0.f)));
		vbData.push_back(Vertex2D(vec2f(-1.f, 1.f), vec2f(0.f, 1.f)));

		m_rect2DShapeInfo.numPoints = int(vbData.size()) - m_rect2DShapeInfo.vertexOffset;
	}

	// Define the vertices for equilateral triangle pointing toward +X
	{
		m_triLeftShapeInfo.vertexOffset = int(vbData.size());

		vbData.push_back(Vertex2D(vec2f(0.f, 1.f) - vec2f(0.5f), vec2f(0.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, 0.5f) - vec2f(0.5f), vec2f(1.f, 0.5f)));
		vbData.push_back(Vertex2D(vec2f(0.f, 0.f) - vec2f(0.5f), vec2f(0.f, 0.f)));

		m_triLeftShapeInfo.numPoints = int(vbData.size()) - m_triLeftShapeInfo.vertexOffset;
	}

	// Create the vertex buffer that holds the shapes devided above.
	m_vb2d = sgedev->requestResource<Buffer>();
	const BufferDesc vbd2D = BufferDesc::GetDefaultVertexBuffer(vbData.size() * sizeof(Vertex2D));
	m_vb2d->create(vbd2D, vbData.data());

	// The 2D vertex declaration
	const VertexDecl vtxDecl_pos2d_uv[] = {
	    {0, "a_position", UniformType::Float2, 0},
	    {0, "a_uv", UniformType::Float2, 8},
	};

	vertexDeclIndex_pos2d_uv = sgedev->getVertexDeclIndex(vtxDecl_pos2d_uv, SGE_ARRSZ(vtxDecl_pos2d_uv));

	m_effect2DColored = sgedev->requestResource<ShadingProgram>();
	m_effect2DColored->createFromCustomHLSL(EFFECT_2D_UBERSHADER, EFFECT_2D_UBERSHADER);

	m_effect2DTextured = sgedev->requestResource<ShadingProgram>();
	const std::string effect2DTexturedCode = std::string("#define COLOR_TEXTURE\n") + EFFECT_2D_UBERSHADER;
	m_effect2DTextured->createFromCustomHLSL(effect2DTexturedCode.c_str(), effect2DTexturedCode.c_str());
}

void TextureDrawer::drawRect(
    const RenderDestination& rdest, const Box2f& boxPixels, const vec4f& rgba, BlendState* blendState)
{
	const vec2f boxSize = boxPixels.size();
	drawRect(rdest, boxPixels.min.x, boxPixels.min.y, boxSize.x, boxSize.y, rgba, blendState);
}

void TextureDrawer::drawRect(
    const RenderDestination& rdest,
    float xPixels,
    float yPixels,
    float width,
    float height,
    const vec4f& rgba,
    BlendState* blendState)
{
	const mat4f sizeScaling = mat4f::getScaling(width / 2.f, height / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(xPixels + width / 2.f, yPixels + height / 2.f, 0.f);
	const mat4f world = transl * sizeScaling;
	const mat4f ortho =
	    mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;
	float alphaMult = 1.f;

	GpuHandle<RasterizerState> rs = getCore()->getGraphicsResources().RS_default;
	GpuHandle<DepthStencilState> dss = getCore()->getGraphicsResources().DSS_default_lessEqual;

	stateGroup.setRenderState(rs, dss, blendState);
	stateGroup.setProgram(m_effect2DColored);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("color", ShaderType::PixelShader), (void*)&rgba),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	};

	DrawCall dc;

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_rect2DShapeInfo.numPoints, m_rect2DShapeInfo.vertexOffset);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void TextureDrawer::drawTriLeft(
    const RenderDestination& rdest, const Box2f& boxPixels, float rotation, const vec4f& rgba, BlendState* blendState)
{
	const vec2f size = boxPixels.size();

	const mat4f sizeScaling = mat4f::getScaling(size.x / 2.f, size.y / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(boxPixels.min.x + size.x / 2.f, boxPixels.min.y + size.y / 2.f, 0.f);
	const mat4f world = transl * mat4f::getRotationZ(rotation) * sizeScaling;
	const mat4f ortho =
	    mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;
	float alphaMult = 1.f;

	GpuHandle<RasterizerState> rs = getCore()->getGraphicsResources().RS_default;
	GpuHandle<DepthStencilState> dss = getCore()->getGraphicsResources().DSS_default_lessEqual;

	stateGroup.setRenderState(rs, dss, blendState);
	stateGroup.setProgram(m_effect2DColored);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("color", ShaderType::PixelShader), (void*)&rgba),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	};

	DrawCall dc;

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_triLeftShapeInfo.numPoints, m_triLeftShapeInfo.vertexOffset);

	rdest.executeDrawCall(dc);
}

void TextureDrawer::drawRectTexture(
    const RenderDestination& rdest,
    float xPixels,
    float yPixels,
    float width,
    float height,
    Texture* texture,
    BlendState* blendState,
    vec2f topUV,
    vec2f bottomUV,
    float alphaMult)
{
	if (texture && !texture->isValid()) {
		return;
	}

	vec4f region(topUV, bottomUV);

	const mat4f sizeScaling = mat4f::getScaling(width / 2.f, height / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(xPixels + width / 2.f, yPixels + height / 2.f, 0.f);
	const mat4f world = transl * sizeScaling;
	const mat4f ortho =
	    mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;

	GpuHandle<RasterizerState> rs = getCore()->getGraphicsResources().RS_default;
	GpuHandle<DepthStencilState> dss = getCore()->getGraphicsResources().DSS_default_lessEqual;

	stateGroup.setRenderState(rs, dss, blendState);
	stateGroup.setProgram(m_effect2DTextured);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	DrawCall dc;
	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();

	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("uvRegion", ShaderType::PixelShader), (void*)&region),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	    BoundUniform(refl.textures.findUniform("colorTexture", ShaderType::PixelShader), texture),
	};

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_rect2DShapeInfo.numPoints, m_rect2DShapeInfo.vertexOffset);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void TextureDrawer::drawRectTexture(
    const RenderDestination& rdest,
    const Box2f& boxPixels,
    Texture* texture,
    BlendState* blendState,
    vec2f topUV,
    vec2f bottomUV,
    float alphaMult)
{
	vec2f size = boxPixels.size();
	drawRectTexture(
	    rdest, boxPixels.min.x, boxPixels.min.y, size.x, size.y, texture, blendState, topUV, bottomUV, alphaMult);
}

void TextureDrawer::drawTexture(
    const RenderDestination& rdest,
    float xPixels,
    float yPixels,
    float width,
    Texture* texture,
    BlendState* blendState,
    vec2f topUV,
    vec2f bottomUV,
    float alphaMult)
{
	if (!texture || !texture->isValid()) {
		return;
	}

	const TextureDesc desc = texture->getDesc();

	float aspectRatio = 0.f;
	if (desc.textureType == UniformType::Texture1D)
		aspectRatio = float(desc.texture1D.width) / 1.f;
	if (desc.textureType == UniformType::Texture2D)
		aspectRatio = float(desc.texture2D.width) / float(desc.texture2D.height);
	else {
		sgeAssert(false);
	}

	const float height = floorf(width / aspectRatio);

	drawRectTexture(rdest, xPixels, yPixels, width, height, texture, blendState, topUV, bottomUV, alphaMult);
}


} // namespace sge
