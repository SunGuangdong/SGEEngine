#include "GameInspectorWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/text/format.h"

namespace sge {
void GameInspectorWindow::update(SGEContext* const UNUSED(sgecon), GameInspector* inspector, const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		GameWorld* world = inspector->getWorld();

		if (ImGui::Button("Undo")) {
			inspector->undoCommand();
		}

		ImGui::SameLine();

		if (ImGui::Button("Redo")) {
			inspector->redoCommand();
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if (ImGui::Button("Delete") && inspector->hasSelection()) {
			inspector->deleteSelection(false);
		}

		if (ImGui::Button("Delete Hierarchy") && inspector->hasSelection()) {
			inspector->deleteSelection(true);
		}

		if (ImGui::CollapsingHeader(ICON_FK_CODE " Game World Scripts")) {
			std::string label;
			std::string currentObjectName;
			int indexToDelete = -1;
			for (int t = 0; t < int(world->m_scriptObjects.size()); ++t) {
				GameObject* const go = world->getObjectById(world->m_scriptObjects[t]);

				if (go) {
					currentObjectName = go->getDisplayName();
				}
				else {
					currentObjectName = "<not-assigned-object>";
				}

				ImGui::PushID(t);
				string_format(label, "Object %d", t);
				ImGuiEx::Label(label.c_str(), false);

				if (ImGuiEx::InputText("##ObjectName", currentObjectName)) {
					GameObject* newObj = world->getObjectByName(currentObjectName.c_str());
					if (newObj) {
						world->m_scriptObjects[t] = newObj->getId();
					}
				}

				if (ImGui::Button(ICON_FK_EYEDROPPER)) {
					auto& selection = inspector->getSelection();
					if (selection.size() >= 1) {
						GameObject* newObj = world->getObjectById(selection[0].objectId);
						if (newObj) {
							world->m_scriptObjects[t] = newObj->getId();
						}
					}
				}

				ImGui::SameLine();
				if (ImGui::Button(ICON_FK_TRASH)) {
					indexToDelete = t;
				}

				ImGui::PopID();
			}

			if (indexToDelete >= 0) {
				world->m_scriptObjects.erase(world->m_scriptObjects.begin() + indexToDelete);
			}

			if (ImGui::Button(ICON_FK_PLUS " Add")) {
				world->m_scriptObjects.push_back(ObjectId());
			}
		}

		// History.
		if (ImGui::CollapsingHeader("History")) {
			auto commandsNamesGetter = [](void* data, int idx, const char** text) -> bool {
				GameInspector* const inspector = (GameInspector*)data;

				static std::string commandText;

				int const backwardsIdx = int(inspector->m_commandHistory.size()) - idx - 1;

				if (backwardsIdx >= 0 && backwardsIdx < int(inspector->m_commandHistory.size())) {
					inspector->m_commandHistory[backwardsIdx]->getText(commandText);
					*text = commandText.c_str();
					return false;
				}

				return true;
			};

			// Display a list of all commands.
			int curr = 0;
			ImGui::ListBox("Cmd History", &curr, commandsNamesGetter, &inspector, inspector->m_lastExecutedCommandIdx + 1, 5);
		}

		// Stepping.
		ImGui::Separator();
		{
			ImGui::Checkbox("No Auto Step", &inspector->m_disableAutoStepping);

			inspector->m_stepOnce = false;
			if (ImGui::Button("Step Once")) {
				inspector->m_disableAutoStepping = true;
				inspector->m_stepOnce = true;
			}

			ImGui::Text("Steps taken %d", inspector->m_world->totalStepsTaken);
		}

		ImGui::InputInt("MS Delay", &inspector->getWorld()->debug.forceSleepMs, 1, 10);
	}
	ImGui::End();
}
} // namespace sge
