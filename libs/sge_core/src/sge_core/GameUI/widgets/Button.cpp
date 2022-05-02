#pragma once

#include "Button.h"
#include "sge_core/GameUI/UIContext.h"
#include "sge_core/QuickDraw/QuickDraw.h"
#include "sge_core/ICore.h"

namespace sge::gamegui {

std::shared_ptr<ButtonWidget>
    ButtonWidget::create(UIContext& owningContext, Pos position, Size size, const char* const text)
{
	auto btn = std::make_shared<ButtonWidget>(owningContext, position, size);
	if (text != nullptr) {
		btn->setText(text);
	}
	return btn;
}

void ButtonWidget::draw(const UIDrawSets& drawSets)
{
	const Box2f bboxSS = getBBoxPixelsSS();
	const Box2f bboxScissorsSS = getScissorBoxSS();

	const float textHeight = m_fontSize.computeSizePixels(false, bboxSS.size());

	vec4f bgColor = m_bgColorUp;
	if (m_isPressed) {
		bgColor = m_bgColorPressed;
	}
	else if (m_isHovered) {
		bgColor = m_bgColorHovered;
	}

	drawSets.quickDraw->getTextureDrawer().drawRect(
	    drawSets.rdest, bboxScissorsSS, bgColor, getCore()->getGraphicsResources().BS_backToFrontAlpha);

	QuickFont* const font = m_font ? m_font : getContext().getDefaultFont();
	if (font != nullptr && !m_text.empty()) {
		TextRenderer::TextDisplaySettings displaySets = {
		    textHeight, TextRenderer::horizontalAlign_middle, TextRenderer::verticalAlign_middle};

		const Rect2s scissors = getScissorRect();
		drawSets.quickDraw->getTextRenderer().drawText2d(
		    *font, displaySets, m_text.c_str(), bboxSS.center(), drawSets.rdest, m_textColor, &scissors);
	}

	if (m_text.empty()) {
		const Rect2s scissors = getScissorRect();

		if (m_triangleDir == axis_x_pos) {
			drawSets.quickDraw->getTextureDrawer().drawTriLeft(
			    drawSets.rdest, bboxSS, 0, m_textColor, getCore()->getGraphicsResources().BS_backToFrontAlpha);
		}
		else if (m_triangleDir == axis_y_neg) {
			drawSets.quickDraw->getTextureDrawer().drawTriLeft(
			    drawSets.rdest,
			    bboxSS,
			    deg2rad(90.f),
			    m_textColor,
			    getCore()->getGraphicsResources().BS_backToFrontAlpha);
		}
		else if (m_triangleDir == axis_x_neg) {
			drawSets.quickDraw->getTextureDrawer().drawTriLeft(
			    drawSets.rdest,
			    bboxSS,
			    deg2rad(180.f),
			    m_textColor,
			    getCore()->getGraphicsResources().BS_backToFrontAlpha);
		}
		else if (m_triangleDir == axis_x_neg) {
			drawSets.quickDraw->getTextureDrawer().drawTriLeft(
			    drawSets.rdest,
			    bboxSS,
			    deg2rad(-90.f),
			    m_textColor,
			    getCore()->getGraphicsResources().BS_backToFrontAlpha);
		}
	}
}

} // namespace sge::gamegui
