#pragma once

#include "sge_engine_api.h"
#include "sge_utils/sge_utils.h"

namespace sge {

struct InputState;
struct GameInspector;
struct GameDrawSets;

struct InspectorToolResult {
	bool isDone = false;
	bool propagateInput = false;
	bool reapply = false; // The tool will be applied again.
};

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct SGE_ENGINE_API IInspectorTool {
	IInspectorTool() = default;
	virtual ~IInspectorTool() = default;

	virtual void onSetActive(GameInspector* UNUSED(inspector)) {}
	virtual InspectorToolResult updateTool(
	    GameInspector* const inspector,
	    bool isAllowedToTakeInput,
	    const InputState& is,
	    const GameDrawSets& drawSets) = 0;
	virtual void onCancel(GameInspector* inspector) = 0;

	virtual void onUI(GameInspector* UNUSED(inspector)) {}

	virtual void drawOverlay(const GameDrawSets& drawSets) = 0;
};

} // namespace sge