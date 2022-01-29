#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "sge_core/model/Model.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btTriangleMeshShape.h>
#include <btBulletDynamicsCommon.h>
SGE_NO_WARN_END


namespace sge {

struct Actor;
struct RigidBody;

inline btVector3 toBullet(const vec3f& v)
{
	return btVector3(v.x, v.y, v.z);
}

inline vec3f fromBullet(const btVector3& bt)
{
	return vec3f(bt.x(), bt.y(), bt.z());
}

inline btQuaternion toBullet(const quatf v)
{
	return btQuaternion(v.x, v.y, v.z, v.w);
}

inline quatf fromBullet(const btQuaternion& bt)
{
	return quatf(bt.x(), bt.y(), bt.z(), bt.w());
}

inline btTransform toBullet(const transf3d& tr)
{
	return btTransform(toBullet(tr.r), toBullet(tr.p));
}

/// Caution:
/// Scaling in bullet is applied on the rigid body itself, it's not part of the transform!
inline transf3d fromBullet(const btTransform& btTr)
{
	btVector3 const btP = btTr.getOrigin();
	btQuaternion btR;
	btTr.getBasis().getRotation(btR);

	const transf3d res = transf3d(fromBullet(btP), fromBullet(btR), vec3f(1.f));
	return res;
}

/// PhysicsWorld
/// A wrapper around the physics world of the engine that is doing the actual simulation of the object.
/// CAUTION: Do not forget to update the destroy() method!!!
struct SGE_ENGINE_API PhysicsWorld {
	PhysicsWorld() = default;
	~PhysicsWorld()
	{
		destroy();
	}

	void create();
	void destroy();

	void addPhysicsObject(RigidBody& obj);
	void removePhysicsObject(RigidBody& obj);

	/// @brief Changes the default gravity of the world and applies the new gravity to all non-static rigid bodies.
	void setGravity(const vec3f& gravity);

	/// @brief Retrieves the default gravity in the scene.
	vec3f getGravity() const;

	void rayTest(const vec3f& from, const vec3f& to, std::function<void(btDynamicsWorld::LocalRayResult&)> cb);

	// static void dispacherNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo&
	// dispatchInfo);
  public:
	std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
};

/// CollsionShapeDesc
/// @brief This structures stores the information about how to create a single collision shape.
/// Rigid Bodies (or Collision Shapes) might accept multiple descriptions to create compound collision shapes.
struct SGE_ENGINE_API CollsionShapeDesc {
	enum Type : int {
		type_box,
		type_sphere,
		type_capsule,
		type_cylinder,
		type_cone,
		type_convexPoly,
		type_triangleMesh,
		type_infinitePlane,
	};

	CollsionShapeDesc() = default;

