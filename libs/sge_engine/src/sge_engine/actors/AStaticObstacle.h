#pragma once

#include "sge_core/AssetLibrary/AssetLibrary.h"

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------
struct SGE_ENGINE_API AStaticObstacle : public Actor {
	void create() final;
	void postUpdate(const GameUpdateSets& updateSets) final;
	Box3f getBBoxOS() const final;

  public:
	TraitRigidBody m_traitRB;
	TraitModel m_traitModel;
	TraitSprite m_traitSprite;
};

} // namespace sge