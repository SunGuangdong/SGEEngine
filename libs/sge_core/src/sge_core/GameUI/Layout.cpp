#include "Layout.h"
#include "Widget.h"

namespace sge::gamegui {

//----------------------------------------------------
//
//----------------------------------------------------
ILayout::ILayout(std::weak_ptr<IWidget> ownerWidget)
    : m_owner(ownerWidget)
{
	m_owner.lock()->setLayout(this);
}

//----------------------------------------------------
//
//----------------------------------------------------

ColumnLayout::ColumnLayout(std::weak_ptr<IWidget> ownerWidget, Pos pos, Size size)
    : ILayout(ownerWidget)
{
	m_columnContainer = std::make_shared<InvisibleWidget>(getOwner()->getContext(), pos, size);
	getOwner()->addChild(m_columnContainer);
	m_spacing = Unit::fromFrac(0.01f);
}


void ColumnLayout::addWidget(std::shared_ptr<IWidget> widget)
{
	auto itr = std::find(m_widgets.begin(), m_widgets.end(), widget);
	if (itr == m_widgets.end()) {
		m_widgets.push_back(widget);
	}

	m_columnContainer->addChild(widget);

	update();
}

void ColumnLayout::update()
{
	float offsetSS = 0;
	float spacingSS = m_spacing.computeSizePixels(false, m_columnContainer->getBBoxPixelsSS().size());

	for (std::shared_ptr<IWidget> w : m_widgets) {
		if (w) {
			w->getPosition().posY = Unit::fromPixels(offsetSS);
			w->getPosition().posX = Unit::fromPixels(0);

			offsetSS += w->getBBoxPixelsSS().size().y + spacingSS;
		}
	}
}

} // namespace sge::gamegui
