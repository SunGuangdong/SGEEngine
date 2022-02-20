#include "SolidDrawer.h"
#include "sge_core/ICore.h"

namespace sge {

const char EFFECT_3D_VERTEX_COLOR[] = R"(
//----------------------------------------
//
//----------------------------------------
uniform float4x4 projViewWorld;

struct VERTEX_IN {
	float3 a_position : a_position;
	float4 a_color : a_color;
};


struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
	float4 v_color : v_color;
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

	OUT.v_color = IN.a_color;
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 1.0));

	return OUT;
}

//----------------------------------------
//
//----------------------------------------
float4 psMain(VERTEX_OUT IN) : COLOR {
	return IN.v_color;
}
)";

void SolidDrawer::create(SGEContext* sgecon)
{
	SGEDevice* sgedev = sgecon->getDevice();

	m_effect3DVertexColored = sgedev->requestResource<ShadingProgram>();
	m_effect3DVertexColored->createFromCustomHLSL(EFFECT_3D_VERTEX_COLOR, EFFECT_3D_VERTEX_COLOR);

	const VertexDecl vtxDecl_pos3d_rgba_int[] = {
	    {0, "a_position", UniformType::Float3, 0},
	    {0, "a_color", UniformType::Int_RGBA_Unorm_IA, 12},
	};

	vertexDeclIndex_pos3d_rgba_int =
	    sgedev->getVertexDeclIndex(vtxDecl_pos3d_rgba_int, SGE_ARRSZ(vtxDecl_pos3d_rgba_int));

	// [CAUTION] Allocating big chunk of memory (almost 1MB).
	const int wiredVertexBufferSizeBytes = 65534 * sizeof(GeomGen::PosColorVert); // Around 1MB.
}

void SolidDrawer::drawSolidAdd_Triangle(const vec3f a, const vec3f b, const vec3f c, const uint32 rgba)
{
	m_solidColorVerts.push_back(GeomGen::PosColorVert(a, rgba));
	m_solidColorVerts.push_back(GeomGen::PosColorVert(b, rgba));
	m_solidColorVerts.push_back(GeomGen::PosColorVert(c, rgba));
}

void SolidDrawer::drawSolidAdd_Quad(const vec3f& origin, const vec3f& ex, const vec3f& ey, const uint32 rgba)
{
	drawSolidAdd_Triangle(origin, origin + ex, origin + ey, rgba);
	drawSolidAdd_Triangle(origin + ey, origin + ex, origin + ex + ey, rgba);
}

void SolidDrawer::drawSolidAdd_QuadCentered(
    const vec3f& center, const vec3f& exHalf, const vec3f& eyHalf, const uint32 rgba)
{
	drawSolidAdd_Quad(center - exHalf - eyHalf, 2.f * exHalf, 2.f * eyHalf, rgba);
}


void SolidDrawer::drawSolid_Execute(
    const RenderDestination& rdest, const mat4f& projViewWorld, bool shouldUseCulling, BlendState* blendState)
{
	if (m_solidColorVerts.size() == 0) {
		return;
	}

	// As we are drawing trianges, these must be multiple of 3.
	sgeAssert(m_solidColorVerts.size() % 3 == 0);

	RasterizerState* rs = shouldUseCulling ? getCore()->getGraphicsResources().RS_default
	                                       : getCore()->getGraphicsResources().RS_noCulling;
	DepthStencilState* dss = getCore()->getGraphicsResources().DSS_default_lessEqual;

	stateGroup.setRenderState(rs, dss, blendState);
	stateGroup.setProgram(m_effect3DVertexColored);
	stateGroup.setVB(0, m_vbSolidColorGeometry, 0, sizeof(GeomGen::PosColorVert));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos3d_rgba_int);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&projViewWorld),
	};

	// CAUTION: clamp the actual size to something multiple of 3
	// as we are going to draw triangles and the loop below assumes it.
	const unsigned vbSizeVerts =
	    ((uint32(m_vbSolidColorGeometry->getDesc().sizeBytes) / (sizeof(GeomGen::PosColorVert))) / 3) * 3;
	sgeAssert(vbSizeVerts % 3 == 0);

	while (m_solidColorVerts.size()) {
		const unsigned numVertsToCopy =
		    (m_solidColorVerts.size() > vbSizeVerts) ? vbSizeVerts : uint32(m_solidColorVerts.size());

		sgeAssert(numVertsToCopy % 3 == 0);

		GeomGen::PosColorVert* const vbdata =
		    (GeomGen::PosColorVert*)rdest.sgecon->map(m_vbSolidColorGeometry, Map::WriteDiscard);
		std::copy(m_solidColorVerts.begin(), m_solidColorVerts.begin() + numVertsToCopy, vbdata);
		rdest.sgecon->unMap(m_vbSolidColorGeometry);

		m_solidColorVerts.erase(m_solidColorVerts.begin(), m_solidColorVerts.begin() + numVertsToCopy);

		DrawCall dc;

		dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
		dc.setStateGroup(&stateGroup);
		dc.draw(numVertsToCopy, 0);

		rdest.executeDrawCall(dc);
	}
}


} // namespace sge
