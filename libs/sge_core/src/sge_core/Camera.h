#pragma once

#include "sge_utils/math/Frustum.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/math/vec2i.h"
#include "sgecore_api.h"

namespace sge {

struct InputState;

// Currently only perspective projection..
struct CameraProjectionSettings {
	CameraProjectionSettings() = default;

	float fov = 0.f;
	float aspectRatio = 0.f;
	float near = 0.f;
	float far = 0.f;
	vec2i canvasSize = vec2i(0);
};

struct SGE_CORE_API ICamera {
	ICamera() = default;
	virtual ~ICamera() = default;

	virtual vec3f getCameraPosition() const = 0;
	virtual vec3f getCameraLookDir() const = 0;
	virtual mat4f getView() const = 0;
	virtual mat4f getProj() const = 0;
	virtual mat4f getProjView() const = 0;

	/// Returns the frustum planes in world space.
	/// May be nullptr if not applicable.
	/// The pointer is readonly and managed by the ICamera interface.
	virtual const Frustum* getFrustumWS() const = 0;

	Ray perspectivePickWs(const vec2f& cursorUv) const;
};

/// @brief The most simple implementation of the ICamera interface.
/// supoorting matrix-based projection cameras (well these are the only one supported).
struct SGE_CORE_API RawCamera : public ICamera {
	RawCamera() = default;
	RawCamera(const vec3f& camPos, const mat4f& view, const mat4f& proj);

  public:
	vec3f m_camPos = vec3f(0.f);
	mat4f m_view = mat4f::getIdentity();
	mat4f m_proj = mat4f::getIdentity();
	mat4f m_projView = mat4f::getIdentity();
	Frustum m_frustum;

	vec3f getCameraPosition() const final { return m_camPos; }
	virtual vec3f getCameraLookDir() const final { return -getView().getRow(2).xyz(); }
	mat4f getView() const final { return m_view; }
	mat4f getProj() const final { return m_proj; }
	mat4f getProjView() const final { return m_projView; }
	const Frustum* getFrustumWS() const final { return &m_frustum; }
};

} // namespace sge
