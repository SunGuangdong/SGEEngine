#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"
#include "sge_engine/GameDrawer/GameDrawer.h"

#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/ModelAnimator2.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/tiny/orbit_camera.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct SGE_ENGINE_API ModelPreviewWidget {
	orbit_camera camera;
	GpuHandle<FrameTarget> m_frameTarget;

	void doWidget(SGEContext* const sgecon, const InputState& is, EvaluatedModel& m_eval, Optional<vec2f> widgetSize = NullOptional());
};


struct ModelPreviewWindow : public IImGuiWindow {
	ModelPreviewWindow(std::string windowName, bool createAsChild = false)
	    : m_windowName(std::move(windowName))
	    , m_createAsChild(createAsChild) {
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
	std::string m_windowName;
	bool m_createAsChild = false;
	bool m_isOpened = true;

	AssetPtr m_model;
	bool m_autoPlay = true;
	GpuHandle<FrameTarget> m_frameTarget;

	orbit_camera camera;

	int iPreviewAnimDonor = -1;
	int iPreviewAnimation = -1;
	bool autoPlayAnimation = true;
	float previewAimationTime = 0;
	std::string animationComboPreviewValue = "<None>";
	std::vector<AssetPtr> animationDonors;

	EvaluatedModel m_eval;
	ModelAnimator2 m_evalAnimator;

	int nextTrackIndex = 0;
	float animationTime = 0.f;
	std::vector<std::string> trackDisplayName;
	std::vector<float> trackAnimDuration;
};


} // namespace sge
