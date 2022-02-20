#include "sge_core/QuickDraw/WireframeDrawer.h"
#include "sge_utils/math/color.h"

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

namespace sge {

void WireframeDrawer::create(SGEContext* sgecon)
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

	m_vbWiredGeometry = sgedev->requestResource<Buffer>();
	const BufferDesc vbdWiredGeom =
	    BufferDesc::GetDefaultVertexBuffer(wiredVertexBufferSizeBytes, ResourceUsage::Dynamic);
	m_vbWiredGeometry->create(vbdWiredGeom, NULL);
}

void WireframeDrawer::drawWiredAdd_Line(const vec3f& a, const vec3f& b, const uint32 rgba)
{
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, rgba));
}

void WireframeDrawer::drawWiredAdd_Arrow(const vec3f& from, const vec3f& to, const uint32 rgba)
{
	const vec3f diff = to - from;
	vec3f ax;
	if (isEpsEqual(diff.x, diff.z)) {
		ax = vec3f(diff.x, diff.z, -diff.y);
	}
	else {
		ax = vec3f(-diff.z, diff.y, diff.x);
	}

	const vec3f perp0 = cross(diff, ax).normalized() * diff.length() * 0.15f;
	const vec3f perp1 = quatf::getAxisAngle(diff.normalized(), half_pi()).transformDir(perp0);

	const vec3f toBack = diff * -0.15f;

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from + perp0, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from - perp0, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from + perp1, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from - perp1, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to + perp0 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to - perp0 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to + perp1 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to - perp1 + toBack, rgba));
}

void WireframeDrawer::drawWiredAdd_EllipseXZ(const mat4f& transformMtx, float xSize, float ySize, const uint32 rgba)
{
	const int numSegments = 32;
	const float segmentAngle = two_pi() / float(numSegments);

	// Construct a 2x2 matrix represnting a rotation between segment points.
	// column 0
	float m00 = cosf(segmentAngle);
	float m01 = -sinf(segmentAngle);

	// column 1
	float m10 = sinf(segmentAngle);
	float m11 = cosf(segmentAngle);

	vec3f arm = vec3f(1.f, 0.f, 0.f);
	vec3f armScaledPrev = vec3f(0.f);
	for (int iSeg = 0; iSeg < (numSegments + 1); ++iSeg) {
		vec3f armScaled = arm;
		armScaled.x *= xSize;
		armScaled.z *= ySize;

		armScaled = mat_mul_pos(transformMtx, armScaled);

		if (iSeg > 0) {
			m_wireframeVerts.push_back(GeomGen::PosColorVert(armScaledPrev, rgba));
			m_wireframeVerts.push_back(GeomGen::PosColorVert(armScaled, rgba));
		}

		// Rotate the arm.
		arm = vec3f(arm.x * m00 + arm.z * m01, 0.f, arm.x * m10 + arm.z * m11);
		armScaledPrev = armScaled;
	}
}

void WireframeDrawer::drawWiredAdd_Box(const mat4f& world, const uint32 rgba)
{
	GeomGen::wiredBox(m_wireframeVerts, world, rgba);
}

void WireframeDrawer::drawWiredAdd_Box(const Box3f& aabb, const uint32 rgba)
{
	GeomGen::wiredBox(m_wireframeVerts, aabb, rgba);
}

void WireframeDrawer::drawWiredAdd_Box(const mat4f& world, const Box3f& aabb, const uint32 rgba)
{
	const size_t newBoxStart = m_wireframeVerts.size();

	GeomGen::wiredBox(m_wireframeVerts, aabb, rgba);
	for (size_t t = newBoxStart; t < m_wireframeVerts.size(); ++t) {
		m_wireframeVerts[t].pt = (world * vec4f(m_wireframeVerts[t].pt, 1.f)).xyz();
	}
}

void WireframeDrawer::drawWiredAdd_Capsule(
    const mat4f& world, const uint32 rgba, float height, float radius, int numSides)
{
	GeomGen::wiredCapsule(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::center);
}

