#pragma once

#include "InspectorTool.h"
#include "sge_engine/GameObject.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box3f.h"

namespace sge {

struct Actor;

struct SGE_ENGINE_API SelectionToolMode {
	// virtual void begin(GameInspector* inspector) = 0;
	virtual void begin(GameInspector* UNUSED(inspector)) {}
	virtual void end(GameInspector* UNUSED(inspector)) {}
	virtual int getNumItems(GameInspector* inspector) = 0;
	virtual void drawItem(GameInspector* inspector, int const itemIndex, const GameDrawSets& gameDrawSets) = 0;

	// autoSelectHierarchyUnderActorInActorMode is applicable only in actors mode.
	virtual void setSelected(GameInspector* inspector,
	                         int const itemIndex,
	                         bool isSelected,
	                         bool promoteToPrimaryIfAlreadySelected,
	                         bool autoSelectHierarchyUnderActorInActorMode,
	                         bool autoSelectHierarchyAboveActorInActorMode,
	                         bool autoSelectAllRelativesInActorMode) = 0;
	// virtual void end(GameInspector* inspector) = 0;
};

struct SGE_ENGINE_API SelectionToolModeActors : SelectionToolMode {
	int getNumItems(GameInspector* inspector) final;
	void drawItem(GameInspector* inspector, int const itemIndex, const GameDrawSets& gameDrawSets) final;
	void setSelected(GameInspector* inspector,
	                 int const itemIndex,
	                 bool isSelected,
	                 bool promoteToPrimaryIfAlreadySelected,
	                 bool autoSelectHierarchyUnderActorInActorMode,
	                 bool autoSelectHierarchyAboveActorInActorMode,
	                 bool autoSelectAllRelativesInActorMode) final;

	Actor* itemIndexToActor(GameInspector* inspector, int itemIndex);
};

struct SGE_ENGINE_API SelectionToolModePoints : SelectionToolMode {
	void begin(GameInspector* inspector) final;
	void end(GameInspector* inspector) final;
	int getNumItems(GameInspector* inspector) final;
	void drawItem(GameInspector* inspector, int const itemIndex, const GameDrawSets& gameDrawSets) final;
	void setSelected(GameInspector* inspector,
	                 int const itemIndex,
	                 bool isSelected,
	                 bool promoteToPrimaryIfAlreadySelected,
	                 bool autoSelectHierarchyUnderActorInActorMode,
	                 bool autoSelectHierarchyAboveActorInActorMode,
	                 bool autoSelectAllRelativesInActorMode) final;

	std::vector<SelectedItem> items;
};

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct SGE_ENGINE_API SelectionTool : public IInspectorTool {
	// Settings.
	bool m_autoSelectAllChildren = false;
	bool m_autoSelectAllParents = false;
	bool m_autoSelectAllRelatives = false;

	// State.
	bool m_isSelecting = false;
	vec2f m_pickingPointStartCS;
	vec2f m_lastUpdateCursorPos;

	GpuHandle<FrameTarget> m_renderTarget;

	SelectionToolModeActors m_modeActors;
	SelectionToolModePoints m_modePoints;

	void onSetActive(GameInspector* const inspector) override final;
	void onUI(GameInspector* inspector) override final;
	InspectorToolResult updateTool(GameInspector* const inspector,
	                               bool isAllowedToTakeInput,
	                               const InputState& is,
	                               const GameDrawSets& drawSets) override final;
	void onCancel(GameInspector* inspector) override final;

	void drawOverlay(const GameDrawSets& drawSets) override final;

  private:
	void performPicking(GameInspector* inspector, Box2f selectionRectCS, const GameDrawSets& drawSets, bool ctrlDown, bool shiftDown);
};

} // namespace sge
