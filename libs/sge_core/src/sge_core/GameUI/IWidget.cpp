#include "IWidget.h"
#include "UIContext.h"

namespace sge::gamegui {
//----------------------------------------------------
// IWidget
//----------------------------------------------------
// std::atomic<int> IWidget::count = 0;

Box2f IWidget::getParentBBoxSS() const
{
	if (auto parent = getParent(); parent) {
		Box2f parentBBox = parent->getBBoxPixelsSS();
		return parentBBox;
	}
	else {
		Box2f result;
		result.expand(vec2f(0.f));
		vec2i canvasSize = m_owningContext.getCanvasSize();
		result.expand(vec2f(float(canvasSize.x), float(canvasSize.y)));
		return result;
	}
}

Pos IWidget::getParentContentOrigin() const
{
	if (auto parent = getParent(); parent) {
		return parent->m_contentsOrigin;
	}
	else {
		return Pos();
	}
}

void IWidget::addChild(std::shared_ptr<IWidget> widget)
{
	auto itr = std::find(m_children.begin(), m_children.end(), widget);
	if (itr == m_children.end()) {
		widget->m_parentWidget = shared_from_this();
		m_children.emplace_back(std::move(widget));
	}
}


} // namespace sge::gamegui
