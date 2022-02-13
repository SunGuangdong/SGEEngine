#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct Actor;

/// @brief A small class used to specify render items for helper object visualization.
/// Usually for things that are not visible in the gameplay in any way.
/// Examples are: cameras view area wireframe, lights wireframes visualizing the area effect of the light, splines and so on.
struct HelperDrawRenderItem : public IRenderItem {
	HelperDrawRenderItem() = default;

	HelperDrawRenderItem(const SelectedItemDirect& item, DrawReason drawReason);

	/// The game object to be rendered.
	SelectedItemDirect item;
	DrawReason drawReason;
};
} // namespace sge
