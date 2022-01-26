#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"
#include "sge_utils/utils/strings.h"

namespace sge {

void editUI_for_Velocity(GameInspector& inspector, GameObject* gameObject, MemberChain chain)
{
	Velocity* velocity = (Velocity*)chain.follow(gameObject);

	chain.add(sgeFindMember(Velocity, forceType));
	ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
	chain.pop();

	switch (velocity->forceType) {
		case VelictyForce_directional: {
			chain.add(sgeFindMember(Velocity, directional));
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain, ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
			chain.pop();
		} break;
		case VelictyForce_towardsPoint: {
			chain.add(sgeFindMember(Velocity, towardsPoint));
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain, ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
			chain.pop();
		} break;
		case VelictyForce_spherical: {
			chain.add(sgeFindMember(Velocity, spherical));
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain, ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
			chain.pop();
		} break;
		default:
			break;
	}
}

void editUI_for_TraitParticlesSimple(GameInspector& inspector, GameObject* gameObject, MemberChain chain)
{
	TraitParticlesSimple* const ttParticles = (TraitParticlesSimple*)chain.follow(gameObject);
	const TypeDesc* const tdPartDesc = typeLib().find<ParticleGroupDesc>();

	chain.add(sgeFindMember(TraitParticlesSimple, m_isEnabled));
	ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
	chain.pop();

	chain.add(sgeFindMember(TraitParticlesSimple, m_isInWorldSpace));
	ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
	chain.pop();

	// The preview play and restart buttons.
	{
		if (ttParticles->m_isPreviewPlaying) {
			if (ImGui::Button(ICON_FK_STOP " Preview Stop")) {
				ttParticles->clearParticleState();
				ttParticles->m_isPreviewPlaying = false;
			}
		}
		else {
			if (ImGui::Button(ICON_FK_PLAY " Preview Play")) {
				ttParticles->clearParticleState();
				ttParticles->m_isPreviewPlaying = true;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_FK_REFRESH " Preview Restart")) {
			ttParticles->clearParticleState();
			ttParticles->m_isPreviewPlaying = true;
		}
	}

	// Do the select particle group combo.
	const std::string* pSelectedGroupName = nullptr;
	if (ttParticles->m_uiSelectedGroup >= 0 && ttParticles->m_uiSelectedGroup < ttParticles->m_pgroups.size()) {
		pSelectedGroupName = &ttParticles->m_pgroups[ttParticles->m_uiSelectedGroup].m_name;
	}

	bool forceOpenGroupHeader = false;
	if (ImGui::BeginCombo("Edit Group", pSelectedGroupName ? pSelectedGroupName->c_str() : "")) {
		for (int iGrp = 0; iGrp < int(ttParticles->m_pgroups.size()); ++iGrp) {
			ParticleGroupDesc& pdesc = ttParticles->m_pgroups[iGrp];
			if (ImGui::Selectable(pdesc.m_name.c_str(), ttParticles->m_uiSelectedGroup == iGrp)) {
				ttParticles->m_uiSelectedGroup = iGrp;
				forceOpenGroupHeader = true;
			}
		}

		if (ImGui::Selectable("+ Add New", false)) {
			// TODO Member change.
			ttParticles->m_pgroups.emplace_back(ParticleGroupDesc());
			ttParticles->m_pgroups.back().m_name = string_format("New Group %d", ttParticles->m_pgroups.size());
		}

		ImGui::EndCombo();
	}

	/// Show the currently edited particles system in a separate child window.
	if (ImGui::BeginChild("SGEEditParticlesChildWindow")) {
		// Do the group desc user interface.
		if (ttParticles->m_uiSelectedGroup >= 0 && ttParticles->m_uiSelectedGroup < ttParticles->m_pgroups.size()) {
			ParticleGroupDesc& pdesc = ttParticles->m_pgroups[ttParticles->m_uiSelectedGroup];

			if (forceOpenGroupHeader) {
				ImGui::SetNextItemOpen(true);
			}

			if (ImGui::CollapsingHeader(pdesc.m_name.c_str())) {
				chain.add(sgeFindMember(TraitParticlesSimple, m_pgroups), ttParticles->m_uiSelectedGroup);

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_name));
				ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
				chain.pop();

				ImGui::Separator();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::simulationSpeed));
				ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
				chain.pop();

				if (ImGui::CollapsingHeader(ICON_FK_PICTURE_O " Visualization")) {
					ImGuiEx::IndentGuard indent;
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particlesSprite));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spriteGrid));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spriteFPS));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spritePixelsPerUnit));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();
				}

				if (ImGui::CollapsingHeader(ICON_FK_BIRTHDAY_CAKE " Birth Amount")) {
					ImGuiEx::IndentGuard indent;
					chain.add(sgeFindMember(ParticleGroupDesc, birth));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain, ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
					chain.pop();
				}

				if (ImGui::CollapsingHeader(ICON_FK_CUBES " Birth Shape")) {
					ImGuiEx::IndentGuard indent;

					chain.add(sgeFindMember(ParticleGroupDesc, birthShape));

					chain.add(sgeFindMember(BirthShapeDesc, shapeType));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();

					switch (pdesc.birthShape.shapeType) {
						case birthShape_sphere: {
							chain.add(sgeFindMember(BirthShapeDesc, sphere));
							ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain,
							                               ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
							chain.pop();
						} break;
						case birthShape_plane: {
							chain.add(sgeFindMember(BirthShapeDesc, plane));
							ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain,
							                               ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
							chain.pop();
						} break;
						case birthShape_point: {
							chain.add(sgeFindMember(BirthShapeDesc, point));
							ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain,
							                               ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
							chain.pop();
						} break;
						case birthShape_line: {
							chain.add(sgeFindMember(BirthShapeDesc, line));
							ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain,
							                               ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
							chain.pop();
						} break;
						case birthShape_circle: {
							chain.add(sgeFindMember(BirthShapeDesc, circle));
							ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain,
							                               ProperyEditorUIGen::DoMemberUIFlags_structInNoHeader);
							chain.pop();
						} break;
						default:
							break;
					}

					chain.pop();
				}


				if (ImGui::CollapsingHeader(ICON_FK_ARROWS_H " Initial Velocity")) {
					ImGuiEx::IndentGuard indent;
					chain.add(sgeFindMember(ParticleGroupDesc, initialVelocty));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();
				}

				if (ImGui::CollapsingHeader(ICON_FK_ARROWS_ALT " Velocity")) {
					ImGuiEx::IndentGuard indent;
					chain.add(sgeFindMember(ParticleGroupDesc, velocityDrag));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();

					chain.add(sgeFindMember(ParticleGroupDesc, velocityForces));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();
				}

				if (ImGui::CollapsingHeader(ICON_FK_DOT_CIRCLE_O " Alpha")) {
					ImGuiEx::IndentGuard indent;
					chain.add(sgeFindMember(ParticleGroupDesc, alpha));
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
					chain.pop();
				}

				chain.pop();

				// Delete button for the particle group.
				ImGui::Separator();

				if (ImGui::Button("Remove Particle Group")) {
					ttParticles->m_pgroups.erase(ttParticles->m_pgroups.begin() + ttParticles->m_uiSelectedGroup);
					ttParticles->m_uiSelectedGroup--;
				}
			}
		}
	}
	ImGui::EndChild();
}

SgePluginOnLoad()
{
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(Velocity), editUI_for_Velocity);
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitParticlesSimple), editUI_for_TraitParticlesSimple);
}

} // namespace sge
