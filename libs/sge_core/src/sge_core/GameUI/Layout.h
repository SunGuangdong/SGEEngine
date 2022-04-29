#pragma once

#include "sge_core/sgecore_api.h"

#include "Sizing.h"
#include <memory>
#include <vector>

namespace sge::gamegui {

struct IWidget;
struct InvisibleWidget;

//----------------------------------------------------
// ILayout
//----------------------------------------------------
struct SGE_CORE_API ILayout {
	ILayout(std::weak_ptr<IWidget> ownerWidget);
	virtual ~ILayout() = default;

	IWidget* getOwner() { return m_owner.lock().get(); }

	virtual void update() {}

  private:
	/// The widget which contains this layout
	std::weak_ptr<IWidget> const m_owner;
};


} // namespace sge::gamegui
