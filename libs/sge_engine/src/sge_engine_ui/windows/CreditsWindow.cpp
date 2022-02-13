#include "sge_core/SGEImGui.h"
#include "sge_utils/credits/sge_all_3rd_party_credits.h"
#include "sge_utils/sge_utils.h"

#include "CreditsWindow.h"

namespace sge {

void CreditsWindow::update(SGEContext* const UNUSED(sgecon), GameInspector* UNUSED(inspector), const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(300.f, 300.f), ImGuiCond_Appearing);
	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGui::TextUnformatted(get_sge_all_3rd_party_credits());
	}

	ImGui::End();
}

} // namespace sge
