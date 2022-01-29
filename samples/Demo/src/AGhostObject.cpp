#include "AGhostObject.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {

bool recoverFromPenetration(btCollisionWorld* collisionWorld, btPairCachingGhostObject* ghostObj)
{
	// TODO MOVE IT
	btManifoldArray manifoldArray;
	float maxPenetrationDepth = 0.2f;

	// Here we must refresh the overlapping paircache as the penetrating movement itself or the
	// previous recovery iteration might have used setWorldTransform and pushed us into an object
	// that is not in the previous cache contents from the last timestep, as will happen if we
	// are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
	//
	// Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
	// paircache and the ghostobject's internal paircache at the same time.    /BW

	btCollisionShape* collisionShape = ghostObj->getCollisionShape();

	btVector3 minAabb, maxAabb;
	collisionShape->getAabb(ghostObj->getWorldTransform(), minAabb, maxAabb);
	collisionWorld->getBroadphase()->setAabb(ghostObj->getBroadphaseHandle(), minAabb, maxAabb, collisionWorld->getDispatcher());

	bool penetration = false;

	collisionWorld->getDispatcher()->dispatchAllCollisionPairs(ghostObj->getOverlappingPairCache(), collisionWorld->getDispatchInfo(),
	                                                           collisionWorld->getDispatcher());

	btVector3 m_currentPosition = ghostObj->getWorldTransform().getOrigin();

	//	btScalar maxPen = btScalar(0.0);
	for (int i = 0; i < ghostObj->getOverlappingPairCache()->getNumOverlappingPairs(); i++) {
		manifoldArray.resize(0);

		btBroadphasePair* collisionPair = &ghostObj->getOverlappingPairCache()->getOverlappingPairArray()[i];

		btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
		btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

		if ((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()))
			continue;

		bool collides = (obj0->getBroadphaseHandle()->m_collisionFilterGroup & obj1->getBroadphaseHandle()->m_collisionFilterMask) != 0;
		collides = collides && (obj1->getBroadphaseHandle()->m_collisionFilterGroup & obj0->getBroadphaseHandle()->m_collisionFilterMask);
		if (!collides) {
			return false;
		}

		if (collisionPair->m_algorithm)
			collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

		for (int j = 0; j < manifoldArray.size(); j++) {
			btPersistentManifold* manifold = manifoldArray[j];
			btScalar directionSign = manifold->getBody0() == ghostObj ? btScalar(-1.0) : btScalar(1.0);
			for (int p = 0; p < manifold->getNumContacts(); p++) {
				const btManifoldPoint& pt = manifold->getContactPoint(p);

				btScalar dist = pt.getDistance();

				if (dist < -maxPenetrationDepth) {
					// TODO: cause problems on slopes, not sure if it is needed
					// if (dist < maxPen)
					//{
					//	maxPen = dist;
					//	m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

					//}
					m_currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
					penetration = true;
				}
				else {
					// printf("touching %f\n", dist);
				}
			}

			// manifold->clearManifold();
		}
	}
	btTransform newTrans = ghostObj->getWorldTransform();
	newTrans.setOrigin(m_currentPosition);
	ghostObj->setWorldTransform(newTrans);
	//	printf("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
	return penetration;
}


void GhostAction::updateAction(btCollisionWorld* collisionWorld, btScalar UNUSED(deltaTimeStep))
{
	btPairCachingGhostObject* ghostObj = owner->m_traitRB.getRigidBody()->getBulletGhostObject();
	// const btAlignedObjectArray<btCollisionObject*>& pairs = pairs->getOverlappingPairs();

	recoverFromPenetration(collisionWorld, ghostObj);
}

void GhostAction::debugDraw(btIDebugDraw* UNUSED(debugDrawer))
{
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
