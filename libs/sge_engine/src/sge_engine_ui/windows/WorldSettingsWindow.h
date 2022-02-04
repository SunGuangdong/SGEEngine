#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"
#include <string>

namespace sge {

struct InputState;
struct GameInspector;

struct SGE_ENGINE_API WorldSettingsWindow : public IImGuiWindow {
	WorldSettingsWindow(std::string windowName)
	    : m_windowName(std::move(windowName))
	    {
	}
	void close() override
	{
		m_isOpened = false;
	}

	bool isClosed() override {
		return !m_isOpened;
	}
	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override {
		return m_windowName.c_str();
	}

  private:
	bool m_isOpened = true;
	std::string m_windowName;
};


} // namespace sge
