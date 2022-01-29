#include "RigidBody.h"
#include "BulletHelper.h"
#include "CollisionShape.h"
#include "sge_engine/Actor.h"

namespace sge {
bool operator==(const btVector3& b, const vec3f& s)
{
	return b.x() == s.x && b.y() == s.y && b.z() == s.z;
}

bool operator!=(const btVector3& b, const vec3f& s)
{
	return !(b == s);
}

bool operator==(const btQuaternion& b, const quatf& s)
{
	return b.x() == s.x && b.y() == s.y && b.z() == s.z && b.w() == s.w;
}

bool operator!=(const btQuaternion& b, const quatf& s)
{
	return !(b == s);
}

//-------------------------------------------------------------------------
// SgeCustomMoutionState
//-------------------------------------------------------------------------
void SgeCustomMoutionState::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
#if 0
	if (m_pRigidBody) {
		centerOfMassWorldTrans = m_pRigidBody->getBulletRigidBody()->getWorldTransform();
	}
#else
	centerOfMassWorldTrans = btTransform::getIdentity();
#endif
}

/// synchronizes world transform from physics to user
/// Bullet only calls the update of worldtransform for active objects
void SgeCustomMoutionState::setWorldTransform(const btTransform& UNUSED(centerOfMassWorldTrans))
{
	if (m_pRigidBody && m_pRigidBody->isValid()) {
#if 0
			m_pRigidBody->actor->setTransformInternal(fromBullet(centerOfMassWorldTrans.getOrigin()), fromBullet(centerOfMassWorldTrans.getRotation()));
#else
		const btTransform btTransf = m_pRigidBody->getBulletRigidBody()->getWorldTransform();
		transf3d newActorTransform = fromBullet(btTransf);
		// Caution:
		// Bullet cannot change the scaling of the object so use the one inside the actor.
		newActorTransform.s = m_pRigidBody->actor->getTransform().s;
		m_pRigidBody->actor->setTransformEx(newActorTransform, false, false, false);
#endif
	}
}

//-------------------------------------------------------------------------
// RigidBody
//-------------------------------------------------------------------------
void RigidBody::create(Actor* const actor, const CollsionShapeDesc* shapeDesc, int numShapeDescs, float const mass, bool noResponce)
{
	CollisionShape* collisionShape = new CollisionShape();
	collisionShape->create(shapeDesc, numShapeDescs);
	create(actor, collisionShape, mass, noResponce);
}

void RigidBody::create(Actor* const actor, CollsionShapeDesc desc, float const mass, bool noResponce)
{
	CollisionShape* collisionShape = new CollisionShape();
	collisionShape->create(&desc, 1);
	create(actor, collisionShape, mass, noResponce);
}

void RigidBody::create(Actor* const actor, CollisionShape* collisionShapeToBeOwned, float const mass, bool noResponce)
{
	// isGhost = false;
	destroy();

	m_collisionShape.reset(collisionShapeToBeOwned);

	btVector3 inertia = btVector3(0.f, 0.f, 0.f);
	if (mass > 0.f) {
		m_collisionShape->getBulletShape()->calculateLocalInertia(mass, inertia);
	}

	m_collisionObject.reset(new btRigidBody(mass, &m_motionState, m_collisionShape->getBulletShape(), inertia));

	m_collisionObject->setRestitution(0.f);
	m_collisionObject->setFriction(1.f);
	m_collisionObject->setRollingFriction(0.0f);
	m_collisionObject->setSpinningFriction(0.0f);

	if (mass == 0.f) {
		getBulletRigidBody()->setActivationState(ISLAND_SLEEPING);
		m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	}
	else {
		getBulletRigidBody()->setActivationState(DISABLE_DEACTIVATION);
	}

	if (noResponce) {
		m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

	this->actor = actor;
	m_collisionObject->setUserPointer(static_cast<RigidBody*>(this));
}

void RigidBody::createGhost(Actor* actor, CollsionShapeDesc* descs, int numDescs, bool noResponse)
{
	destroy();
	CollisionShape* collisionShapeToBeOwned = new CollisionShape();
	collisionShapeToBeOwned->create(descs, numDescs);

	m_collisionShape.reset(collisionShapeToBeOwned);
	btPairCachingGhostObject* ghostBullet = new btPairCachingGhostObject();
	ghostBullet->setCollisionShape(m_collisionShape->getBulletShape());
	m_collisionObject.reset(ghostBullet);


	m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

	if (noResponse) {
		m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

	//btGhostPairCallback;

	this->actor = actor;
	m_collisionObject->setUserPointer(static_cast<RigidBody*>(this));
}

void RigidBody::setNoCollisionResponse(bool dontRespontToCollisions)
{
	if (dontRespontToCollisions) {
		m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}
	else {
		int flags = m_collisionObject->getCollisionFlags();
		flags = flags & ~int(btCollisionObject::CF_NO_CONTACT_RESPONSE);
		m_collisionObject->setCollisionFlags(flags);
	}
}

bool RigidBody::hasNoCollisionResponse() const
{
	if (m_collisionObject == nullptr) {
		return true;
	}

	return (m_collisionObject->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;
}

float RigidBody::getMass() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		float invMass = btrb->getInvMass();
		return (invMass > 0.f) ? 1.f / invMass : 0.f;
	}

	return 0.f;
}

void RigidBody::setMass(float mass)
{
	btRigidBody* btrb = getBulletRigidBody();
	btCollisionShape* btCollsionShape = btrb->getCollisionShape();
	if (btrb && btCollsionShape) {
		btVector3 inertia = btVector3(0.f, 0.f, 0.f);
		btCollsionShape->calculateLocalInertia(mass, inertia);

		btrb->setMassProps(mass, inertia);
	}
}

void RigidBody::setCanMove(bool x, bool y, bool z)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setLinearFactor(btVector3(x ? 1.f : 0.f, y ? 1.f : 0.f, z ? 1.f : 0.f));
	}
}

