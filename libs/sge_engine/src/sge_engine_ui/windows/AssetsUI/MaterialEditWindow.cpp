#include "MaterialEditWindow.h"

#include "imgui/imgui.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine_ui/ui/UIAssetPicker.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

MaterialEditWindow::MaterialEditWindow(std::string windowName)
    : m_windowName(std::move(windowName))
{
}

void MaterialEditWindow::update(
    SGEContext* const UNUSED(sgecon), GameInspector* UNUSED(inspector), const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		IMaterial* mtl = nullptr;
		if (mtlProvider) {
			mtl = mtlProvider->getMaterial();
		}

		AssetPtr mtlProviderAsset = std::dynamic_pointer_cast<Asset>(mtlProvider);
		if (isAssetLoaded(mtlProviderAsset)) {
			ImGui::Text("Editing Asset %s", mtlProviderAsset->getPath().c_str());
		}
		else {
			ImGui::Text("You are editing a material witch is not an asset!");
		}

		ImGui::Separator();

		if (mtl) {
			if (const TypeDesc* mtlTypeDesc = typeLib().find(mtl->getTypeId())) {
				MemberChain chain;

				bool hadChange = false;

				for (const MemberDesc& md : mtlTypeDesc->members) {
					ImGuiEx::IDGuard idg(&md);

					chain.add(&md);
					if (md.typeId == sgeTypeId(std::shared_ptr<AssetIface_Texture2D>)) {
						AssetIfaceType assetTypes[] = {
						    assetIface_texture2d,
						};

						std::shared_ptr<AssetIface_Texture2D>* pTexIface =
						    (std::shared_ptr<AssetIface_Texture2D>*)chain.follow(mtl);
						AssetPtr asset = pTexIface ? std::dynamic_pointer_cast<Asset>(*pTexIface) : nullptr;

						if (asset) {
							hadChange |= assetPicker(
							    md.name, asset, getCore()->getAssetLib(), assetTypes, SGE_ARRSZ(assetTypes));
							if (hadChange) {
								*pTexIface = std::dynamic_pointer_cast<AssetIface_Texture2D>(asset);
							}
						}
						else {
							// If nothing is attached, or the attached thing is not an asset,
							// we want to offer to use to attach an asset.
							std::string newAssetPathToAttach;
							hadChange |= assetPicker(
							    md.name,
							    newAssetPathToAttach,
							    getCore()->getAssetLib(),
							    assetTypes,
							    SGE_ARRSZ(assetTypes));
							if (hadChange) {
								*pTexIface = std::dynamic_pointer_cast<AssetIface_Texture2D>(
								    getCore()->getAssetLib()->getAssetFromFile(
								        newAssetPathToAttach.c_str(), nullptr, true));
							}
						}
					}
					else if (md.typeId == sgeTypeId(vec4f)) {
						vec4f& v4 = *(vec4f*)chain.follow(mtl);
						ImGuiEx::Label(md.prettyName.c_str());
						if (md.flags & MFF_Vec4fAsColor) {
							hadChange |= ImGui::ColorEdit4("##colorEdit", v4.data, ImGuiColorEditFlags_InputRGB);
						}
						else {
							hadChange |= SGEImGui::DragFloats("##v4Edit", v4.data, 4, nullptr, nullptr);
						}
					}
					else if (md.typeId == sgeTypeId(vec3f)) {
						vec3f& v3 = *(vec3f*)chain.follow(mtl);
						ImGuiEx::Label(md.prettyName.c_str());
						hadChange |= SGEImGui::DragFloats(
						    "##edit",
						    v3.data,
						    3,
						    nullptr,
						    nullptr,
						    0.f,
						    md.sliderSpeed_float,
						    md.min_float,
						    md.max_float);
					}
					else if (md.typeId == sgeTypeId(vec2f)) {
						vec2f& v2 = *(vec2f*)chain.follow(mtl);
						ImGuiEx::Label(md.prettyName.c_str());
						hadChange |= SGEImGui::DragFloats(
						    "##edit",
						    v2.data,
						    2,
						    nullptr,
						    nullptr,
						    0.f,
						    md.sliderSpeed_float,
						    md.min_float,
						    md.max_float);
					}
					else if (md.typeId == sgeTypeId(float)) {
						float& f = *(float*)chain.follow(mtl);

						if (md.flags & MFF_FloatAsDegrees) {
							f = rad2deg(f);
						}

						ImGuiEx::Label(md.prettyName.c_str());
						hadChange |= SGEImGui::DragFloats(
						    "##edit", &f, 1, nullptr, nullptr, 0.f, md.sliderSpeed_float, md.min_float, md.max_float);

						if (md.flags & MFF_FloatAsDegrees) {
							f = deg2rad(f);
						}
					}
					else if (md.typeId == sgeTypeId(bool)) {
						bool& b = *(bool*)chain.follow(mtl);
						ImGuiEx::Label(md.prettyName.c_str());
						hadChange |= ImGui::Checkbox("##Edit", &b);
					}
					else if (md.typeId == sgeTypeId(std::string)) {
		
						std::string& srcString = *(std::string*)chain.follow(mtl);

						std::string stringEdit = srcString;

						ImGuiEx::Label(md.prettyName.c_str());
						bool const change = ImGuiEx::InputText( "##editString", stringEdit, ImGuiInputTextFlags_EnterReturnsTrue);
						if (change) {
							std::string newData = stringEdit;
							srcString = stringEdit;
							hadChange = true;
						}
					}
					else {
						ImGui::LabelText("UI not written for member %s type.", md.name);
					}

					chain.pop();
				}

				if (hadChange) {
					// If there was a change in the material save it.
					std::shared_ptr<AssetMaterial> asset = std::dynamic_pointer_cast<AssetMaterial>(mtlProvider);
					if (asset) {
						asset->saveAssetToFile(asset->getPath().c_str());
					}
				}
			}
		}
	}
	ImGui::End();
}
} // namespace sge
