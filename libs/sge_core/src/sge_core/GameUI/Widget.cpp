#include <functional>

#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/QuickDraw.h"
#include "sge_core/application/input.h"
#include "sge_utils/math/color.h"

#include "UIContext.h"
#include "Widget.h"

namespace sge::gamegui {

Box2f IWidget::getScissorBoxSS() const
{
	Box2f scissorBboxSS = getBBoxPixelsSS();
	if (auto parent = getParent(); parent) {
		Box2f parentScissorBBoxSS = parent->getScissorBoxSS();
		scissorBboxSS = parentScissorBBoxSS.getOverlapBox(scissorBboxSS);
	}

	return scissorBboxSS;
}

Rect2s IWidget::getScissorRect() const
{
	const Box2f scissorBoxSS = getScissorBoxSS();
	const vec2f bboxSizeSS = scissorBoxSS.size();

	Rect2s scissors;
	scissors.x = short(scissorBoxSS.min.x);
	scissors.y = short(scissorBoxSS.min.y);
	scissors.width = short(bboxSizeSS.x);
	scissors.height = short(bboxSizeSS.y);

	return scissors;
}

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

//----------------------------------------------------
// ImageWidget
//----------------------------------------------------
std::shared_ptr<ImageWidget>
    ImageWidget::create(UIContext& owningContext, Pos position, Unit width, GpuHandle<Texture> texture)
{
	float(texture->getDesc().texture2D.width), float(texture->getDesc().texture2D.height);
	gamegui::Size size;
	size.sizeX = width;
	size.sizeY = width;
	size.sizeY.value *= float(texture->getDesc().texture2D.height) / float(texture->getDesc().texture2D.width);

	auto w = std::make_shared<ImageWidget>(owningContext, position, size);
	w->m_texture = texture;
	return w;
}

std::shared_ptr<ImageWidget>
    ImageWidget::createByHeight(UIContext& owningContext, Pos position, Unit height, GpuHandle<Texture> texture)
{
	float(texture->getDesc().texture2D.width), float(texture->getDesc().texture2D.height);
	gamegui::Size size;
	size.sizeX = height;
	size.sizeY = height;
	size.sizeX.value *= float(texture->getDesc().texture2D.width) / float(texture->getDesc().texture2D.height);

	auto w = std::make_shared<ImageWidget>(owningContext, position, size);
	w->m_texture = texture;
	return w;
}


void ImageWidget::draw(const UIDrawSets& drawSets)
{
	if (m_texture.IsResourceValid()) {
		const Box2f bboxSS = getBBoxPixelsSS();
		float opacity = calcTotalOpacity();
		drawSets.quickDraw->getTextureDrawer().drawRectTexture(
		    drawSets.rdest,
		    bboxSS,
		    m_texture,
		    getCore()->getGraphicsResources().BS_backToFrontAlpha,
		    vec2f(0),
		    vec2f(1.f),
		    opacity);
	}
}

//----------------------------------------------------
// ButtonWidget
//----------------------------------------------------
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

//----------------------------------------------------
// HorizontalComboBox
//----------------------------------------------------
HorizontalComboBox::HorizontalComboBox(UIContext& owningContext, Pos position, Size size)
    : IWidget(owningContext)
{
	setPosition(position);
	setSize(size);
}

std::shared_ptr<HorizontalComboBox> HorizontalComboBox::create(UIContext& owningContext, Pos position, Size size)
{
	using namespace literals;

	std::shared_ptr<HorizontalComboBox> hcombo = std::make_shared<HorizontalComboBox>(owningContext, position, size);

	std::shared_ptr<ButtonWidget> leftBtn =
	    std::make_shared<ButtonWidget>(owningContext, Pos(0_f, 0_f), Size(1_hf, 1_hf));
	std::shared_ptr<ButtonWidget> rightBtn =
	    std::make_shared<ButtonWidget>(owningContext, Pos(1_f, 0_f, vec2f(1.f, 0.f)), Size(1_hf, 1_hf));

	leftBtn->setArrow(axis_x_neg);
	rightBtn->setArrow(axis_x_pos);

	hcombo->m_leftBtn = leftBtn;
	hcombo->m_rightBtn = rightBtn;

	hcombo->m_leftPressSub = leftBtn->subscribe_onRelease([hcombo] { hcombo->leftPressed(); });
	hcombo->m_rightPressSub = rightBtn->subscribe_onRelease([hcombo] { hcombo->rightPressed(); });

	hcombo->addChild(leftBtn);
	hcombo->addChild(rightBtn);

	return hcombo;
}

void HorizontalComboBox::draw(const UIDrawSets& drawSets)
{
	if (m_currentOption >= 0 && m_currentOption < int(m_options.size())) {
		const Box2f bboxPixels = getBBoxPixelsSS();

		const std::string& text = m_options[m_currentOption];

		Pos textAreaPos = Pos::fromHFrac(1.f, 0.f);
		Size textAreaSize;
		textAreaSize.sizeX = Unit::fromPixels(bboxPixels.size().x - 2.f * bboxPixels.size().y);
		textAreaSize.sizeY = Unit::fromPixels(bboxPixels.size().y);

		const float textHeight = bboxPixels.size().y * 0.8f;

		const Box2f textBox =
		    TextRenderer::computeTextMetrics(*getContext().getDefaultFont(), textHeight, text.c_str());
		const vec2f textDim = textBox.size();

		this->getSize().minSizeX = Unit::fromPixels(textDim.x);

		const float textPosX = bboxPixels.center().x - textDim.x * 0.5f;
		const float textPosY = bboxPixels.center().y + textBox.size().y * 0.5f - textBox.max.y;

		const Rect2s scissors = getScissorRect();
		drawSets.quickDraw->getTextRenderer().drawText2d(
		    *getContext().getDefaultFont(),
		    textHeight,
		    text.c_str(),
		    vec2f(textPosX, textPosY),
		    drawSets.rdest,
		    vec4f(1.f),
		    &scissors);
	}
}

void HorizontalComboBox::leftPressed()
{
	m_currentOption--;
	if (m_currentOption < 0) {
		m_currentOption = int(m_options.size()) - 1;
	}
}

void HorizontalComboBox::rightPressed()
{
	m_currentOption++;
	if (m_currentOption >= m_options.size()) {
		if (m_options.empty()) {
			m_currentOption = -1;
		}
		else {
			m_currentOption = 0;
		}
	}
}

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