void RigidBody::getCanMove(bool& x, bool& y, bool& z) const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btVector3 linearFactor = btrb->getLinearFactor();

		x = linearFactor.x() != 0.f;
		y = linearFactor.y() != 0.f;
		z = linearFactor.z() != 0.f;
	}
}

void RigidBody::setCanRotate(bool x, bool y, bool z)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setAngularFactor(btVector3(x ? 1.f : 0.f, y ? 1.f : 0.f, z ? 1.f : 0.f));
	}
}

void RigidBody::getCanRotate(bool& x, bool& y, bool& z) const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btVector3 linearFactor = btrb->getAngularFactor();

		x = linearFactor.x() != 0.f;
		y = linearFactor.y() != 0.f;
		z = linearFactor.z() != 0.f;
	}
}

void RigidBody::setBounciness(float v)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setRestitution(v);
	}
}

float RigidBody::getBounciness() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->getRestitution();
	}

	return 0.f;
}

void RigidBody::setDamping(float linearDamping, float angularDamping)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setDamping(linearDamping, angularDamping);
	}
}

void RigidBody::getDamping(float& linearDamping, float& angularDamping) const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		linearDamping = btrb->getLinearDamping();
		angularDamping = btrb->getAngularDamping();
	}
}

void RigidBody::setFriction(float f)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setFriction(f);
	}
}

float RigidBody::getFriction() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		return btrb->getFriction();
	}

	return 0.f;
}

void RigidBody::setRollingFriction(float f)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setRollingFriction(f);
	}
}

float RigidBody::getRollingFriction() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		return btrb->getRollingFriction();
	}

	return 0.f;
}

void RigidBody::setSpinningFriction(float f)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setSpinningFriction(f);
	}
}
float RigidBody::getSpinningFriction() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		return btrb->getSpinningFriction();
	}

	return 0.f;
}

void RigidBody::setGravity(const vec3f& gravity)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setGravity(toBullet(gravity));
	}
}

vec3f RigidBody::getGravity() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		fromBullet(btrb->getGravity());
	}

	return vec3f(0.f);
}

void RigidBody::applyLinearVelocity(const vec3f& v)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		float invMass = btrb->getInvMass();
		if (invMass != 0.f) {
			btrb->applyCentralImpulse(toBullet(v / invMass));
		}
	}
}

void RigidBody::setLinearVelocity(const vec3f& v)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setLinearVelocity(toBullet(v));
	}
}

void RigidBody::applyAngularVelocity(const vec3f& v)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btVector3 initalAngularVel = btrb->getAngularVelocity();
		btrb->setAngularVelocity(initalAngularVel + toBullet(v));
	}
}

void RigidBody::setAngularVelocity(const vec3f& v)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->setAngularVelocity(toBullet(v));
	}
}

void RigidBody::applyTorque(const vec3f& torque)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->applyTorque(toBullet(torque));
	}
}

void RigidBody::applyForce(const vec3f& f, const vec3f& forcePosition)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->applyForce(toBullet(f), toBullet(forcePosition));
	}
}

void RigidBody::applyForceCentral(const vec3f& f)
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->applyCentralForce(toBullet(f));
	}
}

void RigidBody::clearForces()
{
	btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		btrb->clearForces();
	}
}

vec3f RigidBody::getForces() const
{
	const btRigidBody* btrb = getBulletRigidBody();
	if (btrb) {
		return fromBullet(btrb->getTotalForce());
	}

	return vec3f(0.f);
}

void RigidBody::destroy()
{
	if (getBulletRigidBody()) {
		sgeAssert(getBulletRigidBody()->isInWorld() == false);
	}

	actor = nullptr;
	m_collisionShape.reset(nullptr);
	m_collisionObject.reset(nullptr);
}


