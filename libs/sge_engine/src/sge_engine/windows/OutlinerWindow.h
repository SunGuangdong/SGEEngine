#pragma once

#include "IImGuiWindow.h"

#include "imgui/imgui.h"
#include "sge_engine/GameObject.h"
#include <string>

namespace sge {

struct InputState;
struct GameInspector;

struct SGE_ENGINE_API OutlinerWindow : public IImGuiWindow {
	OutlinerWindow(std::string windowName)
	    : m_windowName(std::move(windowName))
	     {}

	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }



  private:
	char m_outlinerFilter[512] = {'*', '\0'};

	bool m_isOpened = true;
	std::string m_windowName;
	ImGuiTextFilter nodeNamesFilter;
	ObjectId m_rightClickedActor;

	bool m_displayObjectIds = false;
};


} // namespace sge
