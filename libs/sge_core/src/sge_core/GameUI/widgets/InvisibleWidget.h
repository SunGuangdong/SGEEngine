#pragma once

#include "sge_core/GameUI/UIContext.h"

namespace sge::gamegui {

/// @brief InvisibleWidget is just an invisible widget with size and position
/// useful for position and aligning objects or root widget for menus.
struct SGE_CORE_API InvisibleWidget final : public IWidget {
	InvisibleWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<InvisibleWidget> create(UIContext& owningContext, Pos position, Size size)
	{
		std::shared_ptr<InvisibleWidget> w = std::make_shared<InvisibleWidget>(owningContext, position, size);
		return w;
	}

	virtual void draw(const UIDrawSets& UNUSED(drawSets)) override {}
};

} // namespace sge::gamegui
