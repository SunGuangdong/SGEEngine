#include "sge_core/SGEImGui.h"
#include "sge_utils/sge_utils.h"

#include "HelpWindow.h"

namespace sge {

void HelpWindow::update(SGEContext* const UNUSED(sgecon), struct GameInspector* UNUSED(inspector), const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(300.f, 300.f), ImGuiCond_Appearing);
	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGui::Text("Help yey!");
	}

	ImGui::End();
}

} // namespace sge
