#include "HelperDrawRenderItem.h"
#include "sge_engine/Actor.h"

namespace sge {

HelperDrawRenderItem::HelperDrawRenderItem(const SelectedItemDirect& item, DrawReason drawReason)
    : item(item)
    , drawReason(drawReason)
{
	if (Actor* actor = item.gameObject->getActor()) {
		Box3f bboxOs = actor->getBBoxOS();
		if (bboxOs.IsEmpty() == false) {
			zSortingPositionWs = mat_mul_pos(actor->getTransformMtx(), actor->getBBoxOS().center());
		}
	}
	else {
		sgeAssert(false && "It is expected that only actors are renderable!");
	}
}

} // namespace sge
