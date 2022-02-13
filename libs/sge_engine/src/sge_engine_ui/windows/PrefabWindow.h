#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"
#include "sge_utils/containers/Optional.h"
#include <string>

namespace sge {

struct GameInspector;

struct SGE_ENGINE_API PrefabWindow : public IImGuiWindow {
	PrefabWindow(std::string windowName)
	    : m_windowName(std::move(windowName))
	{
	}

	void close() override { m_isOpened = false; }

	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

  private:
	bool m_isOpened = true;
	std::string m_windowName;

	Optional<std::vector<std::string>> m_availablePrefabs;

	std::string createPrefabName;
};

} // namespace sge
