#include "ALocator.h"
#include "sge_engine/typelibHelper.h"

namespace sge {

//--------------------------------------------------------------------
// ALocator
//--------------------------------------------------------------------
// clang-format off
ReflAddTypeId(ALocator, 20'03'01'0012);

ReflBlock()
{
	ReflAddActor(ALocator);
}
// clang-format on

//--------------------------------------------------------------------
// ABone
//--------------------------------------------------------------------
// clang-format off
ReflAddTypeId(ABone, 21'01'08'0001);

ReflBlock()
{
	ReflAddActor(ABone)
		ReflMember(ABone, boneLength)
	;
}
// clang-format on

} // namespace sge
