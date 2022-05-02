#pragma once

#include "ImageWidget.h"
#include "sge_core/GameUI/UIContext.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/QuickDraw.h"

namespace sge::gamegui {

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

} // namespace sge::gamegui
