#pragma once

#include "sge_engine/GameObject.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

RelfAddTypeIdExists(TraitActorList);
struct SGE_ENGINE_API TraitActorList : public Trait {
	SGE_TraitDecl_Full(TraitActorList);

  public:
	vector_set<ObjectId> actorIds;
};

} // namespace sge
