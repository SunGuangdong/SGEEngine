#pragma once

#include "sge_core/Camera.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_utils/other/SimpleOrbitCamera.h"

namespace sge {

struct SGE_ENGINE_API EditorCamera : public ICamera {
	orbit_camera m_orbitCamera;
	CameraProjectionSettings m_projSets;

	bool isOrthograhpic = false;
	float orthoCoeff = 0.f;

	vec3f m_camPos = vec3f(0.f);
	mat4f m_view = mat4f::getIdentity();
	mat4f m_proj = mat4f::getIdentity();
	mat4f m_projView = mat4f::getIdentity();
	Frustum m_frustum;

	// Returns true if the the camera has changed.
	bool update(const InputState& is, float aspectRatio);

	vec3f getCameraPosition() const final { return m_camPos; }
	virtual vec3f getCameraLookDir() const final { return -getView().getRow(2).xyz(); }
	mat4f getView() const final { return m_view; }
	mat4f getProj() const final { return m_proj; }
	mat4f getProjView() const final { return m_projView; }
	const Frustum* getFrustumWS() const final { return &m_frustum; }
};

} // namespace sge
