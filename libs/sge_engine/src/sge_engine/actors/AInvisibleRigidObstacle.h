#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitViewportIcon.h"

namespace sge {

//--------------------------------------------------------------------
// AInvisibleRigidObstacle
//--------------------------------------------------------------------
struct SGE_ENGINE_API AInvisibleRigidObstacle : public Actor {
	// Create the ridig body for the actor. The origin is picked to be the bottom mid point
	// so it could be easier to place it over other objects.
	static inline const Box3f kBBoxObjSpace = Box3f::getFromHalfDiagonal(vec3f(0.5f), vec3f(0.5f));

	void create() final;
	Box3f getBBoxOS() const final;

  public:
	TraitRigidBody m_traitRB;
	TraitViewportIcon m_traitViewportIcon;
};

} // namespace sge
