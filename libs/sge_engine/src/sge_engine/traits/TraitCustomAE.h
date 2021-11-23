#pragma once

#include "sge_engine/Actor.h"

namespace sge {

struct GameInspector;

ReflAddTypeIdExists(IActorCustomAttributeEditorTrait);
struct SGE_ENGINE_API IActorCustomAttributeEditorTrait : public Trait {
	SGE_TraitDecl_BaseFamily(IActorCustomAttributeEditorTrait)

	    virtual void doAttributeEditor(GameInspector* inspector) = 0;
};
ReflAddTypeIdInline(IActorCustomAttributeEditorTrait, 20'03'06'0006);

} // namespace sge
