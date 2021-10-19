#pragma once

#include "sge_engine/GameObject.h"
#include "sge_utils/math/Random.h"

namespace sge {
struct LevelInfo {
	bool isInfoGained = false;
	ObjectId roads[3];

	float totalGeneratedDistance = 150.f;

	LevelInfo() = default;
	void generateInteractables(GameWorld* world, float travelDistnaceThisFrame);
};

} // namespace sge
