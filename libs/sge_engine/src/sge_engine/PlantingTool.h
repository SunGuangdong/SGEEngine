#pragma once

#include <string>

#include "InspectorTool.h"
#include "sge_engine/GameObject.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/vector_set.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/transform.h"

namespace sge {

struct Actor;

/// A tool that uses physics world ray-casts to reposition objects.
struct SGE_ENGINE_API PlantingTool : public IInspectorTool {
	void setup(Actor* actorToPlant);
	void setup(const vector_set<ObjectId>& actorsToPlant, GameWorld& world);

	void onSetActive(GameInspector* const inspector) override final;
	void onUI(GameInspector* inspector) override final;
	InspectorToolResult updateTool(
	    GameInspector* const inspector,
	    bool isAllowedToTakeInput,
	    const InputState& is,
	    const GameDrawSets& drawSets) override final;
	void onCancel(GameInspector* inspector) override final;

	void drawOverlay(const GameDrawSets& drawSets) override final;

	std::string m_modelForStaticObstacle;
	vector_map<ObjectId, transf3d> actorsToPlantOriginalTrasnforms;
	// When planting the object we need to ignore some object hits to make the tool usable.
	// These objects are the ones what we currently "plant" and the others are the children
	// of these objects, as they will move as the tool works.
	vector_set<ObjectId> actorsToIgnorePhysicsHits;

	/// True if the object orientation should match the surface orientation.
	bool shouldRotateObjects = true;
};

} // namespace sge
