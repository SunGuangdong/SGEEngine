#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"

#include "sge_utils/time/Timer.h"

#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/SceneInstance.h"

namespace sge {

struct InputState;
struct IGameDrawer;

struct SGE_ENGINE_API GamePlayWindow : public IImGuiWindow {
	GamePlayWindow(std::string windowName, const char* const worldJsonString);

	void close() override
	{
		m_isOpened = false;
	}
	bool isClosed() override
	{
		return !m_isOpened;
	}
	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override
	{
		return m_windowName.c_str();
	}

  private:
	GpuHandle<FrameTarget> m_frameTarget;

	Timer m_timer;
	SceneInstance m_sceneInstance;
	std::unique_ptr<IGameDrawer> m_gameDrawer;

	bool m_isOpened;
	std::string m_windowName;

	bool m_isAllowingRelativeMouse = false;
};


} // namespace sge
