#pragma once

#include "sge_core/typelib/typeLib.h"
#include "sge_engine/GameObject.h"

namespace sge {

ReflAddTypeIdExists(Script);
struct SGE_ENGINE_API Script : public GameObject {
};

} // namespace sge
