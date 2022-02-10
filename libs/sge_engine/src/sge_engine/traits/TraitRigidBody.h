#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/physics/RigidBody.h"
#include "sge_engine/physics/CollisionShape.h"

namespace sge {
struct transf3d;
struct EvaluatedModel;

/// @brief TraitRigidBody provides a trait for Actors to have a rigid body in the physics world.
/// You are not obligated to have a rigid body created with this trait you can manually make one yourself.
struct TraitRigidBody;
ReflAddTypeIdExists(TraitRigidBody);
struct SGE_ENGINE_API TraitRigidBody : public Trait {
	SGE_TraitDecl_Full(TraitRigidBody);

	TraitRigidBody() = default;

	~TraitRigidBody();

	RigidBody* getRigidBody()
	{
		return &m_rigidBody;
	}
	const RigidBody* getRigidBody() const
	{
		return &m_rigidBody;
	}

	bool isValid() const
	{
		return m_rigidBody.isValid();
	}

	/// Returns true if the attached rigid body is currently in the game world.
	bool isInWorld() const;

	// Destroys and removes the rigid body from the physics world.
	void destroyRigidBody();

	// Create a rigid body by using the collision shapes described by the 3d model.
	// It is not mandatory to use these functions to create a rigid body. You can get it from @getRigidBody and configure it yourself.
	// @addToWorldNow - will not prevent adding the object to the world in onPlayStateChanged() callback, it means if the object should be
	// added now or not. Use it when the rigid body has changed during an update.
	bool createBasedOnModel(const char* modelPath, float mass, bool noResponse, bool addToWorldNow);
	bool createBasedOnModel(const EvaluatedModel& eval, float mass, bool noResponse, bool addToWorldNow);

	void onPlayStateChanged(bool const isStartingToPlay);
	void setTrasnform(const transf3d& transf, bool killVelocity);
	void addToWorld();
	void removeFromWorld();

  public:
	RigidBody m_rigidBody;
};

} // namespace sge
