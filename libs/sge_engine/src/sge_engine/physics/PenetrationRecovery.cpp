#include "PenetrationRecovery.h"


namespace sge {

bool computePenerationRecorverVectorFromManifold(btVector3* const recoveryVector,
                                                 const btPersistentManifold* persistManifold,
                                                 const btCollisionObject* collisionObject,
                                                 float maxPenetrationDepth)
{
	bool hadPenetration = false;
	*recoveryVector = btVector3(0.f, 0.f, 0.f);

	const btScalar directionSign = persistManifold->getBody0() == collisionObject ? btScalar(-1.0) : btScalar(1.0);

	// Each manifold can have multiple contact points.
	for (int p = 0; p < persistManifold->getNumContacts(); p++) {
		const btManifoldPoint& manifoldPt = persistManifold->getContactPoint(p);
		const btScalar dist = manifoldPt.getDistance();

		// If the manifold exists, and the penetration is deep enough
		// move the object, otherwise the objects are just touching.
		if (dist < -maxPenetrationDepth) {
			*recoveryVector += manifoldPt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
			hadPenetration = true;
		}
	}

	return hadPenetration;
}

bool recoverFromPenetrationVector(btVector3* const recoveryVector,
                                  btCollisionWorld* collisionWorld,
                                  btPairCachingGhostObject* ghostObj,
                                  btManifoldArray& tempManifoldArray,
                                  float maxPenetrationDepth)
{
	bool hadPenetration = false;
	*recoveryVector = btVector3(0.f, 0.f, 0.f);

	collisionWorld->getDispatcher()->dispatchAllCollisionPairs(ghostObj->getOverlappingPairCache(), collisionWorld->getDispatchInfo(),
	                                                           collisionWorld->getDispatcher());

	// Here we must refresh the overlapping paircache as the penetrating movement itself or the
	// previous recovery iteration might have used setWorldTransform and pushed us into an object
	// that is not in the previous cache contents from the last timestep, as will happen if we
	// are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
	//
	// Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
	// paircache and the ghostobject's internal paircache at the same time.    /BW

	btCollisionShape* const collisionShape = ghostObj->getCollisionShape();
	if (collisionShape == nullptr) {
		sgeAssert(false && "Did you forget to add a collision shape to your object?");
		return false;
	}

	btVector3 minAabb, maxAabb;
	collisionShape->getAabb(ghostObj->getWorldTransform(), minAabb, maxAabb);
	collisionWorld->getBroadphase()->setAabb(ghostObj->getBroadphaseHandle(), minAabb, maxAabb, collisionWorld->getDispatcher());

	collisionWorld->getDispatcher()->dispatchAllCollisionPairs(ghostObj->getOverlappingPairCache(), collisionWorld->getDispatchInfo(),
	                                                           collisionWorld->getDispatcher());

	for (int i = 0; i < ghostObj->getOverlappingPairCache()->getNumOverlappingPairs(); i++) {
		// Clear the manifold array.
		tempManifoldArray.resize(0);

		btBroadphasePair* collisionPair = &ghostObj->getOverlappingPairCache()->getOverlappingPairArray()[i];

		btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
		btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

		// if ((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()))
		//	continue;

		// Check if the objects should collide according the bullet collision filtering.
		bool collides = (obj0->getBroadphaseHandle()->m_collisionFilterGroup & obj1->getBroadphaseHandle()->m_collisionFilterMask) != 0;
		collides = collides && (obj1->getBroadphaseHandle()->m_collisionFilterGroup & obj0->getBroadphaseHandle()->m_collisionFilterMask);
		if (!collides) {
			continue;
		}

		// Get all the contants for the overlapping pair.
		if (collisionPair->m_algorithm) {
			collisionPair->m_algorithm->getAllContactManifolds(tempManifoldArray);
		}

		// For each manifold find the movment needed to exit the penetration.
		for (int iManifold = 0; iManifold < tempManifoldArray.size(); iManifold++) {
			const btPersistentManifold* const persistManifold = tempManifoldArray[iManifold];
			if (persistManifold == nullptr) {
				continue;
			}

			btVector3 recoveryForThisManifold;
			bool hadPenetrationWithManifold =
			    computePenerationRecorverVectorFromManifold(&recoveryForThisManifold, persistManifold, ghostObj, maxPenetrationDepth);
			if (hadPenetrationWithManifold) {
				*recoveryVector += recoveryForThisManifold;
				hadPenetration = true;
			}

			// In the original code from Bullet this was kept commented,
			// I do not know why, or what it does:
			// manifold->clearManifold();
		}
	}

	return hadPenetration;
}

} // namespace sge
