#include "Physics.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "sge_core/model/Model.h"
#include "sge_engine/Actor.h"

namespace sge {

// clang-format off
ReflAddTypeId(CollsionShapeDesc::Type,        21'02'28'0003);
ReflAddTypeId(CollsionShapeDesc,              21'02'28'0004);
ReflAddTypeId(std::vector<CollsionShapeDesc>, 21'02'28'0005);
ReflBlock()
{
	ReflAddType(CollsionShapeDesc::Type)
		ReflEnumVal(CollsionShapeDesc::type_box, "Box")
		ReflEnumVal(CollsionShapeDesc::type_sphere, "Sphere")
		ReflEnumVal(CollsionShapeDesc::type_capsule, "Capsule")
		ReflEnumVal(CollsionShapeDesc::type_cylinder, "Cylinder")
		ReflEnumVal(CollsionShapeDesc::type_cone, "Cone")
		ReflEnumVal(CollsionShapeDesc::type_convexPoly, "Convex")
		ReflEnumVal(CollsionShapeDesc::type_triangleMesh, "Triangle Mesh")
		ReflEnumVal(CollsionShapeDesc::type_infinitePlane, "Plane")
	;

	ReflAddType(CollsionShapeDesc)
		ReflMember(CollsionShapeDesc, type)
		ReflMember(CollsionShapeDesc, offset)
		ReflMember(CollsionShapeDesc, boxHalfDiagonal).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, sphereRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, capsuleHeight).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, capsuleRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, cylinderHalfDiagonal).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, coneHeight).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, coneRadius).uiRange(0.f, 1000000.f, 0.1f)
		ReflMember(CollsionShapeDesc, infinitePlaneNormal)
		ReflMember(CollsionShapeDesc, infinitePlaneConst)
	;

	ReflAddType(std::vector<CollsionShapeDesc>);
}
// clang-format on

//-------------------------------------------------------------------------
// PhysicsWorld
//-------------------------------------------------------------------------
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

void PhysicsWorld::rayTest(const vec3f& from, const vec3f& to, std::function<void(btDynamicsWorld::LocalRayResult&)> lambda)
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
	dynamicsWorld->rayTest(toBullet(from), toBullet(to), rayCB);
}

//// http://bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Filtering
// void PhysicsWorld::dispacherNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo&
// dispatchInfo)
//{
//	// Only dispatch the Bullet collision information if you want the physics to continue.
//	dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
//}

