#pragma once

#include "ColoredWidget.h"
#include "sge_core/GameUI/UIContext.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/QuickDraw.h"

namespace sge::gamegui {

//----------------------------------------------------
// ColoredWidget
//----------------------------------------------------
std::shared_ptr<ColoredWidget> ColoredWidget::create(UIContext& owningContext, Pos position, Size size)
{
	auto panel = std::make_shared<ColoredWidget>(owningContext, position, size);
	return panel;
}

void ColoredWidget::draw(const UIDrawSets& drawSets)
{
	const Box2f bboxScissorsSS = getScissorBoxSS();
	drawSets.quickDraw->getTextureDrawer().drawRect(
	    drawSets.rdest,
	    bboxScissorsSS.min.x,
	    bboxScissorsSS.min.y,
	    bboxScissorsSS.size().x,
	    bboxScissorsSS.size().y,
	    m_color,
	    getCore()->getGraphicsResources().BS_backToFrontAlpha);
}

} // namespace sge::gamegui