	static CollsionShapeDesc createBox(const vec3f& halfDiagonal, const transf3d& offset = transf3d());
	static CollsionShapeDesc createBox(const AABox3f& box);
	static CollsionShapeDesc createSphere(const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCapsule(const float height, const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinder(const vec3f& halfDiagonal, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinder(float height, float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createCylinderBottomAligned(float height, float radius, transf3d offset = transf3d());
	static CollsionShapeDesc createCone(const float height, const float radius, const transf3d& offset = transf3d());
	static CollsionShapeDesc createConvexPoly(std::vector<vec3f> verts, std::vector<int> indices);
	static CollsionShapeDesc createTriMesh(std::vector<vec3f> verts, std::vector<int> indices);

	/// @brief Create an infinite plane rigid body. The body is defined with the canonical plane formula:
	/// Ax + By + Cz + D = 0, where (A,B,C) are the normal of the plane and D is the constant of the plane.
	static CollsionShapeDesc createInfinitePlane(vec3f planeNormal, float planeConstant);

  public:
	Type type = type_box;
	transf3d offset;

	// Boxes.
	vec3f boxHalfDiagonal = vec3f(0.5f);
	// Spheres.
	float sphereRadius = 0.5f;
	// Capsules.
	float capsuleHeight = 0.5f;
	float capsuleRadius = 0.25f;
	// Cylinder.
	vec3f cylinderHalfDiagonal = vec3f(0.5f);
	// Cone.
	float coneHeight = 1.f;
	float coneRadius = 0.5f;
	// Convex or Triangle meshes.
	std::vector<vec3f> verticesConvexOrTriMesh;
	std::vector<int> indicesConvexOrTriMesh;
	// Infinite Plane.
	vec3f infinitePlaneNormal = vec3f::axis_y();
	float infinitePlaneConst = 0.f;
};

/// CollisionShape
/// Represents a collision shape for a rigid body.
/// Do not share it between multiple rigid bodies, as this is possible but not supported by our wrappers yet!
struct SGE_ENGINE_API CollisionShape {
	CollisionShape() = default;
	~CollisionShape()
	{
		destroy();
	}

	void create(const CollsionShapeDesc* desc, const int numDesc);
	void destroy()
	{
		m_triangleMeshes.clear();
		m_btShape.reset(nullptr);
		m_desc.clear();
	}

	btCollisionShape* getBulletShape()
	{
		return m_btShape.get();
	}
	const btCollisionShape* getBulletShape() const
	{
		return m_btShape.get();
	}

  private:
	std::vector<CollsionShapeDesc> m_desc;

	// The main shape used to be attached to the bullet rigid body.
	std::unique_ptr<btCollisionShape> m_btShape;

	// In case the collision shape is represented by concave triangle mesh, this object stores the actual triangles used by bullet physics.
	std::vector<std::unique_ptr<btTriangleMesh>> m_triangleMeshes;
};

/// SgeCustomMoutionState
/// A helper class used to communicate with the physics engine about the location of collsion objects.
struct RigidBody;
struct SGE_ENGINE_API SgeCustomMoutionState : public btMotionState {
	SgeCustomMoutionState(RigidBody* rbTrait)
	    : m_pRigidBody(rbTrait)
	{
	}

	/// synchronizes world transform from user to physics
	void getWorldTransform(btTransform& centerOfMassWorldTrans) const override;

	/// synchronizes world transform from physics to user
	/// Bullet only calls the update of worldtransform for active objects
	void setWorldTransform(const btTransform& centerOfMassWorldTrans) override;

  public:
	RigidBody* m_pRigidBody = nullptr;
};

enum RigidBodyFilterMask : ubyte {
	RigidBodyFilterMask_bitDefault = 1 << 0,
	RigidBodyFilterMask_bit1 = 1 << 1,
	RigidBodyFilterMask_bit2 = 1 << 2,
	RigidBodyFilterMask_bit3 = 1 << 3,
	RigidBodyFilterMask_bit4 = 1 << 4,
	RigidBodyFilterMask_bit5 = 1 << 5,
	RigidBodyFilterMask_bit6 = 1 << 6,
	RigidBodyFilterMask_bit7 = 1 << 7,
};

/// RigidBody
/// This class represents our own wrapper around bullet rigid bodies.
struct SGE_ENGINE_API RigidBody {
	RigidBody()
	    : m_motionState(this)
	{
	}
	~RigidBody()
	{
		destroy();
	}

	void destroy();

	/// @brief Create a rigid body to be used in the PhysicsWorld
	/// @param actor the actor represented by this RigidBody, may be nullptr. Having actor being linked to a rigid body will automatically
	/// update the transformation of the assigned actor, thus multiple rigid bodies cannot be associated with the same actor yet.
	///        Bodies that ARE NOT assigned to any rigid body are useful for sensors.
	///        Keep in mind that the rigid body WILL NOT track the lifetime of the actor by itself (@TraitRigidBody does this tracking).
	/// @param collisionShapeToBeOwned A collision shape to be used for the rigid body. This class will take ownership of the allocated
	///        (with new) collision shape.
	/// @param mass The mass of the object. If 0 the object is going to be static or kinematic.
	/// @param noResponce True if collision with this object should create any new forces. Bullet will still generate contact manifolds.
	///        Useful for triggers and collectables.
	void create(Actor* const actor, CollisionShape* collisionShapeToBeOwned, float const mass, bool noResponce);
	void create(Actor* const actor, const CollsionShapeDesc* shapeDesc, int numShapeDescs, float const mass, bool noResponce);
	void create(Actor* const actor, CollsionShapeDesc desc, float const mass, bool noResponce);


	void createGhost(Actor* actor, CollsionShapeDesc* descs, int numDescs, bool noResponse);

	/// Specifies if the rigid body should not respond to collsions with other objects.
	/// The contant points will still be generated.
	void setNoCollisionResponse(bool dontRespontToCollisions);
	bool hasNoCollisionResponse() const;

	/// @brief Returns true if the RigidBody is properly created.
	bool isValid() const
	{
		return m_collisionObject.get() != nullptr;
	}

	/// @brief Retrieves the mass of the body.
	float getMass() const;

	/// @brief Sets the mass of the body.
	void setMass(float mass);

	/// @brief Disables/Enables movement of the rigid body along the specified axis.
	void setCanMove(bool x, bool y, bool z);

	/// @brief Retrieves if the rigid body is restricted to move in the specified axes.
	void getCanMove(bool& x, bool& y, bool& z) const;

	/// @brief Disables/Enables rotation of the rigid body along the specified axis.
	void setCanRotate(bool x, bool y, bool z);

	/// @brief Retrieves if the rigid body is restricted to rotate around the specified axes.
	void getCanRotate(bool& x, bool& y, bool& z) const;

	/// @brief Changes the restitution of the rigid body. This controls how bouncy is the object.
	void setBounciness(float v);

	/// @brief Retrieves the bounciness of the object.
	float getBounciness() const;

	/// @brief. Changed the damping of the velocity of the object. The dampling is applied always no matter if the object is in contant with
	/// something or in air.
	void setDamping(float linearDamping, float angularDamping);

	/// @brief Retrieves the linear and angular damping of the rigid body.
	void getDamping(float& linearDamping, float& angularDamping) const;

	/// @brief Sets the friction of the rigid body.
	void setFriction(float f);

	/// @brief Retieves the friction of the rigid body.
	float getFriction() const;

	/// @brief Sets the firction when roling along a surface.
	void setRollingFriction(float f);

	/// @brief Retrieves the rolling friction.
	float getRollingFriction() const;

	/// @brief Sets the fricton when spinning around itself on a surface.
	void setSpinningFriction(float f);

	/// @brief Retrieves the spinning friction
	float getSpinningFriction() const;

	/// @brief Sets the gravity of the object.
	void setGravity(const vec3f& gravity);

	/// @brief  Retrieves the gravity used by the object.
	vec3f getGravity() const;

	/// @brief Adds the specified velocity to the rigid body.
	void applyLinearVelocity(const vec3f& v);

	/// @brief Retrieves the linear velocity of the rigid body.
	vec3f getLinearVel() const
	{
		return getBulletRigidBody() ? fromBullet(getBulletRigidBody()->getLinearVelocity()) : vec3f(0.f);
	}

	/// @brief Forces the velocity for the rigid body to be the specified value.
	void setLinearVelocity(const vec3f& v);

	/// @brief Adds the speicified angular velocity to the rigid body.
	void applyAngularVelocity(const vec3f& v);

	/// @brief Forces the angualr velocity to be the specified value.
	void setAngularVelocity(const vec3f& v);

	/// @brief Applies toruqe to the rigid body.
	void applyTorque(const vec3f& torque);

	/// @brief Applies the a force at the specified relative position.
	void applyForce(const vec3f& f, const vec3f& forcePosition);

	/// @brief Applies the specified force
	void applyForceCentral(const vec3f& f);

	/// @brief Removes all applied forces.
	void clearForces();

	/// @brief Retrieves the total force applied to the object.
	vec3f getForces() const;

	/// Including the scaling of the shape.
	transf3d getTransformAndScaling() const;
	void setTransformAndScaling(const transf3d& tr, bool killVelocity);

	btCollisionObject* getCollisionShape()
	{
		return m_collisionObject.get();
	}

	const btCollisionObject* getCollisionShape() const
	{
		return m_collisionObject.get();
	}

	/// @brief Retrieves the Bullet Physics representation of the rigid body.
	btRigidBody* getBulletRigidBody()
	{
		if (m_collisionObject.get()) {
			return btRigidBody::upcast(m_collisionObject.get());
		}

		return nullptr;
	}

	btPairCachingGhostObject* getBulletGhostObject();

	/// @brief Retrieves the Bullet Physics representation of the rigid body.
	const btRigidBody* getBulletRigidBody() const
	{
		if (m_collisionObject.get()) {
			return btRigidBody::upcast(m_collisionObject.get());
		}

		return nullptr;
	}

	/// @brief Retrurns true if the rigid body participates in a physics world.
	bool isInWorld() const;

	/// @brief Returns the axis aligned bounding box according to the physics engine in world space.
	AABox3f getBBoxWs() const;


	ubyte getMaskIdentifiesAs() const
	{
		return m_maskIndetifiedAs;
	}

	void setMaskIdentifiesAs(ubyte mask)
	{
		m_maskIndetifiedAs = mask;
	}

	ubyte getMaskCollidesWith() const
	{
		return m_maskCollidesWith;
	}

	void setMaskCollidesWith(ubyte mask)
	{
		m_maskCollidesWith = mask;
	}

  public:
	std::unique_ptr<btCollisionObject> m_collisionObject;
	SgeCustomMoutionState m_motionState;
	std::unique_ptr<CollisionShape> m_collisionShape;
	Actor* actor = nullptr;

	ubyte m_maskIndetifiedAs = RigidBodyFilterMask_bitDefault;
	ubyte m_maskCollidesWith = RigidBodyFilterMask_bitDefault;
};

/// @brief Retieves our represetentation of the rigid body form btCollisionObject and it's derivatives like btRigidBody.
inline RigidBody* fromBullet(btCollisionObject* const co)
{
	return co ? (RigidBody*)co->getUserPointer() : nullptr;
}

/// @brief Retieves our represetentation of the rigid body form btCollisionObject and it's derivatives like btRigidBody.
inline const RigidBody* fromBullet(const btCollisionObject* const co)
{
	return co ? (const RigidBody*)co->getUserPointer() : nullptr;
}

/// @brief Retieves the Actor associated with the specified btCollisionObject and it's derivatives like btRigidBody.
inline const Actor* getActorFromPhysicsObject(const btCollisionObject* const co)
{
	const RigidBody* rb = fromBullet(co);
	return rb ? rb->actor : nullptr;
}

/// @brief Retieves the Actor associated with the specified btCollisionObject and it's derivatives like btRigidBody.
inline Actor* getActorFromPhysicsObjectMutable(const btCollisionObject* const co)
{
	const RigidBody* rb = fromBullet(co);
	return rb ? rb->actor : nullptr;
}

/// @brief Retieves the other actor participating in the collsion manifold. Useful for sensors and triggers.
SGE_ENGINE_API const Actor*
    getOtherActorFromManifold(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx = nullptr);

/// @brief Retieves the other actor participating in the collsion manifold. Useful for sensors and triggers.
SGE_ENGINE_API Actor*
    getOtherActorFromManifoldMutable(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx = nullptr);

/// @brief Retieves the other actor participating in the collsion manifold. Useful for sensors and triggers.
SGE_ENGINE_API const btCollisionObject*
    getOtherFromManifold(const btPersistentManifold* const manifold, const btCollisionObject* const you, int* youIdx = nullptr);

/// @brief Retieves the other actor participating in the collsion manifold. Useful for sensors and triggers.
SGE_ENGINE_API const btCollisionObject*
    getOtherBodyFromManifold(const btPersistentManifold* const manifold, const Actor* const you, int* youIdx = nullptr);

/// @brief Retieves the other actor participating in the collsion manifold. Useful for sensors and triggers.
SGE_ENGINE_API const Actor*
    getOtherBodyFromManifold(const Actor* const you, const btPersistentManifold* const manifold, int* youIdx = nullptr);

/// Caution:
/// As by default Bullet Physics is built with no RTTI support we cannot use dynamic cast.
/// In order to determine the shape of the object we need to use the enum specifying the type of the shape.
template <typename T>
T* btCollisionShapeCast(btCollisionShape* const shape, const BroadphaseNativeTypes type)
{
	if (shape && shape->getShapeType() == type) {
		return static_cast<T*>(shape);
	}

	return nullptr;
}

/// Caution:
/// As by default Bullet Physics is built with no RTTI support we cannot use dynamic cast.
/// In order to determin the shape of the object we need to use the enum specifying the type of the shape.
template <typename T>
const T* btCollisionShapeCast(const btCollisionShape* const shape, const BroadphaseNativeTypes type)
{
	if (shape && shape->getShapeType() == type) {
		return static_cast<const T*>(shape);
	}

	return nullptr;
}

}; // namespace sge