//-------------------------------------------------------------------------
// CollsionShapeDesc
//-------------------------------------------------------------------------
CollsionShapeDesc CollsionShapeDesc::createBox(const vec3f& halfDiagonal, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_box;
	r.boxHalfDiagonal = halfDiagonal * offset.s;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createBox(const AABox3f& box)
{
	sgeAssert(box.IsEmpty() == false);

	CollsionShapeDesc r;
	r.type = type_box;
	r.boxHalfDiagonal = box.halfDiagonal();
	r.offset = transf3d(box.center());

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createSphere(const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_sphere;
	r.sphereRadius = radius * offset.s.hsum() / 3.f;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCapsule(const float height, const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_capsule;
	r.capsuleHeight = height * offset.s.y;
	r.capsuleRadius = radius;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCylinder(const vec3f& halfDiagonal, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_cylinder;
	r.cylinderHalfDiagonal = halfDiagonal * offset.s;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createCylinder(float height, float radius, const transf3d& offset)
{
	vec3f halfDiagonal = vec3f(radius, height * 0.5f, radius);
	return createCylinder(halfDiagonal, offset);
}

CollsionShapeDesc CollsionShapeDesc::createCylinderBottomAligned(float height, float radius, transf3d offset)
{
	offset.p.y += height * 0.5f;
	return createCylinder(height, radius, offset);
}

CollsionShapeDesc CollsionShapeDesc::createCone(const float height, const float radius, const transf3d& offset)
{
	CollsionShapeDesc r;
	r.type = type_cone;
	r.coneHeight = height * offset.s.y;
	r.coneRadius = radius * offset.s.x0z().hsum() / 2.f;
	r.offset = transf3d(offset.p, offset.r, vec3f(1.f));

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createConvexPoly(std::vector<vec3f> verts, std::vector<int> indices)
{
	CollsionShapeDesc r;
	r.type = type_convexPoly;

	r.verticesConvexOrTriMesh = std::move(verts);
	r.indicesConvexOrTriMesh = std::move(indices);

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createTriMesh(std::vector<vec3f> verts, std::vector<int> indices)
{
	CollsionShapeDesc r;
	r.type = type_triangleMesh;

	r.verticesConvexOrTriMesh = std::move(verts);
	r.indicesConvexOrTriMesh = std::move(indices);

	return r;
}

CollsionShapeDesc CollsionShapeDesc::createInfinitePlane(vec3f planeNormal, float planeConstant)
{
	CollsionShapeDesc r;
	r.type = type_infinitePlane;

	r.infinitePlaneNormal = planeNormal;
	r.infinitePlaneConst = planeConstant;

	return r;
}


//-------------------------------------------------------------------------
// CollisionShape
//-------------------------------------------------------------------------
void CollisionShape::create(const CollsionShapeDesc* shapeDescriptors, const int numShapeDescriptors)
{
	destroy();
	m_desc.clear();
	m_desc.reserve(numShapeDescriptors);

	for (int t = 0; t < numShapeDescriptors; ++t) {
		m_desc.push_back(shapeDescriptors[t]);
	}

	struct CreatedShape {
		btCollisionShape* shape = nullptr;
		transf3d offset;
	};

	std::vector<CreatedShape> createdShapes;

	for (CollsionShapeDesc& desc : m_desc) {
		CreatedShape createdShape;

		createdShape.offset = desc.offset;

		switch (desc.type) {
			case CollsionShapeDesc::type_box: {
				createdShape.shape = new btBoxShape(toBullet(desc.boxHalfDiagonal));
			} break;
			case CollsionShapeDesc::type_sphere: {
				createdShape.shape = new btSphereShape(desc.sphereRadius);
			} break;
			case CollsionShapeDesc::type_capsule: {
				createdShape.shape = new btCapsuleShape(desc.capsuleRadius, desc.capsuleHeight);
			} break;
			case CollsionShapeDesc::type_cylinder: {
				createdShape.shape = new btCylinderShape(toBullet(desc.cylinderHalfDiagonal));
			} break;
			case CollsionShapeDesc::type_cone: {
				createdShape.shape = new btConeShape(desc.coneRadius, desc.coneHeight);
			} break;
			case CollsionShapeDesc::type_convexPoly: {
				createdShape.shape = new btConvexHullShape((float*)desc.verticesConvexOrTriMesh.data(),
				                                           int(desc.verticesConvexOrTriMesh.size()), sizeof(vec3f));

				// Caution: [CONVEX_HULLS_TRIANGLE_USER_DATA]
				// The user point here specifies the triangle mesh used
				// to create the convexhull. Bullet doesn't provide a way to store the triangles inside btConvexHullShape
				// however these triangles are needed for the navmesh building.
				createdShape.shape->setUserPointer(&desc);
			} break;
			case CollsionShapeDesc::type_triangleMesh: {
				btTriangleMesh* triMesh = new btTriangleMesh(true, false);
				m_triangleMeshes.push_back(std::make_unique<btTriangleMesh>(triMesh));

				const int numTriangles = int(desc.indicesConvexOrTriMesh.size()) / 3;

				for (int t = 0; t < numTriangles; ++t) {
					const int i0 = desc.indicesConvexOrTriMesh[t * 3 + 0];
					const int i1 = desc.indicesConvexOrTriMesh[t * 3 + 1];
					const int i2 = desc.indicesConvexOrTriMesh[t * 3 + 2];

					const btVector3 v0 = toBullet(desc.verticesConvexOrTriMesh[i0]);
					const btVector3 v1 = toBullet(desc.verticesConvexOrTriMesh[i1]);
					const btVector3 v2 = toBullet(desc.verticesConvexOrTriMesh[i2]);

					triMesh->addTriangle(v0, v1, v2, true);
				}

				createdShape.shape = new btBvhTriangleMeshShape(triMesh, true);

			} break;
			case CollsionShapeDesc::type_infinitePlane: {
				createdShape.shape = new btStaticPlaneShape(toBullet(desc.infinitePlaneNormal), desc.infinitePlaneConst);
			}

			default: {
				sgeAssert("Collision Shape Type not implemented");
			};
		}

		if (createdShape.shape != nullptr) {
			createdShapes.push_back(createdShape);
		}
	}

	// Shortcut for simple collision shape (which is the common case).
	if (createdShapes.size() == 1) {
		if (createdShapes[0].offset == transf3d()) {
			m_btShape.reset(createdShapes[0].shape);
		}
		else {
			btCompoundShape* const compound = new btCompoundShape();
			compound->addChildShape(toBullet(createdShapes[0].offset), createdShapes[0].shape);
			m_btShape.reset(compound);
		}
	}
	else {
		btCompoundShape* const compound = new btCompoundShape();
		for (const CreatedShape& shape : createdShapes) {
			btTransform localTransform = toBullet(shape.offset);
			compound->addChildShape(localTransform, shape.shape);
		}

		m_btShape.reset(compound);
	}
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

	if (noResponse) {
		m_collisionObject->setCollisionFlags(m_collisionObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

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
