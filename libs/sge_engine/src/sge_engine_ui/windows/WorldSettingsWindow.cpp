#include "WorldSettingsWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine_ui/ui/UIAssetPicker.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/text/format.h"

namespace sge {
void WorldSettingsWindow::update(SGEContext* const UNUSED(sgecon), struct GameInspector* inspector, const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		GameWorld* world = inspector->getWorld();

		if (ImGui::CollapsingHeader(ICON_FK_CODE "  World Scripts")) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			ImGui::Text("Add World Scripts objects to be executed.");
			std::string label;
			int indexToDelete = -1;
			for (int t = 0; t < int(world->m_scriptObjects.size()); ++t) {
				ImGui::PushID(t);

				string_format(label, "Script %d", t);
				actorPicker(label.c_str(), *world, world->m_scriptObjects[t], nullptr, true);

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

		if (ImGui::CollapsingHeader(ICON_FK_LIGHTBULB_O " Scene Default Lighting")) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			ImGui::ColorEdit3("Ambient Light", world->m_ambientLight.data);
			ImGuiEx::Label("Ambient Intensity");
			ImGui::DragFloat("##Ambient Intensity", &world->m_ambientLightIntensity, 0.01f, 0.f, 100.f);
			ImGuiEx::Label("Ambient Fake Detail");

			float fakeDetailAsPercentage = world->m_ambientLightFakeDetailAmount * 100.f;
			ImGui::DragFloat("##Ambient Fake Detail", &fakeDetailAsPercentage, 0.1f, 0.f, 100.f, "%.3f %%");
			world->m_ambientLightFakeDetailAmount = fakeDetailAsPercentage * 0.01f;
			// ImGui::ColorEdit3("Rim Light", world->m_rimLight.data);
			// ImGui::DragFloat("Rim Width Cosine", &world->m_rimCosineWidth, 0.01f, 0.f, 1.f);
		}

		if (ImGui::CollapsingHeader(ICON_FK_CAMERA " Gameplay")) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			ImGuiEx::Label(ICON_FK_CAMERA " Gameplay Camera");
			actorPicker("##CameraActorPicker", *world, world->m_cameraPovider, nullptr, true);

			ImGui::SameLine();
			if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
				ImGui::OpenPopup("Game Camera Picker");
			}

			if (ImGui::BeginPopup("Game Camera Picker")) {
				world->iterateOverPlayingObjects(
				    [&world](const GameObject* object) -> bool {
					    if (getTrait<TraitCamera>(object)) {
						    bool selected = false;
						    if (ImGui::Selectable(object->getDisplayNameCStr(), &selected)) {
							    world->m_cameraPovider = object->getId();
						    }
					    }

					    return true;
				    },
				    false);

				ImGui::EndPopup();
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_CUBES " Physics")) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			ImGuiEx::Label("Number of Physics Steps per Frame");
			ImGui::DragInt("##SPFPhysics", &world->m_physicsSimNumSubSteps, 0.1f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);

			ImGuiEx::Label("Default Gravity");
			if (ImGui::DragFloat3("##gravityDrag", world->m_defaultGravity.data)) {
				world->setDefaultGravity(world->m_defaultGravity);
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_FILTER " Grid")) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			ImGuiEx::Label("Show Grid");
			ImGui::Checkbox("##ShowGirdCB", &world->gridShouldDraw);

			ImGuiEx::Label("Mark Every:");
			ImGui::DragFloat("##MarkEvery", &world->gridSegmentsSpacing);

			ImGuiEx::Label("Gird Segements:");
			ImGui::DragInt2("##Grid Segments", world->gridNumSegments.data);
		}

		if (ImGui::CollapsingHeader(ICON_FK_BUG " Debugging")) {
			ImGui::InputInt("MS Delay", &world->debug.forceSleepMs, 1, 10);
		}
	}
	ImGui::End();
}
} // namespace sge