void WireframeDrawer::drawWiredAdd_Sphere(const mat4f& world, const uint32 rgba, float radius, int numSides)
{
	GeomGen::wiredSphere(m_wireframeVerts, world, rgba, radius, numSides);
}

void WireframeDrawer::drawWiredAdd_Cylinder(
    const mat4f& world, const uint32 rgba, float height, float radius, int numSides)
{
	GeomGen::wiredCylinder(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::center);
}

void WireframeDrawer::drawWiredAdd_ConeBottomAligned(
    const mat4f& world, const uint32 rgba, float height, float radius, int numSides)
{
	GeomGen::wiredCone(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::Bottom);
}

void WireframeDrawer::drawWiredAdd_Basis(const mat4f& world)
{
	GeomGen::wiredBasis(m_wireframeVerts, world);
}

void WireframeDrawer::drawWiredAdd_Bone(const mat4f& n2w, float length, float radius, const vec4f& color)
{
	const int startVertex = int(m_wireframeVerts.size());

	int intColor = colorToIntRgba(color);
	// create the Bone oriented along the X-axis.

	const float midX = length * 0.3f;

	// 1st small pyramid
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));

	// The middle ring.
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));

	// 2nd bigger pyramid.
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	// Now transform vertices to world space using the provide matrix.
	for (int t = startVertex; t < int(m_wireframeVerts.size()); ++t) {
		m_wireframeVerts[t].pt = mat_mul_pos(n2w, m_wireframeVerts[t].pt);
	}
}

void WireframeDrawer::drawWiredAdd_Grid(
    const vec3f& origin, const vec3f& xAxis, const vec3f& zAxis, const int xLines, const int yLines, const int color)
{
	GeomGen::wiredGrid(m_wireframeVerts, origin, xAxis, zAxis, xLines, yLines, color);
}

void WireframeDrawer::drawWiredAdd_triangle(const vec3f& a, const vec3f& b, const vec3f& c, const int color)
{
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(c, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(c, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, color));
}

void WireframeDrawer::drawWired_Clear()
{
	m_wireframeVerts.clear();
}

void WireframeDrawer::drawWired_Execute(
    const RenderDestination& rdest, const mat4f& projView, BlendState* blendState, DepthStencilState* dss)
{
	if (m_wireframeVerts.size() == 0) {
		return;
	}

	sgeAssert(m_wireframeVerts.size() % 2 == 0);

	stateGroup.setRenderState(m_rsDefault, dss ? dss : m_dsDefault.GetPtr(), blendState);
	stateGroup.setProgram(m_effect3DVertexColored);
	stateGroup.setVB(0, m_vbWiredGeometry, 0, sizeof(GeomGen::PosColorVert));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos3d_rgba_int);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::LineList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&projView),
	};

	const uint32 vbSizeVerts = uint32(m_vbWiredGeometry->getDesc().sizeBytes) / (sizeof(GeomGen::PosColorVert));

	while (m_wireframeVerts.size()) {
		const uint32 numVertsToCopy =
		    (m_wireframeVerts.size() > vbSizeVerts) ? vbSizeVerts : uint32(m_wireframeVerts.size());

		GeomGen::PosColorVert* const vbdata =
		    (GeomGen::PosColorVert*)rdest.sgecon->map(m_vbWiredGeometry, Map::WriteDiscard);
		std::copy(m_wireframeVerts.begin(), m_wireframeVerts.begin() + numVertsToCopy, vbdata);
		rdest.sgecon->unMap(m_vbWiredGeometry);

		m_wireframeVerts.erase(m_wireframeVerts.begin(), m_wireframeVerts.begin() + numVertsToCopy);

		DrawCall dc;

		dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
		dc.setStateGroup(&stateGroup);
		dc.draw(numVertsToCopy, 0);

		rdest.executeDrawCall(dc);
	}

	drawWired_Clear();
}


} // namespace sge
