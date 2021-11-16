#define _CRT_SECURE_NO_WARNINGS

#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/ui/ImGuiDragDrop.h"
#include "sge_utils/utils/strings.h"

#include "UIAssetPicker.h"

// imgui.h needs to be already included.
#define IM_VEC2_CLASS_EXTRA
#include <imgui/imconfig.h>

namespace sge {

bool assetPicker(const char* label,
                 std::string& assetPath,
                 AssetLibrary* const assetLibrary,
                 const AssetIfaceType assetTypes[],
                 const int numAssetIfaceTypes) {
	ImGuiEx::IDGuard idGuard(label);
	ImGuiEx::Label(label);

	if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
		ImGui::OpenPopup("Asset Picker");
	}

	ImGui::SameLine();

	float inputTextWidth = ImGui::GetContentRegionAvailWidth();

	bool wasAssetPicked = false;
	ImGui::PushItemWidth(inputTextWidth);
	if (ImGuiEx::InputText("##Asset Picker Text", assetPath, ImGuiInputTextFlags_EnterReturnsTrue)) {
		wasAssetPicked = true;
	}

	if (ImGui::BeginDragDropTarget()) {
		if (Optional<std::string> droppedAssetPath = DragDropPayloadAsset::accept()) {
			assetPath = *droppedAssetPath;
			wasAssetPicked = true;
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::PopItemWidth();

	if (ImGui::BeginPopup("Asset Picker")) {
		static ImGuiTextFilter filter;

		int textureItemsInLine = 0;


		auto doAssetIfaceTypeMenu = [&](const AssetIfaceType assetType) -> void {
			filter.Draw();
			if (ImGui::IsItemClicked(2)) {
				ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
				filter.Clear();
			}

			for (auto& itr : assetLibrary->getAllAssets()) {
				if (isAssetLoaded(itr.second, assetType)) {
					continue;
				}

				if (!filter.PassFilter(itr.first.c_str())) {
					continue;
				}

				const AssetPtr& asset = itr.second;
				if (assetType == assetIface_texture2d) {
					if (true /* isAssetLoadFailed(asset) == false*/) {
						if (!isAssetLoaded(asset)) {
							if (ImGui::Button(itr.first.c_str(), ImVec2(48, 48))) {
								getCore()->getAssetLib()->getAssetFromFile(asset->getPath().c_str());
							}
						} else if (isAssetLoaded(asset, assetIface_texture2d)) {
							Texture* texture = getAssetIface<AssetIface_Texture2D>(asset)->getTexture();
							if (texture && ImGui::ImageButton(texture, ImVec2(48, 48))) {
								assetPath = itr.first;
								wasAssetPicked = true;
							}
						}

						if (ImGui::IsItemHovered()) {
							ImGui::BeginTooltip();
							ImGui::Text(itr.first.c_str());
							ImGui::EndTooltip();
						}

						textureItemsInLine++;

						if (textureItemsInLine == 8)
							textureItemsInLine = 0;
						else
							ImGui::SameLine();
					}
				} else if (assetType == assetIface_model3d) {
					if (isAssetLoaded(asset)) {
						ImGui::Text(ICON_FK_CHECK);
					} else {
						ImGui::Text(ICON_FK_RECYCLE);
					}

					ImGui::SameLine();

					bool selected = itr.first == assetPath;
					if (ImGui::Selectable(itr.first.c_str(), &selected, ImGuiSelectableFlags_DontClosePopups)) {
						if (!isAssetLoaded(asset)) {
							getCore()->getAssetLib()->getAssetFromFile(asset->getPath().c_str());
						} else {
							assetPath = itr.first;
							wasAssetPicked = true;
						}
					}

					if (ImGui::IsItemHovered() && isAssetLoaded(asset, assetIface_model3d)) {
						ImGui::BeginTooltip();

						const int textureSize = 256;
						static float passedTime = 0.f;
						static GpuHandle<FrameTarget> frameTarget;

						passedTime += ImGui::GetIO().DeltaTime;

						if (!frameTarget) {
							frameTarget = getCore()->getDevice()->requestResource<FrameTarget>();
							frameTarget->create2D(textureSize, textureSize);
						}

						const AssetIface_Model3D* mdlIface = getAssetIface<AssetIface_Model3D>(asset);

						getCore()->getDevice()->getContext()->clearColor(frameTarget, 0, vec4f(0.f).data);
						getCore()->getDevice()->getContext()->clearDepth(frameTarget, 1.f);

						const vec3f camPos =
						    mat_mul_pos(mat4f::getRotationY(passedTime * sge2Pi * 0.25f),
						                mdlIface->getStaticEval().aabox.halfDiagonal() * 1.66f + mdlIface->getStaticEval().aabox.center());
						const mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(90.f), 1.f, 0.01f, 10000.f, 0.f, kIsTexcoordStyleD3D);
						const mat4f lookAt = mat4f::getLookAtRH(camPos, vec3f(0.f), vec3f(0.f, kIsTexcoordStyleD3D ? 1.f : -1.f, 0.f));

						RawCamera camera = RawCamera(camPos, lookAt, proj);

						RenderDestination rdest(getCore()->getDevice()->getContext(), frameTarget);

						drawEvalModel(rdest, camera, mat4f::getIdentity(), ObjectLighting(), mdlIface->getStaticEval(), InstanceDrawMods());

						ImGui::Image(frameTarget->getRenderTarget(0), ImVec2(textureSize, textureSize));
						ImGui::EndTooltip();
					}

				} else {
					// Generic for all asset types.
					if (itr.first == assetPath) {
						ImGui::TextUnformatted(itr.first.c_str());
					} else {
						bool selected = false;
						if (ImGui::Selectable(itr.first.c_str(), &selected)) {
							assetPath = itr.first;
							wasAssetPicked = true;
						}
					}
				}
			}
		};

		if (numAssetIfaceTypes == 1) {
			doAssetIfaceTypeMenu(assetTypes[0]);
		} else {
			for (int iType = 0; iType < numAssetIfaceTypes; ++iType) {
				const AssetIfaceType assetType = assetTypes[iType];

				if (ImGui::BeginMenu(assetIface_getName(assetType))) {
					doAssetIfaceTypeMenu(assetType);
					ImGui::EndMenu();
				}
			}
		}
		ImGui::EndPopup();
	}

	return wasAssetPicked;
}

SGE_ENGINE_API bool assetPicker(
    const char* label, AssetPtr& asset, AssetLibrary* const assetLibrary, const AssetIfaceType assetTypes[], const int numAssetIfaceTypes) {
	std::string tempPath = isAssetLoaded(asset) ? asset->getPath() : "";

	if (assetPicker(label, tempPath, assetLibrary, assetTypes, numAssetIfaceTypes)) {
		asset = assetLibrary->getAssetFromFile(tempPath.c_str());
		return true;
	}

	return false;
}

bool actorPicker(
    const char* label, GameWorld& world, ObjectId& ioValue, std::function<bool(const GameObject&)> filter, bool pickPrimarySelection) {
	GameInspector* inspector = world.getInspector();

	if (!inspector) {
		return false;
	}

	if (ImGui::Button(ICON_FK_EYEDROPPER)) {
		ObjectId newPick;
		if (pickPrimarySelection) {
			newPick = inspector->getPrimarySelection();
		} else {
			newPick = inspector->getSecondarySelection();
		}

		if (newPick.isNull() == false) {
			if (filter) {
				GameObject* obj = world.getObjectById(ioValue);
				if (obj) {
					if (filter(*obj)) {
						ioValue = newPick;
					} else {
						getEngineGlobal()->showNotification("This object cannot be picked!");
					}
				}
			} else {
				ioValue = newPick;
			}


			return true;
		} else {
			if (pickPrimarySelection) {
				getEngineGlobal()->showNotification("Select an object to be picked!");
			} else {
				getEngineGlobal()->showNotification("Select a secondary object to be picked!");
			}
		}
	}

	if (pickPrimarySelection) {
		ImGuiEx::TextTooltip("Select an object and click this button to pick it!");
	} else {
		ImGuiEx::TextTooltip("Select a secondary object and click this button to pick it!");
	}
	ImGui::SameLine();

	const GameObject* const initalObject = world.getObjectById(ioValue);
	char currentActorName[256] = {'\0'};
	if (initalObject) {
		sge_strcpy(currentActorName, initalObject->getDisplayNameCStr());
	}

	if (ImGui::InputText(label, currentActorName, SGE_ARRSZ(currentActorName), ImGuiInputTextFlags_EnterReturnsTrue)) {
		const GameObject* const newlyAssignedObject = world.getObjectByName(currentActorName);
		if (newlyAssignedObject) {
			ioValue = newlyAssignedObject->getId();
		} else {
			ioValue = ObjectId();
		}
		return true;
	}

	if (ImGui::BeginDragDropTarget()) {
		bool accepted = false;
		if (Optional<ObjectId> droppedObjid = DragDropPayloadActor::acceptSingle()) {
			ioValue = *droppedObjid;
			accepted = true;
		}

		ImGui::EndDragDropTarget();
		if (accepted) {
			return true;
		}
	}

	return false;
}

SGE_ENGINE_API bool gameObjectTypePicker(const char* label, TypeId& ioValue, const TypeId needsToInherit) {
	ImGuiEx::IDGuard idGuard(label);
	ImGuiEx::Label(label);

	if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
		ImGui::OpenPopup("Type Picker Popup");
	}

