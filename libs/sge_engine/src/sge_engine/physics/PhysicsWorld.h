#pragma once

#include "BulletHelper.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/sge_utils.h"

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btTriangleMeshShape.h>
#include <btBulletDynamicsCommon.h>
SGE_NO_WARN_END

#include <memory>

namespace sge {

struct RigidBody;

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

  public:
	std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
};



} // namespace sge
