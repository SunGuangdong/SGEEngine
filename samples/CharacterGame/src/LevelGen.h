#pragma once

#include "sge_engine/GameObject.h"
#include "sge_utils/math/Random.h"

namespace sge {
struct LevelInfo {
	bool isInfoGained = false;
	ObjectId roads[3];
	Random rnd;

	LevelInfo() { rnd.setSeed(int(size_t(this))); }

	void gainInfo(GameWorld& world);
	void moveLevel(GameWorld* world, float xDiff);
	void populateRoad(Actor* road);
};

} // namespace sge
