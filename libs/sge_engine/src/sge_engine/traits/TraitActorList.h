#pragma once

#include "sge_engine/GameObject.h"
#include "sge_utils/containers/vector_set.h"

namespace sge {

ReflAddTypeIdExists(TraitActorList);
struct SGE_ENGINE_API TraitActorList : public Trait {
	SGE_TraitDecl_Full(TraitActorList);

  public:
	vector_set<ObjectId> actorIds;
};

} // namespace sge
