#pragma once

#include <string>

#include "IImGuiWindow.h"
#include "imgui/imgui.h"

namespace sge {

struct SGE_ENGINE_API InfoWindow : public IImGuiWindow {
	InfoWindow(std::string windowName)
	    : m_windowName(std::move(windowName)) {}
	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

  private:
	bool m_isOpened = true;
	std::string m_windowName;
};

} // namespace sge
