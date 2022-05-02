#pragma once

#include "sge_core/GameUI/IWidget.h"

namespace sge::gamegui {

/// @brief ColoredWidget is non-interactable widget with a constant color.
struct SGE_CORE_API ColoredWidget final : public IWidget {
	ColoredWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ColoredWidget> create(UIContext& owningContext, Pos position, Size size);

	virtual void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_color = c; }

	bool onMouseWheel(int cnt) override
	{
		Pos co = getContentsOrigin().getAsFraction(getParentBBoxSS().size());
		co.posY.value -= float(cnt) * 0.2f;
		setContentsOrigin(co);
		return true;
	}

  public:
	vec4f m_color = vec4f(1.f);
};

} // namespace sge::gamegui
