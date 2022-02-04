#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"
#include "sge_engine/GameDrawer/GameDrawer.h"

#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/ModelAnimator.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/other/SimpleOrbitCamera.h"
#include "sge_utils/containers/Optional.h"

namespace sge {

/// A widget to be embedded when previweing a 3D model in an ImGui window.
struct SGE_ENGINE_API ModelPreviewWidget {
	orbit_camera camera;
	GpuHandle<FrameTarget> m_frameTarget;
	void doWidget(SGEContext* const sgecon, const InputState& is, EvaluatedModel& m_eval, Optional<vec2f> widgetSize = NullOptional());
};

struct ModelPreviewWindow : public IImGuiWindow {
	ModelPreviewWindow(std::string windowName)
	    : m_windowName(std::move(windowName)) {
	}

	void close() override
	{
		m_isOpened = false;
	}

	bool isClosed() override {
		return !m_isOpened;
	}

	const char* getWindowName() const override {
		return m_windowName.c_str();
	}

	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;

	AssetPtr& getModel() {
		return m_model;
	}

	void setPreviewModel(AssetPtr asset);

  private:
	struct AnimationDisplayInfo {
		std::string name;
		float duration = 0.f;
	};

	std::string m_windowName;
	bool m_isOpened = true;

	/// The asset being previwed.
	AssetPtr m_model;

	/// The evaluated model state used for render.
	EvaluatedModel m_eval;
	// The animator that computes the nodes transforms for a perticualr animation.
	ModelAnimator m_evalAnimator;

	/// For preview puropuses each animation is added in a separate animation track.
	/// If the user wants to add more tracks for another 3D model (as we can share animations)
	/// this index should be used to identify the animations.
	int nextTrackIndex = 0;

	bool autoPlayAnimation = true;
	float animationTime = 0.f;

	std::vector<AnimationDisplayInfo> previewAnimationsInfo;

	// The texture we are rendering to.
	GpuHandle<FrameTarget> m_frameTarget;
	orbit_camera camera;
};

} // namespace sge
