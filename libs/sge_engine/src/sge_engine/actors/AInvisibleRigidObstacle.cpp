#include "AInvisibleRigidObstacle.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"

namespace sge {

ReflAddTypeId(AInvisibleRigidObstacle, 20'08'17'0001);
ReflBlock() {
	ReflAddActor(AInvisibleRigidObstacle);
}

AABox3f AInvisibleRigidObstacle::getBBoxOS() const {
	return kBBoxObjSpace;
}

void AInvisibleRigidObstacle::create() {
	registerTrait(m_traitRB);
	registerTrait(m_traitViewportIcon);

	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/AInvisibleRigidObstacle.png", true);
	m_traitViewportIcon.setObjectSpaceOffset(getBBoxOS().halfDiagonal());

	m_traitRB.getRigidBody()->create(this, CollsionShapeDesc::createBox(kBBoxObjSpace), 0.f, false);
}

} // namespace sge
