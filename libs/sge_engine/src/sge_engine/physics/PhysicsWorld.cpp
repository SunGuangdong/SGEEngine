#include "PhysicsWorld.h"
#include "BulletHelper.h"
#include "RigidBody.h"

namespace sge {

//-------------------------------------------------------------------------
// PhysicsWorld
//-------------------------------------------------------------------------

/// http://bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Filtering
void sgeNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& dispatchInfo)
{
	btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
	btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

	RigidBody* rb0 = (RigidBody*)colObj0->getUserPointer();
	RigidBody* rb1 = (RigidBody*)colObj1->getUserPointer();

	bool needsToCollide = true;
	if (rb0 && rb1) {
		bool agree0 = rb0->getMaskIdentifiesAs() & rb1->getMaskCollidesWith();
		bool agree1 = rb1->getMaskIdentifiesAs() & rb0->getMaskCollidesWith();

		needsToCollide = agree0 || agree1;
	}

	// Should be called we want a collision to occur.
	if (needsToCollide) {
		dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
	}
}

void PhysicsWorld::create()
{
	destroy();

	broadphase.reset(new btDbvtBroadphase());
	collisionConfiguration.reset(new btDefaultCollisionConfiguration());
	dispatcher.reset(new btCollisionDispatcher(collisionConfiguration.get()));
	solver.reset(new btSequentialImpulseConstraintSolver());

	dispatcher->setNearCallback(sgeNearCallback);
	dynamicsWorld.reset(new btDiscreteDynamicsWorld(dispatcher.get(), broadphase.get(), solver.get(), collisionConfiguration.get()));
	dynamicsWorld->setForceUpdateAllAabbs(false);
}

void PhysicsWorld::destroy()
{
	dynamicsWorld.reset();
	solver.reset();
	dispatcher.reset();
	collisionConfiguration.reset();
	broadphase.reset();
}

void PhysicsWorld::addPhysicsObject(RigidBody& obj)
{
	if (obj.getBulletRigidBody()) {
		dynamicsWorld->addRigidBody(obj.getBulletRigidBody());
	}
	else {
		dynamicsWorld->addCollisionObject(obj.m_collisionObject.get());
	}
}

void PhysicsWorld::removePhysicsObject(RigidBody& obj)
{
	if (obj.getBulletRigidBody()) {
		if (obj.isInWorld()) {
			dynamicsWorld->removeRigidBody(obj.getBulletRigidBody());
		}
	}
}

void PhysicsWorld::setGravity(const vec3f& gravity)
{
	if (dynamicsWorld) {
		dynamicsWorld->setGravity(toBullet(gravity));
	}
}

vec3f PhysicsWorld::getGravity() const
{
	if (dynamicsWorld) {
		return fromBullet(dynamicsWorld->getGravity());
	}

	return vec3f(0.f);
}

} // namespace sge
