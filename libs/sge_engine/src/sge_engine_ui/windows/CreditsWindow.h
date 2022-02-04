#pragma once

#include <string>

#include "IImGuiWindow.h"
#include "imgui/imgui.h"

namespace sge {

struct SGE_ENGINE_API CreditsWindow : public IImGuiWindow {
	CreditsWindow(std::string windowName)
	    : m_windowName(std::move(windowName)) {}

	void close() override
	{
		m_isOpened = false;
	}

	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

  private:
	bool m_isOpened = true;
	std::string m_windowName;
};

} // namespace sge
