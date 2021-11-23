#include "ALight.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/typelibHelper.h"

namespace sge {


// clang-format off
ReflAddTypeId(LightType, 20'03'01'0015);
ReflAddTypeId(LightDesc, 20'03'01'0016);
ReflAddTypeId(ALight, 20'03'01'0017);

ReflBlock() {
	ReflAddType(LightType)
		ReflEnumVal(light_point, "light_point")
		ReflEnumVal(light_directional, "light_directional")
		ReflEnumVal(light_spot, "light_spot")
	;

	ReflAddType(LightDesc) ReflMember(LightDesc, isOn)
		ReflMember(LightDesc, type)
		ReflMember(LightDesc, intensity).uiRange(0.01f, 10000.f, 0.01f)
		ReflMember(LightDesc, range).uiRange(0.01f, 10000.f, 0.5f)
		ReflMember(LightDesc, color)
	        .addMemberFlag(MFF_Vec3fAsColor) ReflMember(LightDesc, spotLightAngle)
	        .addMemberFlag(MFF_FloatAsDegrees) ReflMember(LightDesc, hasShadows)
		ReflMember(LightDesc, shadowMapRes)
		ReflMember(LightDesc, shadowMapBias).uiRange(0.f, 100.f, 0.0001f);
	;

	ReflAddActor(ALight)
		ReflMember(ALight, m_lightDesc);
}
// clang-format on


//---------------------------------------------------------------------------
// ALight
//---------------------------------------------------------------------------
void ALight::create() {
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ALight.png", true);
}

AABox3f ALight::getBBoxOS() const {
	switch (m_lightDesc.type) {
		case light_directional: {
			return AABox3f();
		} break;
		case light_point: {
			AABox3f result;
			result.expand(vec3f(vec3f(m_lightDesc.range)));
			result.expand(vec3f(vec3f(-m_lightDesc.range)));
			return result;
		} break;
		case light_spot: {
			const float coneLength = maxOf(m_lightDesc.range, 2.f);
			const float coneRadius = tanf(m_lightDesc.spotLightAngle) * coneLength;
			AABox3f result;
			result.expand(vec3f(0.f));
			result.expand(vec3f(coneLength, coneRadius, coneRadius));
			result.expand(vec3f(coneLength, -coneRadius, -coneRadius));
			return result;
		};
		default:
			sgeAssert(false && "Not implemented for this light type");
			return AABox3f();
	}
}

void ALight::update(const GameUpdateSets& UNUSED(updateSets)) {
}


} // namespace sge
