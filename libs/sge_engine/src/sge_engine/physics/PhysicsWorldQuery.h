#pragma once

#include "BulletHelper.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_utils/containers/StaticArray.h"
#include "sge_utils/containers/vector_map.h"
#include "sge_utils/containers/vector_set.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/vec3f.h"
#include "sge_utils/sge_utils.h"

#include <functional>

SGE_NO_WARN_BEGIN
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btTriangleMeshShape.h>
#include <btBulletDynamicsCommon.h>
SGE_NO_WARN_END

namespace sge {

struct Actor;
struct PhysicsWorld;
struct RigidBody;

namespace PhysicsWorldQuery {

	/// A callback for @boxTest or btCollisionWorld::aabbTest that will contain
	/// all overlapping bodies with the box.
	/// The class will not allocate memory, instead it can handle only fixed amout of collision.
	struct SGE_ENGINE_API BoxTestCallback : public btBroadphaseAabbCallback {
		virtual bool process(const btBroadphaseProxy* proxy) override
		{
			[[maybe_unused]] bool pushSucceeded = proxies.push_back(proxy);
			sgeAssert(pushSucceeded && "Hey, maybe you need more eleements?");

			return true;
		}

		StaticArray<const btBroadphaseProxy*, 16> proxies;
	};

	/// A callback for @boxTest or btCollisionWorld::aabbTest that will contain
	/// all overlapping bodies with the box.
	/// The class will allocate memory, suitable for situations where there might be many
	/// collisions.
	struct SGE_ENGINE_API BoxTestCallbackAlloc : public btBroadphaseAabbCallback {
		virtual bool process(const btBroadphaseProxy* proxy) override
		{
			proxies.push_back(proxy);
			return true;
		}

		std::vector<const btBroadphaseProxy*> proxies;
	};

	SGE_ENGINE_API void
	    rayTest(PhysicsWorld& physWorld, const vec3f& from, const vec3f& to, std::function<void(btDynamicsWorld::LocalRayResult&)> cb);

	/// A callback for finding all potential collisions with thhe specified box.
	/// @param cb a callback to be called. You can use BoxTestCallback for quick results.
	SGE_ENGINE_API void boxTest(PhysicsWorld& physWorld, const Box3f& bbox, btBroadphaseAabbCallback& cb);
}; // namespace PhysicsWorldQuery

//-------------------------------------------------------------------------
// RayResultCollisionObject
//-------------------------------------------------------------------------
struct SGE_ENGINE_API RayResultCollisionObject : public btDynamicsWorld::RayResultCallback {
	typedef bool (*FilterFnType)(const btCollisionObject* btObject);

	RayResultCollisionObject() = default;

	RayResultCollisionObject(const Actor* const igonreActor, const vec3f& rayFromWorld, const vec3f& rayToWorld, FilterFnType filterFn)
	{
		setup(igonreActor, toBullet(rayFromWorld), toBullet(rayToWorld), filterFn);
	}

	RayResultCollisionObject(const Actor* const igonreActor,
	                         const btVector3& rayFromWorld,
	                         const btVector3& rayToWorld,
	                         FilterFnType filterFn)
	{
		setup(igonreActor, rayFromWorld, rayToWorld, filterFn);
	}

	void setup(const Actor* const igonreActor, const btVector3& rayFromWorld, const btVector3& rayToWorld, FilterFnType filterFn)
	{
		m_ignoreActor = igonreActor;
		m_rayFromWorld = rayFromWorld;
		m_rayToWorld = rayToWorld;
		m_filterFn = filterFn;

		m_hitObject = nullptr;
	}

	btScalar addSingleResult(btDynamicsWorld::LocalRayResult& rayResult, bool normalInWorldSpace) final;

	const btCollisionObject* rayTest(const btDynamicsWorld* const dynamicsWorld)
	{
		if_checked(dynamicsWorld)
		{
			dynamicsWorld->rayTest(m_rayFromWorld, m_rayToWorld, *this);
			return m_hitObject;
		}
		return nullptr;
	}

  public:
	FilterFnType m_filterFn = nullptr;
	const Actor* m_ignoreActor = nullptr;
	btVector3 m_rayFromWorld;
	btVector3 m_rayToWorld;

	btVector3 m_hitNormalWorld;
	btVector3 m_hitPointWorld;
	const btCollisionObject* m_hitObject = nullptr;
};


//-------------------------------------------------------------------------
// Raycasting ray result.
//-------------------------------------------------------------------------
struct SGE_ENGINE_API RayResultActor final : public btDynamicsWorld::RayResultCallback {
	RayResultActor() = default;

	RayResultActor(const Actor* const igonreActor, const vec3f& rayFromWorld, const vec3f& rayToWorld)
	{
		setup(igonreActor, toBullet(rayFromWorld), toBullet(rayToWorld));
	}

	RayResultActor(const Actor* const igonreActor, const btVector3& rayFromWorld, const btVector3& rayToWorld)
	{
		setup(igonreActor, rayFromWorld, rayToWorld);
	}

	void setup(const Actor* const igonreActor,
	           const btVector3& rayFromWorld,
	           const btVector3& rayToWorld,
	           std::function<bool(const Actor*)> customFilter = nullptr);

	btScalar addSingleResult(btDynamicsWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override;

	Actor* rayTest(btDynamicsWorld* dynamicsWorld)
	{
		if_checked(dynamicsWorld)
		{
			dynamicsWorld->rayTest(m_rayFromWorld, m_rayToWorld, *this);
			return m_hitActor;
		}
		return nullptr;
	}

  public:
	const Actor* m_ignoreActor = nullptr;
	std::function<bool(const Actor*)> m_customFilter;
	btVector3 m_rayFromWorld;
	btVector3 m_rayToWorld;

	btVector3 m_hitNormalWorld;
	btVector3 m_hitPointWorld;
	Actor* m_hitActor = nullptr;
};

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
// @param shouldFindActorsOnChildrenAsWell true, if we should search for actors sitting on top of our children.
struct FindActorsOnTopResult {
	bool ignoreRotation = false;
};

SGE_ENGINE_API void findActorsOnTop(vector_map<Actor*, FindActorsOnTopResult>& result, Actor* const rootActor);


} // namespace sge
