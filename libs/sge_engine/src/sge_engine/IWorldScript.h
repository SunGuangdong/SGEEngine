#pragma once

#include "ScriptObject.h"

namespace sge {

struct GameUpdateSets;
struct GameDrawSets;

SGE_NO_WARN_BEGIN
/// @brief An interface for getting callbacks on world events.
/// The object needs to be attached to the world in order to work.
/// See the "World Settings" window to find it.
struct IWorldScript : public Script {
	IWorldScript() = default;
	~IWorldScript() = default;

	/// @brief Called before the world starts update all game objects.
	virtual void onPreUpdate(const GameUpdateSets& u) {}
	/// @brief Called after all game object have been updated.
	virtual void onPostUpdate(const GameUpdateSets& u) {}
	/// @brief Called when rendering the world has finished.
	virtual void onPostDraw(const GameDrawSets& drawSets) {}
};
SGE_NO_WARN_END

} // namespace sge
