#include "TraitScriptSlot.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"
#include "sge_utils/text/format.h"
#include <vector>

namespace sge {

struct Script;

// clang-format off
ReflAddTypeId(TraitScriptSlot::ScriptSlot, 21'02'01'0001);
ReflAddTypeId(std::vector<TraitScriptSlot::ScriptSlot>, 21'02'01'0002);
ReflAddTypeId(TraitScriptSlot, 21'02'01'0003);

ReflBlock()
{
	ReflAddType(TraitScriptSlot::ScriptSlot)
		ReflMember(TraitScriptSlot::ScriptSlot, slotName)
		ReflMember(TraitScriptSlot::ScriptSlot, scriptObjectId)
	;

	ReflAddType(std::vector<TraitScriptSlot::ScriptSlot>);

	ReflAddType(TraitScriptSlot)
		ReflMember(TraitScriptSlot, slots)
	;
}
// clang-format on

void TraitScriptSlot::addSlot(const char* const name, TypeId possibleType)
{
	ScriptSlot slot;
	slot.slotName = name;
	slot.possibleTypesIncludeList.push_back(possibleType);

	slots.push_back(slot);
}

void TraitScriptSlot_doProperyEditor(GameInspector& inspector, GameObject* const gameObject, MemberChain chain)
{
	TraitScriptSlot& ttScriptSlot = *(TraitScriptSlot*)chain.follow(gameObject);

	ImGuiEx::BeginGroupPanel("Scripts", ImVec2(-1, -1));

	if (ttScriptSlot.slots.size() == 0) {
		ImGui::Text("No slots are defined.");
	}
	else {
		for (int iSlot = 0; iSlot < ttScriptSlot.slots.size(); ++iSlot) {
			TraitScriptSlot::ScriptSlot& slot = ttScriptSlot.slots[iSlot];

			ImGui::PushID(iSlot);
			chain.add(sgeFindMember(TraitScriptSlot, slots), iSlot);
			chain.add(sgeFindMember(TraitScriptSlot::ScriptSlot, scriptObjectId));

			ImGui::Text(slot.slotName.c_str());
			ImGui::SameLine();

			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);

			std::string popUpMenuName = string_format("sgePopUpScriptSlotCreateScript_%d", iSlot);
			if (slot.scriptObjectId.isNull()) {
				ImGui::SameLine();
				if (ImGui::Button("Create")) {
					if (slot.possibleTypesIncludeList.size() == 1 && false) {
					}
					else {
						ImGui::OpenPopup(popUpMenuName.c_str());
					}
				}
			}

			if (ImGui::BeginPopup(popUpMenuName.c_str())) {
				for (auto& typePair : typeLib().m_registeredTypes) {
					if (typePair.second.doesInherits(sgeTypeId(Script)) && typePair.second.newFn != nullptr) {
						const TypeId scriptType = typePair.first;
						const TypeDesc* scriptTd = &typePair.second;

						for (TypeId possibleType : slot.possibleTypesIncludeList) {
							if (scriptType == possibleType || scriptTd->doesInherits(possibleType)) {
								if (ImGui::MenuItem(scriptTd->name)) {
									CmdObjectCreation* cmdScriptCreate = new CmdObjectCreation();
									cmdScriptCreate->setup(scriptType);
									cmdScriptCreate->apply(&inspector);

									CmdMemberChange* cmdAssign = new CmdMemberChange();
									ObjectId originalValue = ObjectId();
									ObjectId createdMaterialId = cmdScriptCreate->getCreatedObjectId();
									cmdAssign->setup(gameObject, chain, &originalValue, &createdMaterialId, nullptr);
									cmdAssign->apply(&inspector);

									CmdCompound* cmdCompound = new CmdCompound();
									cmdCompound->addCommand(cmdScriptCreate);
									cmdCompound->addCommand(cmdAssign);
									inspector.appendCommand(cmdCompound, false);
								}

								// If we rach here, the current script passes the requierments no no need to display it
								// agian.
								break;
							}
						}
					}
				}

				ImGui::EndPopup();
			}

			chain.pop();
			chain.pop();
			ImGui::PopID();
		}
	}
	ImGuiEx::EndGroupPanel();
}

} // namespace sge