	ImGui::SameLine();

	float inputTextWidth = ImGui::GetContentRegionAvailWidth();

	bool wasAssetPicked = false;
	ImGui::PushItemWidth(inputTextWidth);

	const TypeDesc* initialType = typeLib().find(ioValue);

	static std::string currentTypeName;
	currentTypeName = initialType ? initialType->name : "<None>";
	if (ImGuiEx::InputText("##Asset Picker Text", currentTypeName, ImGuiInputTextFlags_EnterReturnsTrue)) {
		const TypeDesc* pickedType = typeLib().findByName(currentTypeName.c_str());
		if (pickedType != nullptr) {
			ioValue = pickedType->typeId;
			wasAssetPicked = true;
		} else {
			sgeLogError("The speicifed type cannot be found!");
		}
	}

	ImGui::PopItemWidth();

	if (ImGui::BeginPopup("Type Picker Popup")) {
		static ImGuiTextFilter filter;
		filter.Draw();
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			filter.Clear();
		}

		for (TypeId typeId : typeLib().m_gameObjectTypes) {
			const TypeDesc* potentialType = typeLib().find(typeId);
			if (potentialType == nullptr) {
				continue;
			}

			if (!needsToInherit.isNull() && !potentialType->doesInherits(needsToInherit)) {
				continue;
			}


			if (!filter.PassFilter(potentialType->name)) {
				continue;
			}

			const AssetIface_Texture2D* texIface =
			    getAssetIface<AssetIface_Texture2D>(getEngineGlobal()->getEngineAssets().getIconForObjectType(potentialType->typeId));

			Texture* const iconTexture = texIface ? texIface->getTexture() : nullptr;

			ImGui::Image(iconTexture, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
			ImGui::SameLine();

			if (ImGui::Selectable(potentialType->name)) {
				ioValue = potentialType->typeId;
				wasAssetPicked = true;
			}
		}
		ImGui::EndPopup();
	}


	return wasAssetPicked;
}


} // namespace sge
