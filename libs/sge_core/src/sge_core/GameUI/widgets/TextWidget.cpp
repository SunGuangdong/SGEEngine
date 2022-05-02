#include "TextWidget.h"
#include "sge_core/GameUI/UIContext.h"
#include "sge_core/QuickDraw/QuickDraw.h"

namespace sge::gamegui {

void TextWidget::draw(const UIDrawSets& drawSets)
{
	if (m_text.empty()) {
		return;
	}

	QuickFont* const font = m_font ? m_font : getContext().getDefaultFont();

	const Box2f bboxSS = getBBoxPixelsSS();

	const float textHeight = m_fontSize.computeSizePixels(false, bboxSS.size());

	const Box2f textBox = TextRenderer::computeTextMetrics(*font, textHeight, m_text.c_str());
	const vec2f textDim = textBox.size();

	this->getSize().minSizeX = Unit::fromPixels(textDim.x);

	const float textPosX = bboxSS.center().x - textDim.x * 0.5f;
	const float textPosY = bboxSS.center().y + textBox.size().y * 0.5f - textBox.max.y;

	const Rect2s scissors = getScissorRect();
	drawSets.quickDraw->getTextRenderer().drawText2d(
	    *font, textHeight, m_text.c_str(), vec2f(textPosX, textPosY), drawSets.rdest, m_color, &scissors);
}

} // namespace sge::gamegui
