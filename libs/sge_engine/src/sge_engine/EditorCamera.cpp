#include "EditorCamera.h"
#include "sge_core/application/input.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

bool EditorCamera::update(const InputState& is, float aspectRatio)
{
	m_projSets.aspectRatio = aspectRatio;
	bool const updated = m_orbitCamera.update(
	    is.IsKeyDown(Key_LAlt),
	    is.IsKeyDown(Key_MouseLeft),
	    is.IsKeyDown(Key_MouseMiddle),
	    is.IsKeyDown(Key_MouseRight),
	    is.GetCursorPos());

	m_camPos = m_orbitCamera.eyePosition();
	if (isOrthograhpic) {
		orthoCoeff += 5.f / 60.f;

		m_proj = mat4f::getOrthoRHCentered(
		    m_orbitCamera.radius * aspectRatio,
		    m_orbitCamera.radius,
		    m_projSets.near,
		    m_projSets.far,
		    kIsTexcoordStyleD3D);
	}
	else {
		orthoCoeff -= 5.f / 60.f;
		m_proj = mat4f::getPerspectiveFovRH(
		    m_projSets.fov, m_projSets.aspectRatio, m_projSets.near, m_projSets.far, 0.f, kIsTexcoordStyleD3D);
	}
	mat4f o = mat4f::getOrthoRHCentered(
	    m_orbitCamera.radius * aspectRatio, m_orbitCamera.radius, m_projSets.near, m_projSets.far, kIsTexcoordStyleD3D);
	mat4f p = mat4f::getPerspectiveFovRH(
	    m_projSets.fov, m_projSets.aspectRatio, m_projSets.near, m_projSets.far, 0.f, kIsTexcoordStyleD3D);

	orthoCoeff = clamp(orthoCoeff, 0.f, 1.f);
	float k = 1.f - pow(1.f - orthoCoeff, 3.f);
	;
	for (int t = 0; t < 4; ++t) {
		m_proj.data[t] = lerp(p.data[t], o.data[t], k);
	}

	m_view = m_orbitCamera.GetViewMatrix();
	m_projView = m_proj * m_view;

	m_frustum = Frustum::extractClippingPlanes(m_projView, kIsTexcoordStyleD3D);

	return updated;
}


} // namespace sge
