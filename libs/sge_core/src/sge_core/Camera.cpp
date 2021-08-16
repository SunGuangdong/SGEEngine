#include "Camera.h"
#include "sge_core/application/input.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/RayFromProjectionMatrix.h"

namespace sge {

Ray ICamera::perspectivePickWs(const vec2f& cursorUv) const {
	Ray resultWs = rayFromProjectionMatrix(getProj(), inverse(getProj()), inverse(getView()), cursorUv);
	return resultWs;
}

RawCamera::RawCamera(const vec3f& camPos, const mat4f& view, const mat4f& proj)
    : m_camPos(camPos)
    , m_view(view)
    , m_proj(proj) {
	m_projView = proj * view;
	m_frustum = Frustum::extractClippingPlanes(m_projView, kIsTexcoordStyleD3D);
}

} // namespace sge
