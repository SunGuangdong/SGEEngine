#pragma once

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_engine/physics/PhysicsAction.h"
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"


namespace sge {

struct AGhostObject;
struct GhostAction : PhysicsAction {
	virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override;

	AGhostObject* owner;
	btManifoldArray tempManifoldArr;
};

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------
struct SGE_ENGINE_API AGhostObject : public Actor {
	void create() final;
	void postUpdate(const GameUpdateSets& updateSets) final;
	AABox3f getBBoxOS() const final;


	void onPlayStateChanged(bool const isStartingToPlay) override;

  public:
	TraitRigidBody m_traitRB;
	TraitModel m_traitModel;
	TraitSprite m_traitSprite;
	GhostAction action;
};


} // namespace sge
