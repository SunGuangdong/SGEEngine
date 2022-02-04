#include "AGhostObject.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"
#include "sge_engine/physics/PenetrationRecovery.h"

namespace sge {

void GhostAction::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
{
	btPairCachingGhostObject* ghostObj = owner->m_traitRB.getRigidBody()->getBulletGhostObject();
	if (ghostObj == nullptr) {
		return;
	}

	AGhostObject* actor = (AGhostObject*)getActorFromPhysicsObjectMutable(ghostObj);
	if (!actor) {
		return;
	}



	bool touchingGround = false;
	for (int i = 0; i < ghostObj->getOverlappingPairCache()->getNumOverlappingPairs(); i++) {
		// Clear the manifold array.
		tempManifoldArr.resize(0);

		btBroadphasePair* collisionPair = &ghostObj->getOverlappingPairCache()->getOverlappingPairArray()[i];

		// btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
		// btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

		if (collisionPair->m_algorithm) {
			collisionPair->m_algorithm->getAllContactManifolds(tempManifoldArr);
		}


		for (int iManifold = 0; iManifold < tempManifoldArr.size(); iManifold++) {
			const btPersistentManifold* const persistManifold = tempManifoldArr[iManifold];

			const float penetrationSign = persistManifold->getBody0() == ghostObj ? 1.f : -1.f;

			for (int p = 0; p < persistManifold->getNumContacts(); p++) {
				const btManifoldPoint& manifoldPt = persistManifold->getContactPoint(p);

				btVector3 n = manifoldPt.m_normalWorldOnB * penetrationSign;
				const btScalar dist = fabsf(manifoldPt.getDistance());
				if (n.y() > 0.f) {
					actor->velocity.y = 0.f;

					touchingGround = true;
				}
			}
		}
	}

	{
		btTransform newTransform = ghostObj->getWorldTransform();
		newTransform.setOrigin(newTransform.getOrigin() + toBullet(actor->velocity * deltaTimeStep));
		ghostObj->setWorldTransform(newTransform);
	}
#if 1
	btVector3 recoverVector;
	if (recoverFromPenetrationVector(&recoverVector, collisionWorld, ghostObj, 0.02f)) {
		btTransform newTransform = ghostObj->getWorldTransform();
		newTransform.setOrigin(newTransform.getOrigin() + recoverVector);
		ghostObj->setWorldTransform(newTransform);

		// Not sure if the line below needs to be called, it works without it.
		 collisionWorld->updateSingleAabb(ghostObj);

		if (recoverVector.y() > 0.f) {
			touchingGround = true;
		}
	}
#else
	btVector3 recoverVector;
	if (recoverFromPenetrationVector(&recoverVector, collisionWorld, ghostObj, tempManifoldArr, 0.02f)) {
		btTransform newTransform = ghostObj->getWorldTransform();
		newTransform.setOrigin(newTransform.getOrigin() + recoverVector);
		ghostObj->setWorldTransform(newTransform);

		// Not sure if the line below needs to be called, it works without it.
		 collisionWorld->updateSingleAabb(ghostObj);

		if (recoverVector.y() > 0.f) {
			touchingGround = true;
		}
	}
#endif

	if (!touchingGround) {
		actor->velocity.y += -30.f * deltaTimeStep;
	}
	else {
		if (actor->velocity.y < 0.f) {
			actor->velocity.y = 0.f;
		}
	}

	actor->setTransformEx(fromBullet(ghostObj->getWorldTransform()), false, false, false);
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
	m_traitModel.m_models[0].m_assetProperty.setAsset("assets/editor/models/sphere.mdl");

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

void AGhostObject::postUpdate(const GameUpdateSets& updateSets)
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

	if (updateSets.is.IsKeyPressed(Key_Space)) {
		velocity.y += 10.f;
	}

	velocity.x = updateSets.is.GetArrowKeysDir(true, false).x * 7.f;
	velocity.z = updateSets.is.GetArrowKeysDir(true, false).y * 7.f;
}

} // namespace sge
