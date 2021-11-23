#include "MaterialEditWindow.h"

#include "imgui/imgui.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine/ui/UIAssetPicker.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

MaterialEditWindow::MaterialEditWindow(std::string windowName, GameInspector& UNUSED(inspector))
    : m_windowName(std::move(windowName)) {
}

void MaterialEditWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		std::shared_ptr<IMaterial> mtl = mtlIfaceWeak.lock();
		if (mtl) {
			if (const TypeDesc* mtlTypeDesc = typeLib().find(mtl->getTypeId())) {
				MemberChain chain;

				for (const MemberDesc& md : mtlTypeDesc->members) {
					ImGuiEx::IDGuard idg(&md);

					chain.add(&md);
					if (md.typeId == sgeTypeId(AssetPtr)) {
						AssetPtr* pAsset = (AssetPtr*)chain.follow(mtl.get());
						AssetPtr& asset = *pAsset;

						AssetIfaceType assetTypes[] = {
						    assetIface_texture2d,
						};

						assetPicker(md.name, asset, getCore()->getAssetLib(), assetTypes, SGE_ARRSZ(assetTypes));

					} else if (md.typeId == sgeTypeId(vec4f)) {
						vec4f& v4 = *(vec4f*)chain.follow(mtl.get());
						ImGuiEx::Label(md.name);
						if (md.flags & MFF_Vec4fAsColor) {
							ImGui::ColorEdit4("##colorEdit", v4.data, ImGuiColorEditFlags_InputRGB);
						} else {
							SGEImGui::DragFloats("##v4Edit", v4.data, 4, nullptr, nullptr);
						}

					} else if (md.typeId == sgeTypeId(vec2f)) {
						vec2f& v2 = *(vec2f*)chain.follow(mtl.get());
						ImGuiEx::Label(md.name);
						SGEImGui::DragFloats("##edit", v2.data, 2, nullptr, nullptr, 0.f, md.sliderSpeed_float, md.min_float, md.max_float);
					} else if (md.typeId == sgeTypeId(float)) {
						float& f = *(float*)chain.follow(mtl.get());

						if (md.flags & MFF_FloatAsDegrees) {
							f = rad2deg(f);
						}

						ImGuiEx::Label(md.name);
						SGEImGui::DragFloats("##edit", &f, 1, nullptr, nullptr, 0.f, md.sliderSpeed_float, md.min_float, md.max_float);

						if (md.flags & MFF_FloatAsDegrees) {
							f = deg2rad(f);
						}
					} else if (md.typeId == sgeTypeId(bool)) {
						bool& b = *(bool*)chain.follow(mtl.get());
						ImGuiEx::Label(md.name);
						ImGui::Checkbox("##Edit", &b);
					} else {
						ImGui::LabelText("UI not written for member %s type.", md.name);
					}

					chain.pop();
				}
			}
		}
	}
	ImGui::End();
}
} // namespace sge
