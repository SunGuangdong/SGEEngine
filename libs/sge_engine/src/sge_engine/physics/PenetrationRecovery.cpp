#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/BroadphaseCollision/btDispatcher.h>
SGE_NO_WARN_END

#include "PenetrationRecovery.h"
#include "PhysicsWorldQuery.h"
#include <vector>

namespace sge {

bool computePenerationRecorverVectorFromManifold(btVector3& recoveryVector,
                                                 const btPersistentManifold* persistManifold,
                                                 const btCollisionObject* collisionObject,
                                                 float maxPenetrationDepth)
{
	bool hadPenetration = false;
	recoveryVector = btVector3(0.f, 0.f, 0.f);

	const btScalar directionSign = persistManifold->getBody0() == collisionObject ? btScalar(-1.0) : btScalar(1.0);

	// Each manifold can have multiple contact points.
	for (int p = 0; p < persistManifold->getNumContacts(); p++) {
		const btManifoldPoint& manifoldPt = persistManifold->getContactPoint(p);
		const btScalar dist = manifoldPt.getDistance();

		// If the manifold exists, and the penetration is deep enough
		// move the object, otherwise the objects are just touching.
		if (dist < -maxPenetrationDepth) {
			recoveryVector += manifoldPt.m_normalWorldOnB * directionSign * dist * btScalar(0.2);
			hadPenetration = true;
		}
	}

	return hadPenetration;
}

bool recoverFromPenetrationVector(btVector3& recoveryVector,
                                  btCollisionWorld* collisionWorld,
                                  btPairCachingGhostObject* ghostObj,
                                  btManifoldArray& tempManifoldArray,
                                  float maxPenetrationDepth)
{
	bool hadPenetration = false;
	recoveryVector = btVector3(0.f, 0.f, 0.f);

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
			    computePenerationRecorverVectorFromManifold(recoveryForThisManifold, persistManifold, ghostObj, maxPenetrationDepth);
			if (hadPenetrationWithManifold) {
				recoveryVector += recoveryForThisManifold;
				hadPenetration = true;
			}

			// In the original code from Bullet this was kept commented,
			// I do not know why, or what it does:
			// manifold->clearManifold();
		}
	}

	return hadPenetration;
}

struct FindContactCallback : public btManifoldResult {
	FindContactCallback(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
	    : btManifoldResult(body0Wrap, body1Wrap)
	{
	}

	void FindContactCallback::addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorldOnB, btScalar depth)
	{
		if (m_penetration_distance > depth) {
			const bool isSwapped = m_manifoldPtr->getBody0() != m_body0Wrap->getCollisionObject();
			m_penetration_distance = depth;
			m_other_compound_shape_index = isSwapped ? m_index0 : m_index1;
			m_pointWorld = isSwapped ? (pointInWorldOnB + (normalOnBInWorld * depth)) : pointInWorldOnB;

			m_pointNormalWorld = isSwapped ? normalOnBInWorld * -1 : normalOnBInWorld;
		}
	}

	bool hasPenetration() const { return m_penetration_distance < 0; }

	btVector3 m_pointNormalWorld;
	btVector3 m_pointWorld;
	btScalar m_penetration_distance = 0;
	int m_other_compound_shape_index = 0;
};

bool recoverFromPenetrationVector(btVector3& recoveryVector,
                                  btCollisionWorld* collisionWorld,
                                  btCollisionObject* colObj,
                                  float UNUSED(maxPenetrationDepth))
{
	recoveryVector = btVector3(0.f, 0.f, 0.f);
	bool hadPenetration = false;

	// Find the bounding box of the collision object in world space.
	btVector3 bbMinWs;
	btVector3 bbMaxWs;
	colObj->getCollisionShape()->getAabb(colObj->getWorldTransform(), bbMinWs, bbMaxWs);

	PhysicsWorldQuery::BoxTestCallback aabbCallback;
	collisionWorld->getBroadphase()->aabbTest(bbMinWs, bbMaxWs, aabbCallback);

	// For each potentially found collision object that the @colObj might penetrate
	// find the actual penetration depth.
	btCollisionObjectWrapper obA(nullptr, colObj->getCollisionShape(), colObj, colObj->getWorldTransform(), -1, -1);
	for (const btBroadphaseProxy* proxy : aabbCallback.proxies) {
		btCollisionObject* const otherCo = (btCollisionObject*)(proxy->m_clientObject);

		// The aabbTest above will report the @colObj as well. Skip it.
		if (otherCo == colObj) {
			continue;
		}
		btCollisionObjectWrapper obB(nullptr, otherCo->getCollisionShape(), otherCo, otherCo->getWorldTransform(), -1, -1);

		// Find the collision algorithm that computes the actual penetration depth.
		btCollisionAlgorithm* algorithm = collisionWorld->getDispatcher()->findAlgorithm(&obA, &obB, nullptr, BT_CONTACT_POINT_ALGORITHMS);
		if (algorithm) {
			FindContactCallback manifold(&obA, &obB);
			algorithm->processCollision(&obA, &obB, collisionWorld->getDispatchInfo(), &manifold);

			if (manifold.hasPenetration()) {
				recoveryVector += manifold.m_pointNormalWorld * -manifold.m_penetration_distance;
				hadPenetration = true;
			}

			// The algorithm has been created with placement new in bullet.
			// Call its destructor.
			algorithm->~btCollisionAlgorithm();
			collisionWorld->getDispatcher()->freeCollisionAlgorithm(algorithm);
		}
	}

	return hadPenetration;
}

} // namespace sge
