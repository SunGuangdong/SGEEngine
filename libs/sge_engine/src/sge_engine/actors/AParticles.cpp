#include "AParticles.h"

#include "sge_engine/Actor.h"
#include "sge_utils/utils/strings.h"

namespace sge {

//--------------------------------------------------------------------
// AParticlesSimple
//--------------------------------------------------------------------
ReflAddTypeId(AParticlesSimple, 20'03'02'0052);
// clang-format off

ReflBlock() {
	// AParticlesSimple
	ReflAddActor(AParticlesSimple)
		ReflMember(AParticlesSimple, m_particles)
	;
}
// clang-format on

AABox3f AParticlesSimple::getBBoxOS() const
{
	return m_particles.getBBoxOS();
}

void AParticlesSimple::create()
{
	registerTrait(m_particles);
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/AParticles.png", true);
}







void AParticlesSimple::postUpdate(const GameUpdateSets& u)
{
	m_particles.update(u);
}

} // namespace sge
