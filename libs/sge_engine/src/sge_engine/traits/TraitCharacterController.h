#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/behaviours/CharacterController.h"

namespace sge {

/// TraitCharacterController is a trait around @CharacterCtrl struct.
/// You can use the struct CharacterCtrl directly, or via this trait.
/// The trait is mainly designed to be able to distiquish between character
/// and non-character actors. Usually used when you want to add some forces
/// to the rigid body of the character, ignore collitions or other logic.
RelfAddTypeIdExists(TraitCharacterController);
struct SGE_ENGINE_API TraitCharacterController final : public Trait {
	SGE_TraitDecl_Full(TraitCharacterController);

	CharacterCtrl& getCharCtrl() { return m_charCtrl; }
	const CharacterCtrl& getCharCtrl() const { return m_charCtrl; }

  public:
	CharacterCtrl m_charCtrl;
};

} // namespace sge
