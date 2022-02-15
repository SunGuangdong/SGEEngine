#include "ProjectSettingsWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameExport.h"
#include "sge_utils/other/FileOpenDialog.h"
#include "sge_utils/text/Path.h"
#include <filesystem>

namespace sge {
ProjectSettingsWindow::ProjectSettingsWindow(std::string windowName)
    : m_windowName(std::move(windowName))
{
	m_gamePlayerSetting.loadFromJsonFile("appdata/game_project_settings.json");
}

void ProjectSettingsWindow::update(
    SGEContext* const UNUSED(sgecon), struct GameInspector* UNUSED(inspector), const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		ImGuiEx::Label("Window Width");
		ImGui::DragInt("##Window Width", &m_gamePlayerSetting.windowWidth, 1.f, 0, 100000);

		ImGuiEx::Label("Window Heigth");
		ImGui::DragInt("##Window Heigth", &m_gamePlayerSetting.windowHeight, 1.f, 0, 100000);

		ImGuiEx::Label("Is Window Resizable");
		ImGui::Checkbox("##Is Resizable", &m_gamePlayerSetting.windowIsResizable);

		ImGuiEx::Label("Initial Level");
		if (ImGui::Button(ICON_FK_FOLDER_OPEN)) {
			std::string pickedLevel = FileOpenDialog(
			    "Select a the Initial level when the game is launched...", true, "*.lvl\0*.lvl\0", "./assets/levels");
			if (pickedLevel.empty() == false) {
				try {
					pickedLevel = canonizePathRespectOS(std::filesystem::proximate(pickedLevel).string());
					m_gamePlayerSetting.initalLevel = pickedLevel;
				}
				catch (...) {
					sgeLogError("Failed to convert initial level path to relative!");
				}
			}
		}

		ImGui::SameLine();
		ImGuiEx::InputText("##Inital Level input text", m_gamePlayerSetting.initalLevel);

		auto saveSettings = [&]() -> void {
			std::filesystem::create_directories("appdata");
			m_gamePlayerSetting.saveToJsonFile("appdata/game_project_settings.json");
		};

		if (ImGui::Button(ICON_FK_FLOPPY_O " Save")) {
			saveSettings();
		}

		if (ImGui::Button(ICON_FK_PLAY " Run")) {
			saveSettings();
#if WIN32
			system("start sge_player");
#else
			system("./sge_player");
#endif
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_FK_DOWNLOAD " Export")) {
			saveSettings();

			std::string exportFolder = FolderOpenDialog("Pick an Export location:", std::string());
			if (std::filesystem::exists("appdata/game_project_settings.json")) {
				if (exportFolder.empty() == false) {
					exportGame(exportFolder);
				}
			}
			else {
				DialongOk(
				    "Game Export",
				    "Game cannot be exported, as there is no initial project settings specified. To specify them open "
				    "Windows -> "
				    "Project Settings");
			}
		}
	}
	ImGui::End();
}
} // namespace sge
