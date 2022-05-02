#pragma once

#include "Checkbox.h"
#include "sge_core/GameUI/UIContext.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/QuickDraw.h"
#include "sge_utils/math/color.h"

namespace sge::gamegui {

//----------------------------------------------------
// Checkbox
//----------------------------------------------------
std::shared_ptr<Checkbox>
    Checkbox::create(UIContext& owningContext, Pos position, Size size, const char* const text, bool isOn)
{
	auto checkbox = std::make_shared<gamegui::Checkbox>(owningContext, position, size);
	if (text) {
		checkbox->setText(text);
	}
	checkbox->m_isOn = isOn;

	return checkbox;
}

void Checkbox::draw(const UIDrawSets& drawSets)
{
	using namespace literals;

	const Box2f bboxPixels = getBBoxPixelsSS();
	const Box2f bboxScissors = getScissorBoxSS();
	const Rect2s scissors = getScissorRect();

	if (m_isPressed) {
		drawSets.quickDraw->getTextureDrawer().drawRect(
		    drawSets.rdest, bboxScissors, colorBlack(0.333f), getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}

	if (m_text.empty() == false) {
		const float textHeight = bboxPixels.size().y * 0.8f;
		const Box2f textBox =
		    TextRenderer::computeTextMetrics(*getContext().getDefaultFont(), textHeight, m_text.c_str());
		const vec2f textDim = textBox.size();

		this->getSize().minSizeX = Unit::fromPixels(textDim.x);

		const float textPosX = bboxPixels.center().x - textDim.x * 0.5f;
		const float textPosY = bboxPixels.center().y + textBox.size().y * 0.5f - textBox.max.y;

		drawSets.quickDraw->getTextRenderer().drawText2d(
		    *getContext().getDefaultFont(),
		    textHeight,
		    m_text.c_str(),
		    vec2f(textPosX, textPosY),
		    drawSets.rdest,
		    vec4f(1.f),
		    &scissors);
	}

	// Draw the checkbox square.
	const Pos checboxPos(1_f, 0_f, vec2f(1.f, 0.f));
	const Size checkboxSize(1_hf, 1_hf);

	const Box2f checkBoxRectPixelSpace =
	    checboxPos.getBBoxPixelsSS(bboxPixels, getParentContentOrigin().toPixels(bboxPixels.size()), checkboxSize);
	const vec4f checkBoxColor = m_isOn ? vec4f(0.f, 1.f, 0.f, 1.f) : vec4f(0.3f, 0.3f, 0.3f, 1.f);
	drawSets.quickDraw->getTextureDrawer().drawRect(
	    drawSets.rdest, checkBoxRectPixelSpace, checkBoxColor, getCore()->getGraphicsResources().BS_backToFrontAlpha);
}

bool Checkbox::onPress()
{
	m_isPressed = true;
	return true;
}

void Checkbox::onRelease(bool wasReleaseInside)
{
	m_isPressed = false;
	if (wasReleaseInside) {
		m_isOn = !m_isOn;
		m_onReleaseListeners.invokeEvent(m_isOn);
	}
}

} // namespace sge::gamegui
