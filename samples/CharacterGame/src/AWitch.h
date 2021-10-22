#pragma once

#include "sge_audio/AudioDevice.h"

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"

#include "LevelGen.h"

namespace sge {

struct AWitch : public Actor {
	AudioDecoder bgMusic;

	AWitch() = default;
	AABox3f getBBoxOS() const;
	void create();

	void update(const GameUpdateSets& u);

	void applyDamage();
	bool isDamaged() const { return timeImmune > 0.f; }
	int getHealth() const { return health; }
	bool isDead() const { return health <= 0; }

  public:
	float timeImmune = 0.f;
	int health = 3;

	float currentSpeedX = 0.f;

	TraitRigidBody ttRigidbody;
	TraitSprite ttSprite;

	/// The amount of tilt the witch has when turning, puerly visual.
	float visualXTilt = 0.f;

	float targetWorldCurvatureY = 15.f;
	float targetWorldCurvatureZ = -10.f;
	float nextWorldCurvatureY = 15.f;
	float nextWorldCurvatureZ = 10.f;
	float currentCurvatureRemainingDistance = 100.f;

	LevelInfo levelInfo;
};

} // namespace sge
