#include "AStaticObstacle.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------

// clang-format off
//ReflAddTypeId(CollisionShapeSource, 20'03'02'0005);
//ReflBlock() {
//	ReflAddType(CollisionShapeSource) ReflEnumVal((int)CollisionShapeSource::FromBoundingBox, "FromBoundingBox")
//	ReflEnumVal((int)CollisionShapeSource::FromConcaveHulls, "FromConcaveHulls")
//	ReflEnumVal((int)CollisionShapeSource::FromConvexHulls, "FromConvexHulls");
//}

ReflAddTypeId(AStaticObstacle, 20'03'02'0006);
ReflBlock() {
	ReflAddActor(AStaticObstacle)
		ReflMember(AStaticObstacle, m_traitModel)
		ReflMember(AStaticObstacle, m_traitSprite)
	;
}
// clang-format on

Box3f AStaticObstacle::getBBoxOS() const
{
	Box3f bbox = m_traitModel.getBBoxOS();
	bbox.expand(m_traitSprite.getBBoxOS());
	return bbox;
}

void AStaticObstacle::create()
{
	registerTrait(m_traitRB);
	registerTrait(m_traitModel);
	registerTrait(m_traitSprite);

	m_traitModel.uiDontOfferResizingModelCount = false;
	m_traitModel.m_models.resize(1);
	m_traitModel.m_models[0].m_assetProperty.setAsset("assets/editor/models/box.mdl");
}

void AStaticObstacle::update(const GameUpdateSets& UNUSED(updateSets))
{
	m_traitSprite.postUpdate();

	if (m_traitModel.postUpdate()) {
		if (m_traitRB.getRigidBody()->isValid()) {
			this->getWorld()->physicsWorld.removePhysicsObject(*m_traitRB.getRigidBody());
			m_traitRB.getRigidBody()->destroy();
		}

		std::vector<CollsionShapeDesc> shapeDescs;
		addCollisionShapeBasedOnTraitModel(shapeDescs, m_traitModel);
		if (shapeDescs.empty() == false) {
			m_traitRB.getRigidBody()->create((Actor*)this, shapeDescs.data(), int(shapeDescs.size()), 0.f, false);

			// CAUTION: this looks a bit hacky but it is used to set the physics scaling.
			// TODO: Check if it would be better if we explicitly set it here.
			setTransform(getTransform(), true);
			getWorld()->physicsWorld.addPhysicsObject(*m_traitRB.getRigidBody());
		}
		else {
			sgeLogError("Static obstacle failed to create rigid body, the shape isn't valid!\n");
		}
	}
}

} // namespace sge
