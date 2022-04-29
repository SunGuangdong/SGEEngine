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


} // namespace sge::gamegui
