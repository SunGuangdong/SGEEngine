#include "sge_core/ICore.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_log/Log.h"

#include "LogWindow.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

namespace sge {

void LogWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		ImGui::Checkbox("Old Messages", &m_showOldMessages);
		ImGui::SameLine();
		ImGui::Checkbox("Info Messages", &m_showInfoMessages);

		const Log& log = *getLog();

		if (ImGui::BeginChild("MessagesWindow", ImVec2(-1.f, -1.f))) {
			const std::vector<Log::Message>& messages = log.getMessages();

			size_t maxNumImagesToDisplay = 0;
			if (!m_showOldMessages) {
				maxNumImagesToDisplay = 40;
			} else {
				maxNumImagesToDisplay = int(messages.size());
			}

			int numDisplayedMessages = 0;
			for (int iMessage = int(messages.size()) - 1; true; --iMessage) {
				// Check if we've displayed enough messages.
				if (iMessage < 0 || numDisplayedMessages == maxNumImagesToDisplay) {
					break;
				}

				const Log::Message& msg = messages[iMessage];
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
					case Log::messageType_log:
						if (m_showInfoMessages) {
							ImGui::TextUnformatted(msg.message.c_str());
						}
						break;
					default: {
						ImGui::TextUnformatted(msg.message.c_str());
					} break;
				}
			}

			// Check if new messages have appeared since the last update.
			// If so, scroll to the bottom of the messages.
			if (prevUpdateMessageCount != int(log.getMessages().size())) {
				ImGui::SetScrollHere(1.0f);
			}
			prevUpdateMessageCount = int(log.getMessages().size());
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

} // namespace sge
