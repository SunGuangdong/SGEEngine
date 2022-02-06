#include "ASky.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"

namespace sge {

// clang-format off
ReflAddTypeId(SkyShaderSettings::Mode, 21'06'23'0001);
ReflAddTypeId(ASky, 21'06'23'0002);

ReflBlock() {
	ReflAddType(SkyShaderSettings::Mode)
		ReflEnumVal(SkyShaderSettings::mode_colorGradinet, "Top-Down Gradient")
		ReflEnumVal(SkyShaderSettings::mode_semiRealistic, "Semi-Realistic")
		ReflEnumVal(SkyShaderSettings::mode_textureSphericalMapped, "Sphere Mapped")
		ReflEnumVal(SkyShaderSettings::mode_textureCubeMapped, "Cube Mapped 2D")
	;

	ReflAddActor(ASky)
		ReflMember(ASky, m_mode)
		ReflMember(ASky, m_topColor).addMemberFlag(MFF_Vec3fAsColor)
		ReflMember(ASky, m_bottomColor).addMemberFlag(MFF_Vec3fAsColor)
		ReflMember(ASky, m_sunDirection)
		ReflMember(ASky, m_textureAssetProp)

	;

}
// clang-format on

void ASky::create()
{
	registerTrait(ttViewportIcon);
	registerTrait(static_cast<IActorCustomAttributeEditorTrait&>(*this));

	ttViewportIcon.setTexture(getEngineGlobal()->getEngineAssets().getIconForObjectType(sgeTypeId(ASky)), true);
}

void ASky::postUpdate(const GameUpdateSets& UNUSED(updateSets))
{
	m_textureAssetProp.update();
}

void ASky::doAttributeEditor(GameInspector* inspector)
{
	MemberChain chain;

	chain.add(typeLib().findMember(&ASky::m_mode));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	if (m_mode == SkyShaderSettings::mode_colorGradinet) {
		chain.add(typeLib().findMember(&ASky::m_topColor));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();

		chain.add(typeLib().findMember(&ASky::m_bottomColor));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}
	else if (m_mode == SkyShaderSettings::mode_semiRealistic) {
		chain.add(typeLib().findMember(&ASky::m_sunDirection));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}
	else if (m_mode == SkyShaderSettings::mode_textureSphericalMapped || m_mode == SkyShaderSettings::mode_textureCubeMapped) {
		chain.add(typeLib().findMember(&ASky::m_textureAssetProp));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}
}

SkyShaderSettings ASky::getSkyShaderSetting() const
{
	SkyShaderSettings result;

	const AssetIface_Texture2D* texIface = m_textureAssetProp.getAssetInterface<AssetIface_Texture2D>();

	result.mode = m_mode;
	result.topColor = m_topColor;
	result.bottomColor = m_bottomColor;
	result.texture = texIface ? texIface->getTexture() : nullptr;
	result.sunDirection = m_sunDirection.getDirection();

	return result;
}

} // namespace sge