transf3d RigidBody::getTransformAndScaling() const
{
	btVector3 const scaling = m_collisionObject->getCollisionShape()->getLocalScaling();
	transf3d tr = fromBullet(m_collisionObject->getWorldTransform());

	tr.s.x = scaling.x();
	tr.s.y = scaling.y();
	tr.s.z = scaling.z();

	return tr;
}

void RigidBody::setTransformAndScaling(const transf3d& tr, bool killVelocity)
{
	if (m_collisionObject == nullptr) {
		return;
	}

	const btTransform& currentWorldTransform = m_collisionObject->getWorldTransform();

	if (currentWorldTransform.getOrigin() != tr.p || currentWorldTransform.getRotation() != tr.r) {
		m_collisionObject->setWorldTransform(toBullet(tr));
		btRigidBody* rigidBodyBullet = getBulletRigidBody();
		if (rigidBodyBullet) {
			rigidBodyBullet->getMotionState()->setWorldTransform(toBullet(tr));
		}
		else {
			btPairCachingGhostObject* ghostBullet = getBulletGhostObject();
			if (ghostBullet) {
				ghostBullet->setWorldTransform(toBullet(tr));
			}
		}
	}

	// Change the scaling only if needed.
	// Dismiss all zero scaling because bullet would not be able simulate properly.
	const btVector3 scaling = toBullet(tr.s);
	const bool isAllScalingNonZero = tr.s.x != 0.f && tr.s.y != 0.f && tr.s.z != 0.f;
	if (isAllScalingNonZero && scaling != m_collisionShape->getBulletShape()->getLocalScaling()) {
		m_collisionShape->getBulletShape()->setLocalScaling(scaling);
	}

	if (killVelocity && getBulletRigidBody()) {
		btVector3 const zero(0.f, 0.f, 0.f);
		getBulletRigidBody()->clearForces();
		getBulletRigidBody()->setLinearVelocity(zero);
		getBulletRigidBody()->setAngularVelocity(zero);
	}
}

btPairCachingGhostObject* RigidBody::getBulletGhostObject()
{
	if (m_collisionObject.get()) {
		return (btPairCachingGhostObject*)btPairCachingGhostObject::upcast(m_collisionObject.get());
	}
	return nullptr;
}

bool RigidBody::isInWorld() const
{
	if (getBulletRigidBody()) {
		return getBulletRigidBody()->isInWorld();
	}
	return false;
}

AABox3f RigidBody::getBBoxWs() const
{
	btVector3 btMin, btMax;
	getBulletRigidBody()->getAabb(btMin, btMax);
	AABox3f result(fromBullet(btMin), fromBullet(btMax));
	return result;
}

const Actor* getOtherActorFromManifold(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx)
{
	const Actor* const a0 = getActorFromPhysicsObject(manifold->getBody0());
	const Actor* const a1 = getActorFromPhysicsObject(manifold->getBody1());

	if (you == a0) {
		if (youIdx)
			*youIdx = 0;
		return a1;
	}
	else if (you == a1) {
		if (youIdx)
			*youIdx = 1;
		return a0;
	}

	if (youIdx)
		*youIdx = -1;
	return nullptr;
}

Actor* getOtherActorFromManifoldMutable(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx)
{
	Actor* const a0 = getActorFromPhysicsObjectMutable(manifold->getBody0());
	Actor* const a1 = getActorFromPhysicsObjectMutable(manifold->getBody1());

	if (you == a0) {
		if (youIdx)
			*youIdx = 0;
		return a1;
	}
	else if (you == a1) {
		if (youIdx)
			*youIdx = 1;
		return a0;
	}

	if (youIdx)
		*youIdx = -1;
	return nullptr;
}

const btCollisionObject* getOtherFromManifold(const btPersistentManifold* const manifold, const btCollisionObject* const you, int* youIdx)
{
	const btCollisionObject* const a0 = manifold->getBody0();
	const btCollisionObject* const a1 = manifold->getBody1();

	if (you == a0) {
		if (youIdx)
			*youIdx = 0;
		return a1;
	}
	else if (you == a1) {
		if (youIdx)
			*youIdx = 1;
		return a0;
	}

	if (youIdx)
		*youIdx = -1;
	return nullptr;
}

const btCollisionObject* getOtherBodyFromManifold(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx)
{
	const Actor* const a0 = getActorFromPhysicsObject(manifold->getBody0());
	const Actor* const a1 = getActorFromPhysicsObject(manifold->getBody1());

	if (you == a0) {
		if (youIdx)
			*youIdx = 0;
		return manifold->getBody1();
	}
	else if (you == a1) {
		if (youIdx)
			*youIdx = 1;
		return manifold->getBody0();
	}

	if (youIdx)
		*youIdx = -1;
	return nullptr;
}


} // namespace sge
