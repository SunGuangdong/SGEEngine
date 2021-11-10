#include "sge_core/ICore.h"
#include "sge_log/Log.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"

#include "LogWindow.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

namespace sge {

void LogWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		const Log& log = *getLog();

		for (auto& msg : log.getMessages()) {
			switch (msg.type) {
				case Log::messageType_check:
					ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), msg.message.c_str());
					break;
				case Log::messageType_error:
					ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), msg.message.c_str());
					break;
				case Log::messageType_warning:
					ImGui::TextColored(ImVec4(0.f, 1.f, 1.f, 1.f), msg.message.c_str());
					break;
				default:
					ImGui::TextUnformatted(msg.message.c_str());
					break;
			}
		}

		// Check if new messages have appeared since the last update.
		// If so, scroll to the bottom of the messages.
		if (prevUpdateMessageCount != int(log.getMessages().size())) {
			ImGui::SetScrollHere(1.0f);
		}
		prevUpdateMessageCount = int(log.getMessages().size());
	}
	ImGui::End();
}

} // namespace sge
