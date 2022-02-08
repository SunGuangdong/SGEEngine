#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/RigidBodyEditorConfig.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct AInfinitePlaneObstacle : public Actor {
	void create() final;
	void onPlayStateChanged(bool const isStartingToPlay) override;
	Box3f getBBoxOS() const final;

  public:
	TraitRigidBody ttRigidBody;
	float displayScale = 1.f;
	RigidBodyPropertiesConfigurator rbPropConfig;
};

} // namespace sge
