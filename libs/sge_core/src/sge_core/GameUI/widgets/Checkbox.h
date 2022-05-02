#pragma once

#include "sge_core/GameUI/IWidget.h"
#include "sge_utils/react/Event.h"

namespace sge::gamegui {

//----------------------------------------------------
// Checkbox
//----------------------------------------------------
struct SGE_CORE_API Checkbox final : public IWidget {
	Checkbox(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<Checkbox>
	    create(UIContext& owningContext, Pos position, Size size, const char* const text, bool isOn);
	bool isGamepadTargetable() override { return true; }
	void draw(const UIDrawSets& drawSets) override;
	bool onPress() override;
	void onRelease(bool wasReleaseInside) override;

	void setText(std::string s) { m_text = std::move(s); }

	EventSubscription onRelease(std::function<void(bool)> fn) { return m_onReleaseListeners.subscribe(std::move(fn)); }

  private:
	bool m_isOn = false;
	bool m_isPressed = false;
	std::string m_text;

	EventEmitter<bool> m_onReleaseListeners;
};


} // namespace sge::gamegui
