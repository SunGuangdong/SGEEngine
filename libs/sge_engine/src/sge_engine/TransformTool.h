#pragma once

#include "Actor.h"
#include "InspectorTool.h"
#include "sge_core/TrasnformGizmos/Gizmo3D.h"

#include <vector>

namespace sge {

enum TransformToolOrigin : int {
	transformToolOrigin_lastSelected,
	transformToolOrigin_firstSelected,
	transformToolOrigin_averagePosition,
};

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct SGE_ENGINE_API TransformTool : IInspectorTool {
	int workingSelectionDirtyIndex = 0;

	Gizmo3D::Mode m_mode = Gizmo3D::Mode_Translation;
	bool m_useSnapSettings = true;
	bool m_localSpaceRotation = false;
	GizmoSnapSets m_snapSettings = GizmoSnapSets(vec3f(0.5f), deg2rad(10.f), vec3f(0.5f));
	TransformToolOrigin m_origin = transformToolOrigin_firstSelected;
	Gizmo3D m_gizmo;

	struct PerControlledItemData {
		transf3d initialTasform;  // in world space
		transf3d editedTransform; // in world space
		SelectedItem item;
		bool shouldPerformLocalSpaceTransformChange = false;
		transf3d parentInitialTransform; // in world space
	};

	std::vector<PerControlledItemData> perItemData;

	void onSetActive(GameInspector* const inspector) override final;
	InspectorToolResult updateTool(GameInspector* const inspector,
	                               bool isAllowedToTakeInput,
	                               const InputState& is,
	                               const GameDrawSets& drawSets) override final;
	void onCancel(GameInspector* inspector) override final;

	void onUI(GameInspector* inspector) override final;

	void drawOverlay(const GameDrawSets& drawSets) override final;

  private:
	void clear() { perItemData.clear(); }


	// This is the function that must get called right before you start using the gizmo.
	void prepareForEditing();

	bool interact(const InputState& is, const vec3f& rayOrigin, const vec3f rayDir, const float customScale, bool* hasClickedAway);
};


} // namespace sge
