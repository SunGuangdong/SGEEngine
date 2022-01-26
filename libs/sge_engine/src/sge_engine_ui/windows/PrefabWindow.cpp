#include "sge_utils/sge_utils.h"
#include <filesystem>

#include "sge_core/SGEImGui.h"

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_renderer/renderer/renderer.h"

#include "IconsForkAwesome/IconsForkAwesome.h"
#include "PrefabWindow.h"
#include "sge_core/ICore.h"
#include "sge_engine//GameSerialization.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine_ui/ui/ImGuiDragDrop.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"

namespace sge {

void PrefabWindow::update(SGEContext* const UNUSED(sgecon), struct GameInspector* inspector, const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		// If we haven't searched for existing prefabs do a search.
		bool shouldOpenCreatePrefabPopup = false;
		if (ImGui::Button("Create from selection...")) {
			shouldOpenCreatePrefabPopup = true;
		}

		if (shouldOpenCreatePrefabPopup) {
			ImGui::OpenPopup("SGEPrefabWindowsCreatePrefabPopup");
			createPrefabName.clear();
		}

		if (ImGui::BeginPopup("SGEPrefabWindowsCreatePrefabPopup")) {
			ImGuiEx::InputText("Prefab Name", createPrefabName);
			if (ImGui::Button(ICON_FK_CHECK " Create")) {
				std::string prefabFilePath = "./assets/prefabs/" + createPrefabName + ".lvl";

				// Create the prefab world.
				vector_set<ObjectId> entitesToSaveInPrefab;
				for (const SelectedItem& item : inspector->m_selection) {
					if (item.editMode != editMode_actors) {
						continue;
					}

					inspector->getWorld()->getAllRelativesOf(entitesToSaveInPrefab, item.objectId);
					entitesToSaveInPrefab.insert(item.objectId);
				}

				if (entitesToSaveInPrefab.empty() == false) {
					JsonValueBuffer jvb;

					GameWorld prefabWorld;
					inspector->getWorld()->createPrefab(prefabWorld, false, &entitesToSaveInPrefab);
					const JsonValue* const jPrefabWorld = serializeGameWorld(&prefabWorld, jvb);

					// Ensure that the prefabs directory exists.
					createDirectory(extractFileDir(prefabFilePath.c_str(), false).c_str());

					JsonWriter jw;
					[[maybe_unused]] bool succeeded = jw.WriteInFile(prefabFilePath.c_str(), jPrefabWorld, true);
					sgeAssert(succeeded);
					getEngineGlobal()->showNotification("Created prefab " + prefabFilePath);

					m_availablePrefabs = NullOptional(); // force prefab window to refresh.
				} else {
					const char* const errorMsg = "Please selected object to create a prefab!";
					getEngineGlobal()->showNotification(errorMsg);
					sgeLogWarn(errorMsg);
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (m_availablePrefabs.isValid() == false) {
			m_availablePrefabs = std::vector<std::string>();
			if (std::filesystem::is_directory("assets/prefabs"))
				for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("./assets/prefabs")) {
					if (entry.path().extension() == ".lvl") {
						std::string prefabPath = entry.path().generic_u8string().c_str();
						m_availablePrefabs->emplace_back(std::move(prefabPath));
					}
				}
		}

		// Create a button for each prefab that can instantiate it.
		if (m_availablePrefabs.isValid()) {
			for (const std::string& prefabPath : m_availablePrefabs.get()) {
				if (ImGui::Selectable(prefabPath.c_str())) {
					inspector->getWorld()->instantiatePrefab(prefabPath.c_str(), true, true, NullOptional());
				}

				if (ImGui::BeginDragDropSource()) {
					DragDropPayloadPrefabFile::setPayload(prefabPath);
					ImGui::Text(prefabPath.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}
	}
	ImGui::End();
}

} // namespace sge
