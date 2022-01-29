#include "AGhostObject.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"
#include "sge_engine/physics/PenetrationRecovery.h"

namespace sge {

void GhostAction::updateAction(btCollisionWorld* collisionWorld, btScalar UNUSED(deltaTimeStep))
{
	btPairCachingGhostObject* ghostObj = owner->m_traitRB.getRigidBody()->getBulletGhostObject();
	if (ghostObj == nullptr) {
		return;
	}

	btVector3 recoverVector;
	if (recoverFromPenetrationVector(&recoverVector, collisionWorld, ghostObj, tempManifoldArr, 0.2f)) {
		btTransform newTransform = ghostObj->getWorldTransform();
		newTransform.setOrigin(newTransform.getOrigin() + recoverVector);
		ghostObj->setWorldTransform(newTransform);

		// Not sure if the line below needs to be called, it works without it.
		// collisionWorld->updateSingleAabb(ghostObj);

		if (Actor* a = getActorFromPhysicsObjectMutable(ghostObj)) {
			a->setTransformEx(fromBullet(ghostObj->getWorldTransform()), false, false, false);
		}
	}
}

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------

ReflAddTypeId(AGhostObject, 22'01'29'0001);
ReflBlock()
{
	ReflAddActor(AGhostObject) ReflMember(AGhostObject, m_traitModel) ReflMember(AGhostObject, m_traitSprite);
}
// clang-format on

AABox3f AGhostObject::getBBoxOS() const
{
	AABox3f bbox = m_traitModel.getBBoxOS();
	bbox.expand(m_traitSprite.getBBoxOS());
	return bbox;
}

void AGhostObject::create()
{
	registerTrait(m_traitRB);
	registerTrait(m_traitModel);
	registerTrait(m_traitSprite);

	m_traitModel.uiDontOfferResizingModelCount = false;
	m_traitModel.m_models.resize(1);
	m_traitModel.m_models[0].m_assetProperty.setAsset("assets/editor/models/box.mdl");

	action.owner = this;
}

void AGhostObject::onPlayStateChanged(bool const isStartingToPlay)
{
	if (isStartingToPlay) {
		getWorld()->physicsWorld.dynamicsWorld->addAction(&action);
	}
	else {
		getWorld()->physicsWorld.dynamicsWorld->removeAction(&action);
	}
}

void AGhostObject::postUpdate(const GameUpdateSets& UNUSED(updateSets))
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
			m_traitRB.getRigidBody()->createGhost((Actor*)this, shapeDescs.data(), int(shapeDescs.size()), true);

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
