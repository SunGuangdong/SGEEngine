#pragma once

#include "GameInspector.h"
#include "GameWorld.h"
#include "sge_engine/GameDrawer/GameDrawer.h"

#include "GameSerialization.h"

namespace sge {

struct InputState;

struct SGE_ENGINE_API SceneInstance {
	SceneInstance() { newScene(); }

	GameWorld& getWorld() { return m_world; }
	const GameWorld& getWorld() const { return m_world; }
	GameInspector& getInspector() { return m_inspector; }
	const GameInspector& getInspector() const { return m_inspector; }

	void newScene();
	void loadWorldFromJson(const char* const json, bool disableAutoSepping);
	void loadWorldFromFile(const char* const filename, bool disableAutoSepping);
	bool saveWorldToFile(const char* const filename);

	void update(float dt, const InputState& is);

  private:
	GameInspector m_inspector;
	GameWorld m_world;
};

} // namespace sge
