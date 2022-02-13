#pragma once

#include "IImGuiWindow.h"

#include "imgui/imgui.h"
#include "sge_engine/GameObject.h"
#include <string>

namespace sge {

struct InputState;
struct GameInspector;

struct SGE_ENGINE_API ActorCreateWindow : public IImGuiWindow {
	ActorCreateWindow(std::string windowName);
	void close() override { m_isOpened = false; }
	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

  private:
	bool m_isOpened = true;
	std::string m_windowName;
	ImGuiTextFilter nodeNamesFilter;
};


} // namespace sge
