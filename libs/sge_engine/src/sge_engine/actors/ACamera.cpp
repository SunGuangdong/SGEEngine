#include "ACamera.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/typelibHelper.h"

namespace sge {

// clang-format off
ReflAddTypeId(PerspectiveCameraSettings, 20'03'01'0018);
ReflAddTypeId(CameraTraitCamera, 20'03'01'0019);
ReflAddTypeId(ACamera, 20'03'01'0020);

ReflBlock()
{
	ReflAddType(PerspectiveCameraSettings)
	ReflMember(PerspectiveCameraSettings, orthoGraphicMode)
	ReflMember(PerspectiveCameraSettings, nearPlane)
	ReflMember(PerspectiveCameraSettings, farPlane)
	ReflMember(PerspectiveCameraSettings, fov)
		.addMemberFlag(MFF_FloatAsDegrees)
	ReflMember(PerspectiveCameraSettings, orthographicWidth)
	ReflMember(PerspectiveCameraSettings, orthographicHeight)
	ReflMember(PerspectiveCameraSettings, heightShift).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
	ReflMember(PerspectiveCameraSettings, orthographicMantainRatio)
;
	
	ReflAddType(CameraTraitCamera)
		ReflMember(CameraTraitCamera, m_cameraSettings)
	;

	ReflAddActor(ACamera)
		ReflMember(ACamera, m_traitCamera)
	;
}
// clang-format on

//---------------------------------------------------------------
//
//---------------------------------------------------------------
void CameraTraitCamera::update(const GameUpdateSets& UNUSED(updateSets)) {
	GameWorld* const world = getWorld();

	const CameraProjectionSettings& projSets = world->userProjectionSettings;
	m_proj = m_cameraSettings.calcMatrix(projSets.aspectRatio);
	m_view = getActor()->getTransform().toMatrix().inverse();
	m_view = m_view;
	m_projView = m_proj * m_view;
	m_cachedFrustumWS = Frustum::extractClippingPlanes(m_projView, kIsTexcoordStyleD3D);
}

//---------------------------------------------------------------
//
//---------------------------------------------------------------
ACamera::ACamera() {
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ACamera.png", true);
}

void ACamera::create() {
	registerTrait(m_traitCamera);
}

Box3f ACamera::getBBoxOS() const {
	return Box3f::getFromHalfDiagonal(vec3f(1.f, 1.f, 1.f));
}


void ACamera::update(const GameUpdateSets& updateSets) {
	m_traitCamera.update(updateSets);
}

} // namespace sge
