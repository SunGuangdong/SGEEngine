#include "PhysicsWorldQuery.h"
#include "BulletHelper.h"
#include "PhysicsWorld.h"
#include "RigidBody.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_utils/containers/vector_set.h"

namespace sge {

void PhysicsWorldQuery::rayTest(PhysicsWorld& physWorld,
                                const vec3f& from,
                                const vec3f& to,
                                std::function<void(btDynamicsWorld::LocalRayResult&)> lambda)
{
	struct RayCallback : public btDynamicsWorld::RayResultCallback {
		RayCallback(std::function<void(btDynamicsWorld::LocalRayResult&)>& lambda)
		    : lambda(lambda)
		{
		}

		std::function<void(btDynamicsWorld::LocalRayResult&)>& lambda;

		btScalar addSingleResult(btDynamicsWorld::LocalRayResult& rayResult, bool UNUSED(normalInWorldSpace)) override
		{
			lambda(rayResult);
			return rayResult.m_hitFraction;
		}
	};

	RayCallback rayCB(lambda);
	physWorld.dynamicsWorld->rayTest(toBullet(from), toBullet(to), rayCB);
}


SGE_ENGINE_API void PhysicsWorldQuery::boxTest(PhysicsWorld& physWorld, const AABox3f& bbox, btBroadphaseAabbCallback& cb)
{
	const btVector3 bmin = toBullet(bbox.min);
	const btVector3 bmax = toBullet(bbox.max);
	physWorld.dynamicsWorld->getBroadphase()->aabbTest(bmin, bmax, cb);
}


//-------------------------------------------------------------------------
// RayResultCollisionObject
//-------------------------------------------------------------------------
btScalar RayResultCollisionObject::addSingleResult(btDynamicsWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
{
	// Caller already does the filter on the m_closestHitFraction.
	if (rayResult.m_hitFraction > m_closestHitFraction) {
		return 1.f;
	}

	if (rayResult.m_collisionObject == nullptr) {
		return 1.f;
	}

	if (m_filterFn) {
		const bool shouldSkip = m_filterFn(rayResult.m_collisionObject);
		if (shouldSkip) {
			return 1;
		}
	}

	m_hitObject = rayResult.m_collisionObject;

	m_closestHitFraction = rayResult.m_hitFraction;
	m_collisionObject = rayResult.m_collisionObject;

	m_hitNormalWorld =
	    (normalInWorldSpace) ? rayResult.m_hitNormalLocal : m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;

	m_hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, rayResult.m_hitFraction);
	return rayResult.m_hitFraction;
}

void RayResultActor::setup(const Actor* const igonreActor,
                           const btVector3& rayFromWorld,
                           const btVector3& rayToWorld,
                           std::function<bool(const Actor*)> customFilter)
{
	m_customFilter = customFilter;
	m_ignoreActor = igonreActor;
	m_rayFromWorld = rayFromWorld;
	m_rayToWorld = rayToWorld;

	m_hitActor = nullptr;
}

//-------------------------------------------------------------------------
// RayResultActor
//-------------------------------------------------------------------------
btScalar RayResultActor::addSingleResult(btDynamicsWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
{
	// Caller already does the filter on the m_closestHitFraction.
	if (rayResult.m_hitFraction > m_closestHitFraction) {
		return 1.f;
	}

	const Actor* const hitActor = getActorFromPhysicsObject(rayResult.m_collisionObject);
	if (m_ignoreActor != nullptr && m_ignoreActor == hitActor) {
		return 1.f;
	}

	if (hitActor && m_customFilter) {
		const bool shouldIgnore = m_customFilter(hitActor);
		if (shouldIgnore) {
			return 1.f;
		}
	}

	m_hitActor = getActorFromPhysicsObjectMutable(rayResult.m_collisionObject);

	m_closestHitFraction = rayResult.m_hitFraction;
	m_collisionObject = rayResult.m_collisionObject;

	m_hitNormalWorld =
	    (normalInWorldSpace) ? rayResult.m_hitNormalLocal : m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;

	m_hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, rayResult.m_hitFraction);
	return rayResult.m_hitFraction;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void findActorsOnTop(vector_map<Actor*, FindActorsOnTopResult>& result, Actor* const rootActor)
{
	GameWorld& world = *rootActor->getWorld();
	vector_set<const RigidBody*> processedRigidBodies; // TODO: make this an input to avoid allocating on every frame when it is used.

	std::function<void(const RigidBody*, bool)> processManifolds = [&](const RigidBody* const rbContactsToProcess,
	                                                                   bool parentIgnoreRotation) -> void {
		if (!rbContactsToProcess || !rbContactsToProcess->actor) {
			return;
		}

		// Check if the contacts for the current rigid body were processed.
		if (processedRigidBodies.count(rbContactsToProcess) > 0) {
			return;
		}
		processedRigidBodies.insert(rbContactsToProcess);

		for (const btPersistentManifold* const manifold : world.m_physicsManifoldList[rbContactsToProcess]) {
			if (manifold == nullptr) {
				sgeAssert(false && "Manifolds are expected to be non-null");
				continue;
			}

			int rbContactsToProcessIdx = 0;
			const btCollisionObject* const contactedCollisionObjBullet =
			    getOtherFromManifold(manifold, rbContactsToProcess->getBulletRigidBody(), &rbContactsToProcessIdx);

			const RigidBody* const contactedRigidBody = fromBullet(contactedCollisionObjBullet);
			sgeAssert(contactedRigidBody != nullptr);

			// Report only contacts to dynamic objects (mass > 0).
			if (contactedRigidBody) {
				const btRigidBody* bulletRigidBody = contactedRigidBody->getBulletRigidBody();
				if (bulletRigidBody == nullptr || bulletRigidBody->getInvMass() == 0.f) {
					continue;
				}
			}

			// Check if the contact normal is pointing upwards (towards Y) if so, concider the rigid body to be
			// placed on top.
			vec3f contactNormal = fromBullet(manifold->getContactPoint(0).m_normalWorldOnB);
			if (rbContactsToProcessIdx == 0) {
				contactNormal = -contactNormal;
			}

			if (contactNormal.y < 1e-3f) {
				continue;
			}

			// Check if the rigid body has an actor assigned (it if expected to have if it is wrapped in our RigidBody class).
			// Notice that the contacted actor here has a dynamic rigid body attached to it!
			Actor* const contactedActor = contactedRigidBody->actor;
			if (contactedActor == nullptr) {
				sgeAssert(false);
				continue;
			}

			result[contactedActor].ignoreRotation = parentIgnoreRotation;
			processManifolds(contactedRigidBody, parentIgnoreRotation);
		}

		// Now process the contacts of our children.
		const vector_set<ObjectId>* pChildern = world.getChildensOf(rbContactsToProcess->actor->getId());
		if (pChildern) {
			for (const ObjectId childId : *pChildern) {
				Actor* child = world.getActorById(childId);
				sgeAssert(child != nullptr);
				if (child) {
					TraitRigidBody* childRBTrait = getTrait<TraitRigidBody>(child);
					if (childRBTrait) {
						processManifolds(childRBTrait->getRigidBody(), parentIgnoreRotation);
					}
				}
			}
		}
	};

	TraitRigidBody* rbTrait = getTrait<TraitRigidBody>(rootActor);
	if (rbTrait) {
		processManifolds(rbTrait->getRigidBody(), false);
	}

	const vector_set<ObjectId>* pChildern = world.getChildensOf(rootActor->getId());
	if (pChildern) {
		for (const ObjectId childId : *pChildern) {
			Actor* child = world.getActorById(childId);
			sgeAssert(child != nullptr);
			if (child) {
				TraitRigidBody* childRBTrait = getTrait<TraitRigidBody>(child);
				if (childRBTrait) {
					processManifolds(childRBTrait->getRigidBody(), false);
				}
			}
		}
	}
}


} // namespace sge
