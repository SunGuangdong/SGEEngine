#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/behaviours/CharacterController.h"

namespace sge {

/// TraitCharacterController is a trait around @CharacterCtrlDynamic struct.
/// You can use the struct CharacterCtrlDynamic directly, or via this trait.
/// The trait is mainly designed to be able to distiquish between character
/// and non-character actors. Usually used when you want to add some forces
/// to the rigid body of the character, ignore collitions or other logic.
ReflAddTypeIdExists(TraitCharacterController);
struct SGE_ENGINE_API TraitCharacterController final : public Trait {
	SGE_TraitDecl_Full(TraitCharacterController);

	CharacterCtrlDynamic& getCharCtrl() { return m_charCtrl; }
	const CharacterCtrlDynamic& getCharCtrl() const { return m_charCtrl; }

  public:
	CharacterCtrlDynamic m_charCtrl;
};

} // namespace sge
