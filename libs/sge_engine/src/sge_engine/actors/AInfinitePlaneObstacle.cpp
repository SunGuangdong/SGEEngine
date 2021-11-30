#include "AInfinitePlaneObstacle.h"
#include "sge_engine/Physics.h"

namespace sge {
// clang-format off
ReflAddTypeId(AInfinitePlaneObstacle, 21'02'27'0001);
ReflBlock()
{
	ReflAddActor(AInfinitePlaneObstacle)
		ReflMember(AInfinitePlaneObstacle, displayScale).uiRange(0.001f, 10000.f, 0.1f)
		ReflMember(AInfinitePlaneObstacle, rbPropConfig)
	;
}
// clang-format on

AABox3f AInfinitePlaneObstacle::getBBoxOS() const {
	return AABox3f();
}

void AInfinitePlaneObstacle::create() {
	registerTrait(ttRigidBody);
	ttRigidBody.getRigidBody()->create(this, CollsionShapeDesc::createInfinitePlane(vec3f::axis_y(), 0.f), 0.f, false);

	// Disable making the rigid body dynamic. This Actor represents a static object.
	rbPropConfig.extractPropsFromRigidBody(*ttRigidBody.getRigidBody());
	rbPropConfig.dontShowDynamicProperties = true;
}

void AInfinitePlaneObstacle::onPlayStateChanged(bool const isStartingToPlay) {
	Actor::onPlayStateChanged(isStartingToPlay);

	if (isStartingToPlay) {
		rbPropConfig.applyProperties(*this);
	}
}

} // namespace sge
